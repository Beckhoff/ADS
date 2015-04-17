#include "AmsRouter.h"
#include "Log.h"

#include <algorithm>

AmsRouter::AmsRouter(AmsNetId netId)
    : localAddr(netId)
{}

long AmsRouter::AddRoute(AmsNetId ams, const IpV4& ip)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    const auto oldConnection = GetConnection(ams);
    auto conn = connections.find(ip);
    if (conn == connections.end()) {
        conn = connections.emplace(ip, std::unique_ptr<AmsConnection>(new AmsConnection { *this, ip })).first;
    }

    mapping[ams] = conn->second.get();
    DeleteIfLastConnection(oldConnection);
    return 0;
}

void AmsRouter::DelRoute(const AmsNetId& ams)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    auto route = mapping.find(ams);
    if (route != mapping.end()) {
        const AmsConnection* conn = route->second;
        mapping.erase(route);
        DeleteIfLastConnection(conn);
    }
}

void AmsRouter::SetNetId(AmsNetId ams)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    localAddr = ams;
}

void AmsRouter::DeleteIfLastConnection(const AmsConnection* conn)
{
    if (conn) {
        for (const auto& r : mapping) {
            if (r.second == conn) {
                return;
            }
        }
        connections.erase(conn->destIp);
    }
}

uint16_t AmsRouter::OpenPort()
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    for (uint16_t i = 0; i < NUM_PORTS_MAX; ++i) {
        if (!ports[i].IsOpen()) {
            return ports[i].Open(PORT_BASE + i);
        }
    }
    return 0;
}

long AmsRouter::ClosePort(uint16_t port)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if ((port < PORT_BASE) || (port >= PORT_BASE + NUM_PORTS_MAX) || !ports[port - PORT_BASE].IsOpen()) {
        return ADSERR_CLIENT_PORTNOTOPEN;
    }
    ports[port - PORT_BASE].Close();
    return 0;
}

long AmsRouter::GetLocalAddress(uint16_t port, AmsAddr* pAddr)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if ((port < PORT_BASE) || (port >= PORT_BASE + NUM_PORTS_MAX)) {
        return ADSERR_CLIENT_PORTNOTOPEN;
    }

    if (ports[port - PORT_BASE].IsOpen()) {
        memcpy(&pAddr->netId, &localAddr, sizeof(localAddr));
        pAddr->port = port;
        return 0;
    }
    return ADSERR_CLIENT_PORTNOTOPEN;
}

long AmsRouter::GetTimeout(uint16_t port, uint32_t& timeout)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if ((port < PORT_BASE) || (port >= PORT_BASE + NUM_PORTS_MAX)) {
        return ADSERR_CLIENT_PORTNOTOPEN;
    }

    timeout = ports[port - PORT_BASE].tmms;
    return 0;
}

long AmsRouter::SetTimeout(uint16_t port, uint32_t timeout)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if ((port < PORT_BASE) || (port >= PORT_BASE + NUM_PORTS_MAX)) {
        return ADSERR_CLIENT_PORTNOTOPEN;
    }

    ports[port - PORT_BASE].tmms = timeout;
    return 0;
}

AmsConnection* AmsRouter::GetConnection(const AmsNetId& amsDest)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    const auto it = __GetConnection(amsDest);
    if (it == connections.end()) {
        return nullptr;
    }
    return it->second.get();
}

std::map<IpV4, std::unique_ptr<AmsConnection> >::iterator AmsRouter::__GetConnection(const AmsNetId& amsDest)
{
    const auto it = mapping.find(amsDest);
    if (it != mapping.end()) {
        return connections.find(it->second->destIp);
    }
    return connections.end();
}

long AmsRouter::Read(uint16_t       port,
                     const AmsAddr* pAddr,
                     uint32_t       indexGroup,
                     uint32_t       indexOffset,
                     uint32_t       bufferLength,
                     void*          buffer,
                     uint32_t*      bytesRead)
{
    Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AoERequestHeader));
    request.prepend(AoERequestHeader {
        indexGroup,
        indexOffset,
        bufferLength
    });
    return AdsRequest<AoEReadResponseHeader>(request, *pAddr, port, AoEHeader::READ, bufferLength, buffer, bytesRead);
}

long AmsRouter::ReadDeviceInfo(uint16_t port, const AmsAddr* pAddr, char* devName, AdsVersion* version)
{
    static const size_t NAME_LENGTH = 16;
    Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader));
    uint8_t buffer[sizeof(*version) + NAME_LENGTH];

    const auto status = AdsRequest<AoEResponseHeader>(request,
                                                      *pAddr,
                                                      port,
                                                      AoEHeader::READ_DEVICE_INFO,
                                                      sizeof(buffer),
                                                      buffer);
    if (!status) {
        version->version = buffer[0];
        version->revision = buffer[1];
        version->build = qFromLittleEndian<uint16_t>(buffer + offsetof(AdsVersion, build));
        memcpy(devName, buffer + sizeof(*version), NAME_LENGTH);
    }
    return status;
}

long AmsRouter::ReadState(uint16_t port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* devState)
{
    Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader));
    uint8_t buffer[sizeof(*adsState) + sizeof(*devState)];

    const auto status = AdsRequest<AoEResponseHeader>(request,
                                                      *pAddr,
                                                      port,
                                                      AoEHeader::READ_STATE,
                                                      sizeof(buffer),
                                                      buffer);
    if (!status) {
        *adsState = qFromLittleEndian<uint16_t>(buffer);
        *devState = qFromLittleEndian<uint16_t>(buffer + sizeof(*adsState));
    }
    return status;
}

long AmsRouter::ReadWrite(uint16_t       port,
                          const AmsAddr* pAddr,
                          uint32_t       indexGroup,
                          uint32_t       indexOffset,
                          uint32_t       readLength,
                          void*          readData,
                          uint32_t       writeLength,
                          const void*    writeData,
                          uint32_t*      bytesRead)
{
    Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AoEReadWriteReqHeader) + writeLength);
    request.prepend(writeData, writeLength);
    request.prepend(AoEReadWriteReqHeader {
        indexGroup,
        indexOffset,
        readLength,
        writeLength
    });
    return AdsRequest<AoEReadResponseHeader>(request,
                                             *pAddr,
                                             port,
                                             AoEHeader::READ_WRITE,
                                             readLength,
                                             readData,
                                             bytesRead);
}

template<class T>
long AmsRouter::AdsRequest(Frame&         request,
                           const AmsAddr& destAddr,
                           uint16_t       port,
                           uint16_t       cmdId,
                           uint32_t       bufferLength,
                           void*          buffer,
                           uint32_t*      bytesRead)
{
    if (bytesRead) {
        *bytesRead = 0;
    }

    auto ads = GetConnection(destAddr.netId);
    if (!ads) {
        return GLOBALERR_MISSING_ROUTE;
    }
    return ads->AdsRequest<T>(request,
                              destAddr,
                              ports[port - Router::PORT_BASE].tmms,
                              port,
                              cmdId,
                              bufferLength,
                              buffer,
                              bytesRead);
}

long AmsRouter::Write(uint16_t       port,
                      const AmsAddr* pAddr,
                      uint32_t       indexGroup,
                      uint32_t       indexOffset,
                      uint32_t       bufferLength,
                      const void*    buffer)
{
    Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AoERequestHeader) + bufferLength);
    request.prepend(buffer, bufferLength);
    request.prepend<AoERequestHeader>({
        indexGroup,
        indexOffset,
        bufferLength
    });
    return AdsRequest<AoEResponseHeader>(request, *pAddr, port, AoEHeader::WRITE);
}

long AmsRouter::WriteControl(uint16_t       port,
                             const AmsAddr* pAddr,
                             uint16_t       adsState,
                             uint16_t       devState,
                             uint32_t       bufferLength,
                             const void*    buffer)
{
    Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AdsWriteCtrlRequest) + bufferLength);
    request.prepend(buffer, bufferLength);
    request.prepend<AdsWriteCtrlRequest>({
        adsState,
        devState,
        bufferLength
    });
    return AdsRequest<AoEResponseHeader>(request, *pAddr, port, AoEHeader::WRITE_CONTROL);
}

long AmsRouter::AddNotification(uint16_t                     port,
                                const AmsAddr*               pAddr,
                                uint32_t                     indexGroup,
                                uint32_t                     indexOffset,
                                const AdsNotificationAttrib* pAttrib,
                                PAdsNotificationFuncEx       pFunc,
                                uint32_t                     hUser,
                                uint32_t*                    pNotification)
{
    Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AdsAddDeviceNotificationRequest));
    request.prepend(AdsAddDeviceNotificationRequest {
        indexGroup,
        indexOffset,
        pAttrib->cbLength,
        pAttrib->nTransMode,
        pAttrib->nMaxDelay,
        pAttrib->nCycleTime
    });

    uint8_t buffer[sizeof(*pNotification)];
    const long status =
        AdsRequest<AoEResponseHeader>(request, *pAddr, port, AoEHeader::ADD_DEVICE_NOTIFICATION, sizeof(buffer),
                                      buffer);
    if (!status) {
        *pNotification = qFromLittleEndian<uint32_t>(buffer);
        AmsConnection& conn = *GetConnection(pAddr->netId);
        const auto hash = conn.CreateNotifyMapping(Notification {pFunc, *pNotification, hUser, pAttrib->cbLength,
                                                                 *pAddr, port});
        ports[port - Router::PORT_BASE].AddNotification(hash);
    }
    return status;
}

long AmsRouter::DelNotification(uint16_t port, const AmsAddr* pAddr, uint32_t hNotification)
{
    auto& p = ports[port - Router::PORT_BASE];
    return p.DelNotification(*pAddr, hNotification) ? ADSERR_NOERR : ADSERR_CLIENT_REMOVEHASH;
}

template<class T> T extractLittleEndian(Frame& frame)
{
    const auto value = qFromLittleEndian<T>(frame.data());
    frame.remove(sizeof(T));
    return value;
}
