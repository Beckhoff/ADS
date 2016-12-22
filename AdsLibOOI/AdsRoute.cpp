#include "AdsRoute.h"
#include "AdsException.h"
#include "AdsLib/AdsLib.h"

static void CloseLocalPort(const long* port)
{
    AdsPortCloseEx(*port);
    delete port;
}

static void DeleteRoute(AmsNetId* pNetId)
{
    AdsDelRoute(*pNetId);
    delete pNetId;
}

static AmsNetId* AddRoute(AmsNetId ams, const char* ip)
{
    const auto error = AdsAddRoute(ams, ip);
    if (error) {
        throw AdsException(error);
    }
    return new AmsNetId {ams};
}

AdsRoute::AdsRoute(const std::string& ipV4, AmsNetId netId, uint16_t port)
    : m_NetId(AddRoute(netId, ipV4.c_str()), DeleteRoute),
    m_Port({netId, port}),
    m_LocalPort(new long { AdsPortOpenEx() }, CloseLocalPort)
{}

long AdsRoute::GetLocalPort() const
{
    return *m_LocalPort;
}

void AdsRoute::SetTimeout(const uint32_t timeout) const
{
    const auto error = AdsSyncSetTimeoutEx(GetLocalPort(), timeout);
    if (error) {
        throw AdsException(error);
    }
}

uint32_t AdsRoute::GetTimeout() const
{
    uint32_t timeout = 0;
    const auto error = AdsSyncGetTimeoutEx(GetLocalPort(), &timeout);
    if (error) {
        throw AdsException(error);
    }
    return timeout;
}
