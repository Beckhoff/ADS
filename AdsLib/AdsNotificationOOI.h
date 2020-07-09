#pragma once

#include "AdsDevice.h"

struct AdsNotification {
    AdsNotification(const AdsDevice&             route,
                    const std::string&           symbolName,
                    const AdsNotificationAttrib& notificationAttributes,
                    PAdsNotificationFuncEx       callback,
                    uint32_t                     hUser);

    AdsNotification(const AdsDevice&             route,
                    uint32_t                     indexGroup,
                    uint32_t                     indexOffset,
                    const AdsNotificationAttrib& notificationAttributes,
                    PAdsNotificationFuncEx       callback,
                    uint32_t                     hUser);
private:
    AdsHandle m_Symbol;
    AdsHandle m_Notification;
};
