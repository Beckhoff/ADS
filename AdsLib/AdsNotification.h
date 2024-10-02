// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "AdsDef.h"
#include "RingBuffer.h"

#include <vector>

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
        buffer(sizeof(AdsNotificationHeader) + length),
        hUser(__hUser),
        size(length)
    {
        auto header = reinterpret_cast<AdsNotificationHeader*>(buffer.data());
        header->hNotification = 0;
        header->cbSampleSize = length;
    }

    void Notify(RingBuffer& ring, const uint64_t timestamp, const uint32_t sampleSize)
    {
        auto header = reinterpret_cast<AdsNotificationHeader*>(buffer.data());
        uint8_t* data = reinterpret_cast<uint8_t*>(header + 1);
        header->cbSampleSize = sampleSize;
        header->nTimeStamp = timestamp;
        for (size_t i = 0; i < sampleSize; ++i) {
            data[i] = ring.ReadFromLittleEndian<uint8_t>();
        }
        callback(&connection.second, header, hUser);
    }

    uint32_t Size() const
    {
        return size;
    }

    void hNotify(uint32_t value)
    {
        auto header = reinterpret_cast<AdsNotificationHeader*>(buffer.data());
        header->hNotification = value;
    }

private:
    const PAdsNotificationFuncEx callback;
    std::vector<uint8_t> buffer;
    const uint32_t hUser;
    const uint32_t size;
};
