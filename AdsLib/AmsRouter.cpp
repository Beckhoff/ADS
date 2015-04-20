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

long AmsRouter::AddNotification(AmsRequest& request, uint32_t* pNotification, Notification& notify)
{
    if (request.bytesRead) {
        *request.bytesRead = 0;
    }

    auto ads = GetConnection(request.destAddr.netId);
    if (!ads) {
        return GLOBALERR_MISSING_ROUTE;
    }

    auto& port = ports[request.port - Router::PORT_BASE];
    const long status = ads->AdsRequest<AoEResponseHeader>(request, port.tmms);
    if (!status) {
        *pNotification = qFromLittleEndian<uint32_t>((uint8_t*)request.buffer);
        const auto notifyId = ads->CreateNotifyMapping(*pNotification, notify);
        port.AddNotification(notifyId);
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
