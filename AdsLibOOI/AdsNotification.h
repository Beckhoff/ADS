#pragma once

#include "AdsDevice.h"

struct AdsNotification {
    AdsNotification(const AdsDevice&             route,
                    const std::string&           symbolName,
                    const AdsNotificationAttrib& notificationAttributes,
                    PAdsNotificationFuncEx       callback);

    AdsNotification(const AdsDevice&             route,
                    uint32_t                     indexGroup,
                    uint32_t                     indexOffset,
                    const AdsNotificationAttrib& notificationAttributes,
                    PAdsNotificationFuncEx       callback);
private:
    AdsHandle m_Symbol;
    AdsHandle m_Notification;
};

/*
    Doesn't work. Connection to host is closed as soon as the device goes back to config-mode.


   class AdsDeviceStatusChangedNotification : public AdsNotification
   {
   public:
    AdsDeviceStatusChangedNotification(const AdsDevice& route, std::function<void()> callback)
        :AdsNotification(route, ADSIGRP_DEVICE_DATA, ADSIOFFS_DEVDATA_ADSSTATE, GetNotificationAttributes(), callback)
    {
    }

   private:
    AdsNotificationAttrib GetNotificationAttributes()
    {
        AdsNotificationAttrib notificationAttributes;
        memset(&notificationAttributes, 0, sizeof(notificationAttributes));
        notificationAttributes.cbLength = sizeof(uint16_t);
        notificationAttributes.nMaxDelay = 0;
        notificationAttributes.nTransMode = ADSTRANS_SERVERONCHA;
        return notificationAttributes;
    }
   };

 */
