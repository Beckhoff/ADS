#pragma once
#include "AdsLib/AdsDef.h"
#include <memory>
#include <string>

class AdsRouteImpl {
public:
    AdsRouteImpl(const std::string& ipV4, AmsNetId netId, uint16_t taskPort, uint16_t symbolPort);
    ~AdsRouteImpl();

    const AmsNetId GetAmsNetId() const;
    const AmsAddr GetTaskAmsAddr() const;
    const AmsAddr GetSymbolsAmsAddr() const;
    const long GetLocalPort() const;
    void SetTimeout(const uint32_t timeout) const;
    const uint32_t GetTimeout() const;

private:
    static void CloseLocalPort(const long* port);

    const std::string m_Ip;
    const AmsNetId m_NetId;
    const uint16_t m_TaskPort;
    const uint16_t m_SymbolPort;
    std::unique_ptr<long, decltype(& CloseLocalPort)> m_LocalPort;
};

struct AdsRoute {
    // Shared route instance
    std::shared_ptr<AdsRouteImpl> route;
    AdsRoute(const std::string& ipV4, AmsNetId netId, uint16_t taskPort, uint16_t symbolPort);
    const AdsRouteImpl* operator->() const;

    // Timeout for ADS operations
    void SetTimeout(const uint32_t timeout) const;
    const uint32_t GetTimeout() const;
};
