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

void AmsPort::AddNotification(NotifyMapping mapping)
{
    std::lock_guard<std::mutex> lock(mutex);
    notifications.insert(mapping);
}

void AmsPort::Close()
{
    std::lock_guard<std::mutex> lock(mutex);

    auto it = std::begin(notifications);
    while (it != std::end(notifications)) {
        it->second->Erase(it->first, tmms);
        it = notifications.erase(it);
    }
    tmms = DEFAULT_TIMEOUT;
    port = 0;
}

long AmsPort::DelNotification(const AmsAddr& ams, uint32_t hNotify)
{
    std::lock_guard<std::mutex> lock(mutex);
    for (auto it = notifications.begin(); it != notifications.end(); ++it) {
        if (it->first == hNotify) {
            if (std::ref(it->second->conn.second) == ams) {
                const auto status = it->second->Erase(hNotify, tmms);
                notifications.erase(it);
                return status;
            }
        }
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
