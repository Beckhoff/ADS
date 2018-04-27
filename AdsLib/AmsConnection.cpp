/**
   Copyright (c) 2015 - 2018 Beckhoff Automation GmbH & Co. KG

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
    : request(nullptr),
    errorCode(WAITING_FOR_RESPONSE)
{}

void AmsResponse::Notify(const uint32_t error)
{
    std::unique_lock<std::mutex> lock(mutex);
    errorCode = error;
    cv.notify_all();
}

uint32_t AmsResponse::Wait()
{
    std::unique_lock<std::mutex> lock(mutex);

    cv.wait_until(lock, request.load()->deadline, [&]() { return !invokeId.load(); });

    if (invokeId.exchange(0)) {
        /* invokeId wasn't consumed -> AmsConnection::recv() didn't got a valid response until now */
        return ADSERR_CLIENT_SYNCTIMEOUT;
    }

    /* AmsConnection::recv() is currently processing a response and using the user supplied buffer, we need to wait until that finished */
    cv.wait(lock, [&]() { return errorCode != WAITING_FOR_RESPONSE; });
    return errorCode;
}

SharedDispatcher AmsConnection::DispatcherListAdd(const VirtualConnection& connection)
{
    const auto dispatcher = DispatcherListGet(connection);
    if (dispatcher) {
        return dispatcher;
    }
    std::lock_guard<std::recursive_mutex> lock(dispatcherListMutex);
    return dispatcherList.emplace(connection,
                                  std::make_shared<NotificationDispatcher>(std::bind(&AmsConnection::DeleteNotification,
                                                                                     this,
                                                                                     connection.second,
                                                                                     std::placeholders::_1,
                                                                                     std::placeholders::_2,
                                                                                     connection.first))).first->second;
}

SharedDispatcher AmsConnection::DispatcherListGet(const VirtualConnection& connection)
{
    std::lock_guard<std::recursive_mutex> lock(dispatcherListMutex);

    const auto it = dispatcherList.find(connection);
    if (it != dispatcherList.end()) {
        return it->second;
    }
    return {};
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

SharedDispatcher AmsConnection::CreateNotifyMapping(uint32_t hNotify, std::shared_ptr<Notification> notification)
{
    auto dispatcher = DispatcherListAdd(notification->connection);
    notification->hNotify(hNotify);
    dispatcher->Emplace(hNotify, notification);
    return dispatcher;
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
    return AdsRequest(request, tmms);
}

AmsResponse* AmsConnection::Write(AmsRequest& request, const AmsAddr srcAddr)
{
    const AoEHeader aoeHeader {
        request.destAddr.netId, request.destAddr.port,
        srcAddr.netId, srcAddr.port,
        request.cmdId,
        static_cast<uint32_t>(request.frame.size()),
        GetInvokeId()
    };
    request.frame.prepend<AoEHeader>(aoeHeader);

    const AmsTcpHeader header { static_cast<uint32_t>(request.frame.size()) };
    request.frame.prepend<AmsTcpHeader>(header);

    auto response = Reserve(&request, srcAddr.port);

    if (!response) {
        return nullptr;
    }

    response->invokeId.store(aoeHeader.invokeId());
    if (request.frame.size() != socket.write(request.frame)) {
        response->Release();
        return nullptr;
    }
    return response;
}

long AmsConnection::AdsRequest(AmsRequest& request, const uint32_t timeout)
{
    AmsAddr srcAddr;
    const auto status = router.GetLocalAddress(request.port, &srcAddr);
    if (status) {
        return status;
    }
    request.SetDeadline(timeout);
    AmsResponse* response = Write(request, srcAddr);
    if (response) {
        const auto errorCode = response->Wait();

        response->Release();
        return errorCode;
    }
    return -1;
}

uint32_t AmsConnection::GetInvokeId()
{
    uint32_t result;
    do {
        result = invokeId.fetch_add(1);
    } while (!result);
    return result;
}

AmsResponse* AmsConnection::GetPending(const uint32_t id, const uint16_t port)
{
    const uint16_t portIndex = port - Router::PORT_BASE;
    if (portIndex >= Router::NUM_PORTS_MAX) {
        LOG_WARN("Port 0x" << std::hex << port << " is out of range");
        return nullptr;
    }

    auto currentId = id;
    if (queue[portIndex].invokeId.compare_exchange_strong(currentId, 0)) {
        return &queue[portIndex];
    }
    LOG_WARN("InvokeId mismatch: waiting for 0x" << std::hex << currentId << " received 0x" << id);
    return nullptr;
}

AmsResponse* AmsConnection::Reserve(AmsRequest* request, const uint16_t port)
{
    AmsRequest* isFree = nullptr;
    if (!queue[port - Router::PORT_BASE].request.compare_exchange_strong(isFree, request)) {
        LOG_WARN("Port: " << port << " already in use as " << isFree);
        return nullptr;
    }
    return &queue[port - Router::PORT_BASE];
}

void AmsResponse::Release()
{
    errorCode = WAITING_FOR_RESPONSE;
    request.store(nullptr);
}

void AmsConnection::Receive(void* buffer, size_t bytesToRead, timeval* timeout) const
{
    auto pos = reinterpret_cast<uint8_t*>(buffer);
    while (bytesToRead) {
        const size_t bytesRead = socket.read(pos, bytesToRead, timeout);
        bytesToRead -= bytesRead;
        pos += bytesRead;
    }
}

void AmsConnection::Receive(void* buffer, size_t bytesToRead, const Timepoint& deadline) const
{
    const auto now = std::chrono::steady_clock::now();
    const auto usec = std::chrono::duration_cast<std::chrono::microseconds>(deadline - now).count();
    if (usec <= 0) {
        throw Socket::TimeoutEx("deadline reached already!!!");
    }

    timeval timeout {(long)(usec / 1000000), (int)(usec % 1000000)};
    Receive(buffer, bytesToRead, &timeout);
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

template<class T>
void AmsConnection::ReceiveFrame(AmsResponse* const response, size_t bytesLeft, uint32_t aoeError) const
{
    AmsRequest* const request = response->request.load();
    const auto responseId = response->invokeId.load();
    T header;

    if (bytesLeft > sizeof(header) + request->bufferLength) {
        LOG_WARN("Frame to long: " << std::dec << bytesLeft << '<' << sizeof(header) + request->bufferLength);
        response->Notify(ADSERR_DEVICE_INVALIDSIZE);
        ReceiveJunk(bytesLeft);
        return;
    }

    try {
        Receive(&header, sizeof(header), request->deadline);
        bytesLeft -= sizeof(header);
        Receive(request->buffer, bytesLeft, request->deadline);

        if (request->bytesRead) {
            *(request->bytesRead) = bytesLeft;
        }
        response->Notify(aoeError ? aoeError : header.result());
    } catch (const Socket::TimeoutEx&) {
        LOG_WARN("InvokeId of response: " << std::dec << responseId << " timed out");
        response->Notify(ADSERR_CLIENT_SYNCTIMEOUT);
        ReceiveJunk(bytesLeft);
    }
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

        switch (aoeHeader.cmdId()) {
        case AoEHeader::READ_DEVICE_INFO:
        case AoEHeader::WRITE:
        case AoEHeader::READ_STATE:
        case AoEHeader::WRITE_CONTROL:
        case AoEHeader::ADD_DEVICE_NOTIFICATION:
        case AoEHeader::DEL_DEVICE_NOTIFICATION:
            ReceiveFrame<AoEResponseHeader>(response, aoeHeader.length(), aoeHeader.errorCode());
            continue;

        case AoEHeader::READ:
        case AoEHeader::READ_WRITE:
            ReceiveFrame<AoEReadResponseHeader>(response, aoeHeader.length(), aoeHeader.errorCode());
            continue;

        default:
            LOG_WARN("Unkown AMS command id");
            response->Notify(ADSERR_CLIENT_SYNCRESINVALID);
            ReceiveJunk(aoeHeader.length());
        }
    }
}
