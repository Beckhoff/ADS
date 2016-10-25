#pragma once

#include "AdsHandle.h"
#include "AdsRoute.h"

struct AdsNotification {
// Sends the AddDeviceNotification command to the target machine
    static AdsNotification Register(const AdsRoute&              route,
                                    const std::string&           symbolName,
                                    const AdsNotificationAttrib& notificationAttributes,
                                    PAdsNotificationFuncEx       callback);

    static AdsNotification Register(const AdsRoute&              route,
                                    uint32_t                     indexGroup,
                                    uint32_t                     indexOffset,
                                    const AdsNotificationAttrib& notificationAttributes,
                                    PAdsNotificationFuncEx       callback);
private:
    AdsNotification(std::shared_ptr<uint32_t> hNotify, AdsHandle hSymbol)
        : m_Notification(hNotify),
        m_Symbol(std::move(hSymbol))
    {}

    std::shared_ptr<uint32_t> m_Notification;
    AdsHandle m_Symbol;
};
#if 0
template<typename T>
class AdsCyclicNotification : public AdsNotification {
public:
    AdsCyclicNotification(AdsVariable<T>& var, uint32_t cycleTimeInMs, std::function<void()> callback)
        : AdsNotification(var.GetRoute(), var.GetIndexGroup(), var.GetHandle(), GetNotificationAttributes(
                              cycleTimeInMs), callback)
    {}

private:

    AdsNotificationAttrib GetNotificationAttributes(uint32_t cycleTimeInMs)
    {
        const uint32_t MillisecondsTo100NsTicks = 10000;
        AdsNotificationAttrib notificationAttributes;
        memset(&notificationAttributes, 0, sizeof(notificationAttributes));
        notificationAttributes.cbLength = sizeof(T);
        notificationAttributes.nCycleTime = cycleTimeInMs * MillisecondsTo100NsTicks;
        notificationAttributes.nMaxDelay = 0;
        notificationAttributes.nTransMode = ADSTRANS_SERVERCYCLE;
        return notificationAttributes;
    }
};

template<typename T>
class AdsChangeNotification : public AdsNotification {
public:
    AdsChangeNotification(AdsVariable<T>& var, std::function<void()> callback)
        : AdsNotification(var.GetRoute(), var.GetIndexGroup(), var.GetHandle(), GetNotificationAttributes(), callback)
    {}

private:
    AdsNotificationAttrib GetNotificationAttributes()
    {
        AdsNotificationAttrib notificationAttributes;
        memset(&notificationAttributes, 0, sizeof(notificationAttributes));
        notificationAttributes.cbLength = sizeof(T);
        notificationAttributes.nMaxDelay = 0;
        notificationAttributes.nTransMode = ADSTRANS_SERVERONCHA;
        return notificationAttributes;
    }
};
#endif
/*
    Doesn't work. Connection to host is closed as soon as the device goes back to config-mode.


   class AdsDeviceStatusChangedNotification : public AdsNotification
   {
   public:
    AdsDeviceStatusChangedNotification(const AdsRoute& route, std::function<void()> callback)
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
