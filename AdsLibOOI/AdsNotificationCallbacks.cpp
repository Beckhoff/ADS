#include "AdsNotificationCallbacks.h"
#include "AdsLib/AdsDef.h"

std::mutex AdsNotificationCallbacks::Mutex;

std::map<uint32_t, std::function<void()> >& AdsNotificationCallbacks::GetCallbacks()
{
    static std::map<uint32_t, std::function<void()> > callbacks;
    return callbacks;
}

void AdsNotificationCallbacks::AddCallback(uint32_t handle, std::function<void()> function)
{
    std::lock_guard<std::mutex> lock(Mutex);
    GetCallbacks()[handle] = function;
}

void AdsNotificationCallbacks::RemoveCallback(uint32_t handle)
{
    std::lock_guard<std::mutex> lock(Mutex);
    auto callbacks = GetCallbacks();
    if (callbacks.count(handle)) {
        callbacks.erase(handle);
    }
}

void AdsNotificationCallbacks::OnCallback(const AmsAddr*               pAddr,
                                          const AdsNotificationHeader* pNotification,
                                          uint32_t                     hUser)
{
    std::lock_guard<std::mutex> lock(Mutex);
    auto callbacks = GetCallbacks();
    if (callbacks.count(pNotification->hNotification)) {
        callbacks[pNotification->hNotification]();
    }
}
