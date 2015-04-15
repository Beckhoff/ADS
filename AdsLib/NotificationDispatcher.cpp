#include "NotificationDispatcher.h"
#include "Log.h"

NotificationDispatcher::NotificationDispatcher(AmsProxy& __proxy, AmsAddr __amsAddr, uint16_t __port)
    : amsAddr(__amsAddr),
    ring(4 * 1024 * 1024),
    port(__port),
    proxy(__proxy),
    thread(&NotificationDispatcher::Run, this)
{}

NotificationDispatcher::~NotificationDispatcher()
{
    sem.Close();
    thread.join();
}

bool NotificationDispatcher::operator<(const NotificationDispatcher& ref) const
{
    return amsAddr < ref.amsAddr;
}

void NotificationDispatcher::Emplace(PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t length, uint32_t hNotify)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    notifications.emplace(hNotify, Notification { pFunc, hNotify, hUser, length, amsAddr, port });
}

bool NotificationDispatcher::Erase(uint32_t hNotify, uint32_t tmms)
{
    const auto status = proxy.DeleteNotification(amsAddr, hNotify, tmms, port);
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return !!notifications.erase(hNotify) && status;
}

void NotificationDispatcher::Run()
{
    while (sem.Wait()) {
        const auto length = ring.ReadFromLittleEndian<uint32_t>();
        (void)length;
        const auto numStamps = ring.ReadFromLittleEndian<uint32_t>();
        for (uint32_t stamp = 0; stamp < numStamps; ++stamp) {
            const auto timestamp = ring.ReadFromLittleEndian<uint64_t>();
            const auto numSamples = ring.ReadFromLittleEndian<uint32_t>();
            for (uint32_t sample = 0; sample < numSamples; ++sample) {
                const auto hNotify = ring.ReadFromLittleEndian<uint32_t>();
                const auto size = ring.ReadFromLittleEndian<uint32_t>();
                std::lock_guard<std::recursive_mutex> lock(mutex);
                auto it = notifications.find(hNotify);
                if (it != notifications.end()) {
                    auto& notification = it->second;
                    if (size != notification.Size()) {
                        LOG_WARN("Notification sample size: " << size << " doesn't match: " << notification.Size());
                        ring.Read(size);
                        return;
                    }
                    notification.Notify(timestamp, ring);
                } else {
                    ring.Read(size);
                }
            }
        }
    }
}
