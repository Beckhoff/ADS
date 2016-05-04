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

#ifndef _AMS_PORT_H_
#define _AMS_PORT_H_

#include "NotificationDispatcher.h"

#include <set>

using NotifyMapping = std::pair<uint32_t, std::shared_ptr<NotificationDispatcher> >;

struct AmsPort {
    AmsPort();
    void Close();
    bool IsOpen() const;
    uint16_t Open(uint16_t __port);
    uint32_t tmms;
    uint16_t port;

    void AddNotification(NotifyMapping mapping);
    long DelNotification(const AmsAddr& ams, uint32_t hNotify);

private:
    static const uint32_t DEFAULT_TIMEOUT = 5000;
    std::set<NotifyMapping> notifications;
    std::mutex mutex;
};
#endif /* #ifndef _AMS_PORT_H_ */
