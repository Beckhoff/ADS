#include "AdsNotification.h"

AdsNotification::AdsNotification(const AdsRoute&              route,
                                 const uint32_t               indexGroup,
                                 const uint32_t               indexOffset,
                                 const AdsNotificationAttrib& NotificationAttributes,
                                 PAdsNotificationFuncEx       callback) :
    m_Route(route), m_Handle(RegisterNotification(route, indexGroup, indexOffset, NotificationAttributes, callback))
{}

AdsNotification::AdsNotificationHandleGuard AdsNotification::RegisterNotification(const AdsRoute&              route,
                                                                                  uint32_t                     indexGroup,
                                                                                  uint32_t                     indexOffset,
                                                                                  const AdsNotificationAttrib& notificationAttributes,
                                                                                  PAdsNotificationFuncEx       callback)
{
    auto amsAddr = route->GetSymbolsAmsAddr();
    uint32_t handle = 0;
    auto error = AdsSyncAddDeviceNotificationReqEx(
        route->GetLocalPort(), &amsAddr, indexGroup, indexOffset, &notificationAttributes, callback, indexOffset,
        &handle);
    if (error || !handle) {
        throw AdsException(error);
    }
    return { new uint32_t { handle }, NotificationDeleter { amsAddr, route->GetLocalPort() } };
}
