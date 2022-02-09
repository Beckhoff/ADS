// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "NotificationDispatcher.h"
#include "Log.h"
#include <future>

NotificationDispatcher::NotificationDispatcher(DeleteNotificationCallback callback)
    : deleteNotification(callback)
    , ring(4 * 1024 * 1024)
    , stopExecution(false)
    , thread(&NotificationDispatcher::Run, this)
{}

NotificationDispatcher::~NotificationDispatcher()
{
    stopExecution = true;
    sem.release();
    thread.join();
}

void NotificationDispatcher::Emplace(uint32_t hNotify, std::shared_ptr<Notification> notification)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    notifications.emplace(hNotify, notification);
}

long NotificationDispatcher::Erase(uint32_t hNotify, uint32_t tmms)
{
    const auto status = deleteNotification(hNotify, tmms);
    std::lock_guard<std::recursive_mutex> lock(mutex);
    notifications.erase(hNotify);
    return status;
}

std::shared_ptr<Notification> NotificationDispatcher::Find(uint32_t hNotify)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto it = notifications.find(hNotify);
    if (it != notifications.end()) {
        return it->second;
    }
    return {};
}

void NotificationDispatcher::Notify()
{
    sem.release();
}

void NotificationDispatcher::Run()
{
    for ( ; ; ) {
        sem.acquire();
        if (stopExecution) {
            return;
        }
        auto fullLength = ring.ReadFromLittleEndian<uint32_t>();
        const auto length = ring.ReadFromLittleEndian<uint32_t>();
        (void)length;
        const auto numStamps = ring.ReadFromLittleEndian<uint32_t>();
        fullLength -= sizeof(length) + sizeof(numStamps);
        for (uint32_t stamp = 0; stamp < numStamps; ++stamp) {
            const auto timestamp = ring.ReadFromLittleEndian<uint64_t>();
            const auto numSamples = ring.ReadFromLittleEndian<uint32_t>();
            fullLength -= sizeof(timestamp) + sizeof(numSamples);
            for (uint32_t sample = 0; sample < numSamples; ++sample) {
                const auto hNotify = ring.ReadFromLittleEndian<uint32_t>();
                const auto size = ring.ReadFromLittleEndian<uint32_t>();
                fullLength -= sizeof(hNotify) + sizeof(size);
                const auto notification = Find(hNotify);
                if (notification) {
                    if (size != notification->Size()) {
                        LOG_WARN("Notification sample size: " << size << " doesn't match: " << notification->Size());
                        goto cleanup;
                    }
                    notification->Notify(timestamp, ring);
                } else {
                    ring.Read(size);
                }
                fullLength -= size;
            }
        }
cleanup:
        ring.Read(fullLength);
    }
}
