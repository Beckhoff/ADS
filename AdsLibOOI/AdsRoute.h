#pragma once
#include "AdsLib/AdsDef.h"
#include <memory>

struct AdsRouteImpl {
    AdsRouteImpl(const std::string& ipV4, AmsNetId netId, uint16_t taskPort, uint16_t symbolPort);

    AmsAddr GetTaskAmsAddr() const;
    AmsAddr GetSymbolsAmsAddr() const;
    long GetLocalPort() const;
    void SetTimeout(const uint32_t timeout) const;
    uint32_t GetTimeout() const;

    const AmsNetId m_NetId;
private:
    const uint16_t m_TaskPort;
    const uint16_t m_SymbolPort;
    std::shared_ptr<long> m_LocalPort;
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
