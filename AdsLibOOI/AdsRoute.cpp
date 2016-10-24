#include "AdsRoute.h"
#include "AdsException.h"
#include "AdsLib/AdsLib.h"

AdsRouteImpl::AdsRouteImpl(const std::string& ipV4, AmsNetId netId, uint16_t taskPort, uint16_t symbolPort)
    : m_Ip(ipV4), m_NetId(netId), m_TaskPort(taskPort), m_SymbolPort(symbolPort), m_LocalPort(
        new long { AdsPortOpenEx() }, CloseLocalPort)
{
    AdsAddRoute(netId, ipV4.c_str());
}

void AdsRouteImpl::CloseLocalPort(const long* port)
{
    AdsPortCloseEx(*port);
    delete port;
}

const AmsNetId AdsRouteImpl::GetAmsNetId() const
{
    return m_NetId;
}

const AmsAddr AdsRouteImpl::GetTaskAmsAddr() const
{
    return { m_NetId, m_TaskPort };
}

const AmsAddr AdsRouteImpl::GetSymbolsAmsAddr() const
{
    return { m_NetId, m_SymbolPort };
}

AdsRouteImpl::~AdsRouteImpl()
{
    AdsDelRoute(m_NetId);
}

const long AdsRouteImpl::GetLocalPort() const
{
    return *m_LocalPort;
}

void AdsRouteImpl::SetTimeout(const uint32_t timeout) const
{
    auto error = AdsSyncSetTimeoutEx(GetLocalPort(), timeout);
    if (error) {
        throw AdsException(error);
    }
}

const uint32_t AdsRouteImpl::GetTimeout() const
{
    uint32_t timeout = 0;
    auto error = AdsSyncGetTimeoutEx(GetLocalPort(), &timeout);
    if (error) {
        throw AdsException(error);
    }
    return timeout;
}

AdsRoute::AdsRoute(const std::string& ipV4, AmsNetId netId, uint16_t taskPort, uint16_t symbolPort)
    : route(std::make_shared<AdsRouteImpl>(ipV4, netId, taskPort, symbolPort)) {}

const AdsRouteImpl* AdsRoute::operator->() const
{
    return route.get();
}

void AdsRoute::SetTimeout(const uint32_t timeout) const
{
    route->SetTimeout(timeout);
}

const uint32_t AdsRoute::GetTimeout() const
{
    return route->GetTimeout();
}
