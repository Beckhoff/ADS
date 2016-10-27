#pragma once
#include "AdsLib/AdsDef.h"
#include <memory>

struct AdsRoute {
    AdsRoute(const std::string& ipV4, AmsNetId netId, uint16_t taskPort, uint16_t symbolPort);

    long GetLocalPort() const;
    void SetTimeout(const uint32_t timeout) const;
    uint32_t GetTimeout() const;

    const std::shared_ptr<const AmsNetId> m_NetId;
    const AmsAddr m_TaskPort;
    const AmsAddr m_SymbolPort;
private:
    std::shared_ptr<long> m_LocalPort;
};
