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

#include "AdsNotification.h"
#include "AmsHeader.h"
#include "Semaphore.h"

#include <atomic>
#include <functional>
#include <map>
#include <thread>

using DeleteNotificationCallback = std::function<long (uint32_t hNotify, uint32_t tmms)>;

struct NotificationDispatcher {
    NotificationDispatcher(DeleteNotificationCallback callback);
    ~NotificationDispatcher();
    void Emplace(uint32_t hNotify, std::shared_ptr<Notification> notification);
    long Erase(uint32_t hNotify, uint32_t tmms);
    void Notify();
    void Run();

    const DeleteNotificationCallback deleteNotification;
    RingBuffer ring;
private:
    std::map<uint32_t, std::shared_ptr<Notification> > notifications;
    std::recursive_mutex mutex;
    Semaphore sem;
    std::atomic<bool> stopExecution;
    std::thread thread;

    std::shared_ptr<Notification> Find(uint32_t hNotify);
};
using SharedDispatcher = std::shared_ptr<NotificationDispatcher>;
