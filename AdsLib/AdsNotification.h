// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "AdsDef.h"
#include "RingBuffer.h"

#include <utility>

using VirtualConnection = std::pair<uint16_t, AmsAddr>;

struct Notification {
    const VirtualConnection connection;

    Notification(PAdsNotificationFuncEx __func,
                 uint32_t               __hUser,
                 uint32_t               length,
                 AmsAddr                __amsAddr,
                 uint16_t               __port)
        : connection({__port, __amsAddr}),
        callback(__func),
        buffer(new uint8_t[sizeof(AdsNotificationHeader) + length]),
        hUser(__hUser)
    {
        auto header = reinterpret_cast<AdsNotificationHeader*>(buffer.get());
        header->hNotification = 0;
        header->cbSampleSize = length;
    }

    void Notify(uint64_t timestamp, RingBuffer& ring) const
    {
        auto header = reinterpret_cast<AdsNotificationHeader*>(buffer.get());
        uint8_t* data = reinterpret_cast<uint8_t*>(header + 1);
        for (size_t i = 0; i < header->cbSampleSize; ++i) {
            data[i] = ring.ReadFromLittleEndian<uint8_t>();
        }
        header->nTimeStamp = timestamp;
        callback(&connection.second, header, hUser);
    }

    uint32_t Size() const
    {
        auto header = reinterpret_cast<AdsNotificationHeader*>(buffer.get());
        return header->cbSampleSize;
    }

    void hNotify(uint32_t value)
    {
        auto header = reinterpret_cast<AdsNotificationHeader*>(buffer.get());
        header->hNotification = value;
    }

private:
    const PAdsNotificationFuncEx callback;
    const std::shared_ptr<uint8_t> buffer;
    const uint32_t hUser;
};
