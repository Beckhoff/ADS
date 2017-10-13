/**
   Copyright (c) 2015 Beckhoff Automation GmbH & Co. KG

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
 */

#include "AmsConnection.h"
#include "Log.h"

AmsResponse::AmsResponse()
    : frame(4096),
    invokeId(0),
    errorCode(0)
{}

void AmsResponse::Notify()
{
    invokeId = 0;
    cv.notify_all();
}

bool AmsResponse::Wait(uint32_t timeout_ms)
{
    std::unique_lock<std::mutex> lock(mutex);
    return cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&]() {
        return !invokeId;
    });
}

std::shared_ptr<NotificationDispatcher> AmsConnection::DispatcherListAdd(const VirtualConnection& connection)
{
    const auto dispatcher = DispatcherListGet(connection);
    if (dispatcher) {
        return dispatcher;
    }
    std::lock_guard<std::recursive_mutex> lock(dispatcherListMutex);
    return dispatcherList.emplace(connection,
                                  std::make_shared<NotificationDispatcher>(*this, connection)).first->second;
}

std::shared_ptr<NotificationDispatcher> AmsConnection::DispatcherListGet(const VirtualConnection& connection)
{
    std::lock_guard<std::recursive_mutex> lock(dispatcherListMutex);

    const auto it = dispatcherList.find(connection);
    if (it != dispatcherList.end()) {
        return it->second;
    }
    return std::shared_ptr<NotificationDispatcher>();
}

AmsConnection::AmsConnection(Router& __router, IpV4 __destIp)
    : router(__router),
    socket(__destIp, ADS_TCP_SERVER_PORT),
    refCount(0),
    invokeId(0),
    destIp(__destIp),
    ownIp(socket.Connect())
{
    receiver = std::thread(&AmsConnection::TryRecv, this);
}

AmsConnection::~AmsConnection()
{
    socket.Shutdown();
    receiver.join();
}

NotifyMapping AmsConnection::CreateNotifyMapping(uint32_t hNotify, std::shared_ptr<Notification> notification)
{
    const auto dispatcher = DispatcherListAdd(notification->connection);
    notification->hNotify(hNotify);
    dispatcher->Emplace(hNotify, notification);
    return NotifyMapping {hNotify, dispatcher};
}

long AmsConnection::DeleteNotification(const AmsAddr& amsAddr, uint32_t hNotify, uint32_t tmms, uint16_t port)
{
    AmsRequest request {
        amsAddr,
        port, AoEHeader::DEL_DEVICE_NOTIFICATION,
        0, nullptr, nullptr,
        sizeof(hNotify)
    };
    request.frame.prepend(qToLittleEndian(hNotify));

    return AdsRequest<AoEResponseHeader>(request, tmms);
}

AmsResponse* AmsConnection::Write(Frame& request, const AmsAddr destAddr, const AmsAddr srcAddr, uint16_t cmdId)
{
    AoEHeader aoeHeader { destAddr.netId, destAddr.port, srcAddr.netId, srcAddr.port, cmdId,
                          static_cast<uint32_t>(request.size()), GetInvokeId() };
    request.prepend<AoEHeader>(aoeHeader);

    AmsTcpHeader header { static_cast<uint32_t>(request.size()) };
    request.prepend<AmsTcpHeader>(header);

    auto response = Reserve(aoeHeader.invokeId(), srcAddr.port);

    if (!response) {
        return nullptr;
    }

    if (request.size() != socket.write(request)) {
        Release(response);
        return nullptr;
    }
    return response;
}

uint32_t AmsConnection::GetInvokeId()
{
    uint32_t result;
    do {
        result = invokeId.fetch_add(1);
    } while (!result);
    return result;
}

AmsResponse* AmsConnection::GetPending(uint32_t id, uint16_t port)
{
    const uint16_t portIndex = port - Router::PORT_BASE;
    if (portIndex >= Router::NUM_PORTS_MAX) {
        LOG_WARN("Port 0x" << std::hex << port << " is out of range");
        return nullptr;
    }

    const uint32_t currentId = queue[portIndex].invokeId;
    if (currentId == id) {
        return &queue[portIndex];
    }
    LOG_WARN("InvokeId mismatch: waiting for 0x" << std::hex << currentId << " received 0x" << id);
    return nullptr;
}

AmsResponse* AmsConnection::Reserve(uint32_t id, uint16_t port)
{
    uint32_t isFree = 0;
    if (!queue[port - Router::PORT_BASE].invokeId.compare_exchange_strong(isFree, id)) {
        LOG_WARN("Port: " << port << " already in use as " << isFree);
        return nullptr;
    }
    return &queue[port - Router::PORT_BASE];
}

void AmsConnection::Release(AmsResponse* response)
{
    response->frame.reset();
    response->invokeId = 0;
}

void AmsConnection::Receive(void* buffer, size_t bytesToRead) const
{
    auto pos = reinterpret_cast<uint8_t*>(buffer);
    while (bytesToRead) {
        const size_t bytesRead = socket.read(pos, bytesToRead, nullptr);
        bytesToRead -= bytesRead;
        pos += bytesRead;
    }
}

void AmsConnection::ReceiveJunk(size_t bytesToRead) const
{
    uint8_t buffer[1024];
    while (bytesToRead > sizeof(buffer)) {
        Receive(buffer, sizeof(buffer));
        bytesToRead -= sizeof(buffer);
    }
    Receive(buffer, bytesToRead);
}

Frame& AmsConnection::ReceiveFrame(Frame& frame, size_t bytesLeft) const
{
    if (bytesLeft > frame.capacity()) {
        LOG_WARN("Frame to long: " << std::dec << bytesLeft << '<' << frame.capacity());
        ReceiveJunk(bytesLeft);
        return frame.clear();
    }
    Receive(frame.rawData(), bytesLeft);
    return frame.limit(bytesLeft);
}

bool AmsConnection::ReceiveNotification(const AoEHeader& header)
{
    const auto dispatcher = DispatcherListGet(VirtualConnection { header.targetPort(), header.sourceAms() });
    if (!dispatcher) {
        ReceiveJunk(header.length());
        LOG_WARN("No dispatcher found for notification");
        return false;
    }

    auto& ring = dispatcher->ring;
    auto bytesLeft = header.length();
    if (bytesLeft + sizeof(bytesLeft) > ring.BytesFree()) {
        ReceiveJunk(bytesLeft);
        LOG_WARN("port " << std::dec << header.targetPort() << " receive buffer was full");
        return false;
    }

    /** store AoEHeader.length() in ring buffer to support notification parsing */
    for (size_t i = 0; i < sizeof(bytesLeft); ++i) {
        *ring.write = (bytesLeft >> (8 * i)) & 0xFF;
        ring.Write(1);
    }

    auto chunk = ring.WriteChunk();
    while (bytesLeft > chunk) {
        Receive(ring.write, chunk);
        ring.Write(chunk);
        bytesLeft -= chunk;
        chunk = ring.WriteChunk();
    }
    Receive(ring.write, bytesLeft);
    ring.Write(bytesLeft);
    dispatcher->Notify();
    return true;
}

void AmsConnection::TryRecv()
{
    try {
        Recv();
    } catch (const std::runtime_error& e) {
        LOG_INFO(e.what());
    }
}

void AmsConnection::Recv()
{
    AmsTcpHeader amsTcpHeader;
    AoEHeader aoeHeader;
    for ( ; ownIp; ) {
        Receive(amsTcpHeader);
        if (amsTcpHeader.length() < sizeof(aoeHeader)) {
            LOG_WARN("Frame to short to be AoE");
            ReceiveJunk(amsTcpHeader.length());
            continue;
        }

        Receive(aoeHeader);
        if (aoeHeader.cmdId() == AoEHeader::DEVICE_NOTIFICATION) {
            ReceiveNotification(aoeHeader);
            continue;
        }

        auto response = GetPending(aoeHeader.invokeId(), aoeHeader.targetPort());
        if (!response) {
            LOG_WARN("No response pending");
            ReceiveJunk(aoeHeader.length());
            continue;
        }

        ReceiveFrame(response->frame, aoeHeader.length());

        switch (aoeHeader.cmdId()) {
        case AoEHeader::READ_DEVICE_INFO:
        case AoEHeader::READ:
        case AoEHeader::WRITE:
        case AoEHeader::READ_STATE:
        case AoEHeader::WRITE_CONTROL:
        case AoEHeader::ADD_DEVICE_NOTIFICATION:
        case AoEHeader::DEL_DEVICE_NOTIFICATION:
        case AoEHeader::READ_WRITE:
            break;

        default:
            LOG_WARN("Unkown AMS command id");
            response->frame.clear();
        }

        response->errorCode = aoeHeader.errorCode();
        response->Notify();
    }
}
