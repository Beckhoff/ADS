#pragma once
#include "AdsLib/AdsDef.h"
#include <memory>

struct AdsRoute {
    AdsRoute(const std::string& ipV4, AmsNetId netId, uint16_t taskPort, uint16_t symbolPort);

    AmsAddr GetTaskAmsAddr() const;
    AmsAddr GetSymbolsAmsAddr() const;
    long GetLocalPort() const;
    void SetTimeout(const uint32_t timeout) const;
    uint32_t GetTimeout() const;

    const std::shared_ptr<const AmsNetId> m_NetId;
private:
    const uint16_t m_TaskPort;
    const uint16_t m_SymbolPort;
    std::shared_ptr<long> m_LocalPort;
};
