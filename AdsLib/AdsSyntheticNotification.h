// SPDX-License-Identifier: MIT

#pragma once

#include "AdsDef.h"

using VirtualConnection = std::pair<uint16_t, AmsAddr>;

struct SyntheticNotification
{
    const VirtualConnection connection;
    const uint32_t type;

    SyntheticNotification(PAdsSyntheticNotificationFuncEx __func,
                          uint32_t                        __hUser,
                          AmsAddr                         __amsAddr,
                          uint16_t                        __port,
                          uint32_t                        __type)
        : connection({ __port, __amsAddr }),
        type(__type),
        callback(__func),
        hNotification(0),
        hUser(__hUser)
    {
    }

    void Notify()
    {
        callback(&connection.second, hNotification, hUser);
    }

    void hNotify(uint32_t value)
    {
        hNotification = value;
    }

private:
    const PAdsSyntheticNotificationFuncEx callback;
    uint32_t hNotification;
    const uint32_t hUser;
};
