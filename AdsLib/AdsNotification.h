/**
   Copyright (c) 2015 Beckhoff Automation GmbH & Co. KG

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
 */

#ifndef _ADS_NOTIFICATION_H_
#define _ADS_NOTIFICATION_H_

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

#endif /* #ifndef _ADS_NOTIFICATION_H_ */
