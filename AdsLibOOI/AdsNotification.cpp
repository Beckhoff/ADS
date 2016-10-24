#include "AdsNotification.h"

AdsNotification::AdsNotification(const AdsRoute&              route,
                                 const uint32_t               indexGroup,
                                 const uint32_t               indexOffset,
                                 const AdsNotificationAttrib& NotificationAttributes,
                                 std::function<void()>        callback) :
    m_Route(route), m_Handle(RegisterNotification(route, indexGroup, indexOffset, NotificationAttributes, callback))
{}

AdsNotification::AdsNotificationHandleGuard AdsNotification::RegisterNotification(const AdsRoute&              route,
                                                                                  uint32_t                     indexGroup,
                                                                                  uint32_t                     indexOffset,
                                                                                  const AdsNotificationAttrib& notificationAttributes,
                                                                                  std::function<void()>        callback)
{
    auto amsAddr = route->GetSymbolsAmsAddr();
    uint32_t handle = 0;
    auto error = AdsSyncAddDeviceNotificationReqEx(
        route->GetLocalPort(), &amsAddr, indexGroup, indexOffset, &notificationAttributes, &AdsNotificationCallbacks::OnCallback, indexOffset,
        &handle);
    if (error || !handle) {
        throw AdsException(error);
    }
    AdsNotificationCallbacks::AddCallback(handle, callback);
    return { new uint32_t { handle }, NotificationDeleter { amsAddr, route->GetLocalPort() } };
}
