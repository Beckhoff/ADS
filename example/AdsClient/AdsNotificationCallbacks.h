#pragma once
#include <mutex>
#include <map>
#include <functional>

struct AdsNotificationHeader;
struct AmsAddr;

namespace AdsNotificationCallbacks
{
// Static functions to manage the callbacks for AdsNotification objects
std::map<uint32_t, std::function<void()> >& GetCallbacks();
void AddCallback(uint32_t handle, std::function<void()> function);
void RemoveCallback(uint32_t handle);
void OnCallback(const AmsAddr* pAddr, const AdsNotificationHeader* pNotification, uint32_t hUser);
extern std::mutex Mutex;
}
