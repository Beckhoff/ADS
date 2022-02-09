// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "AmsPort.h"

namespace std
{
bool operator==(const AmsAddr& lhs, const AmsAddr& rhs)
{
    return 0 == memcmp(&lhs, &rhs, sizeof(lhs));
}
}

AmsPort::AmsPort()
    : tmms(DEFAULT_TIMEOUT),
    port(0)
{}

void AmsPort::AddNotification(const AmsAddr ams, const uint32_t hNotify, SharedDispatcher dispatcher)
{
    std::lock_guard<std::mutex> lock(mutex);
    dispatcherList.emplace(NotifyUUID {ams, hNotify}, dispatcher);
}

void AmsPort::Close()
{
    std::lock_guard<std::mutex> lock(mutex);

    for (auto& d: dispatcherList) {
        d.second->Erase(d.first.second, tmms);
    }
    dispatcherList.clear();
    tmms = DEFAULT_TIMEOUT;
    port = 0;
}

long AmsPort::DelNotification(const AmsAddr ams, uint32_t hNotify)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = dispatcherList.find({ams, hNotify});
    if (it != dispatcherList.end()) {
        const auto status = it->second->Erase(hNotify, tmms);
        dispatcherList.erase(it);
        return status;
    }
    return ADSERR_CLIENT_REMOVEHASH;
}

bool AmsPort::IsOpen() const
{
    return !!port;
}

uint16_t AmsPort::Open(uint16_t __port)
{
    port = __port;
    return port;
}
