// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "AmsRouter.h"
#include "Log.h"

#include <algorithm>

AmsRouter::AmsRouter(AmsNetId netId)
    : localAddr(netId)
{}

long AmsRouter::AddRoute(AmsNetId ams, const IpV4& ip)
{
    /**
     * We keep this madness only for backwards compatibility, to give
     * downstream projects time to migrate to the much saner interface.
     */
    struct in_addr addr;
    static_assert(sizeof(addr) == sizeof(ip), "Oops sizeof(IpV4) doesn't match sizeof(in_addr)");
    memcpy(&addr, &ip.value, sizeof(addr));
    return AddRoute(ams, std::string(inet_ntoa(addr)));
}

long AmsRouter::AddRoute(AmsNetId ams, const std::string& host)
{
    /**
        DNS lookups are pretty time consuming, we shouldn't do them
        with a looked mutex! So instead we do the lookup first and
        use the results, later.
     */
    auto hostAddresses = bhf::ads::GetListOfAddresses(host, "48898");

    std::lock_guard<std::recursive_mutex> lock(mutex);
    const auto oldConnection = GetConnection(ams);
    if (oldConnection && !oldConnection->IsConnectedTo(hostAddresses.get())) {
        /**
           There is already a route for this AmsNetId, but with
           a different IP. The old route has to be deleted, first!
         */
        return ROUTERERR_PORTALREADYINUSE;
    }

    for (const auto& conn : connections) {
        if (conn->IsConnectedTo(hostAddresses.get())) {
            conn->refCount++;
            mapping[ams] = conn.get();
            return 0;
        }
    }

    auto conn = connections.emplace(std::unique_ptr<AmsConnection>(new AmsConnection { *this, hostAddresses.get()}));
    if (conn.second) {
        /** in case no local AmsNetId was set previously, we derive one */
        if (!localAddr) {
            localAddr = AmsNetId {conn.first->get()->ownIp};
        }
        conn.first->get()->refCount++;
        mapping[ams] = conn.first->get();
        return !conn.first->get()->ownIp;
    }
    return -1;
}

void AmsRouter::DelRoute(const AmsNetId& ams)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    auto route = mapping.find(ams);
    if (route != mapping.end()) {
        AmsConnection* conn = route->second;
        if (0 == --conn->refCount) {
            mapping.erase(route);
            DeleteIfLastConnection(conn);
        }
    }
}

void AmsRouter::DeleteIfLastConnection(const AmsConnection* const conn)
{
    if (conn) {
        for (const auto& r : mapping) {
            if (r.second == conn) {
                return;
            }
        }
        for (auto it = connections.begin(); it != connections.end(); ++it) {
            if (conn == it->get()) {
                connections.erase(it);
                return;
            }
        }
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

void AmsRouter::SetLocalAddress(AmsNetId netId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    localAddr = netId;
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
    const auto it = mapping.find(amsDest);
    if (it != mapping.end()) {
        return it->second;
    }
    return nullptr;
}

long AmsRouter::AdsRequest(AmsRequest& request)
{
    if (request.bytesRead) {
        *request.bytesRead = 0;
    }

    auto ads = GetConnection(request.destAddr.netId);
    if (!ads) {
        return GLOBALERR_MISSING_ROUTE;
    }
    return ads->AdsRequest(request, ports[request.port - Router::PORT_BASE].tmms);
}

long AmsRouter::AddNotification(AmsRequest& request, uint32_t* pNotification, std::shared_ptr<Notification> notify)
{
    if (request.bytesRead) {
        *request.bytesRead = 0;
    }

    auto ads = GetConnection(request.destAddr.netId);
    if (!ads) {
        return GLOBALERR_MISSING_ROUTE;
    }

    auto& port = ports[request.port - Router::PORT_BASE];
    const long status = ads->AdsRequest(request, port.tmms);
    if (!status) {
        *pNotification = bhf::ads::letoh<uint32_t>(request.buffer);
        auto dispatcher = ads->CreateNotifyMapping(*pNotification, notify);
        port.AddNotification(request.destAddr, *pNotification, dispatcher);
    }
    return status;
}

long AmsRouter::DelNotification(uint16_t port, const AmsAddr* pAddr, uint32_t hNotification)
{
    auto& p = ports[port - Router::PORT_BASE];
    return p.DelNotification(*pAddr, hNotification);
}
