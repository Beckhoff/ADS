// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "NotificationDispatcher.h"

struct AmsPort {
    AmsPort();
    void Close();
    bool IsOpen() const;
    uint16_t Open(uint16_t __port);
    uint32_t tmms;
    uint16_t port;

    void AddNotification(AmsAddr ams, uint32_t hNotify, SharedDispatcher dispatcher);
    long DelNotification(AmsAddr ams, uint32_t hNotify);

private:
    using NotifyUUID = std::pair<const AmsAddr, const uint32_t>;
    static const uint32_t DEFAULT_TIMEOUT = 5000;
    std::map<NotifyUUID, SharedDispatcher> dispatcherList;
    std::mutex mutex;
};
