/**
   Copyright (c) 2015 - 2018 Beckhoff Automation GmbH & Co. KG

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
