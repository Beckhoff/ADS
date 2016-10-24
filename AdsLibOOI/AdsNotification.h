#pragma once

#include "AdsVariable.h"
#include "AdsRoute.h"
#include "AdsNotificationCallbacks.h"
#include <cstring>

class AdsNotification {
protected:
    AdsNotification(const AdsRoute&              route,
                    const uint32_t               indexGroup,
                    const uint32_t               indexOffset,
                    const AdsNotificationAttrib& NotificationAttributes,
                    std::function<void()>        callback);

private:
    // Functor that is called once a NotificationHandle is not needed any longer
    struct NotificationDeleter {
        NotificationDeleter(const AmsAddr __address = AmsAddr {}, long __port = 0) : address(__address),
                                                                                     port(__port)
        {}

        void operator()(uint32_t* handle)
        {
            uint32_t error = 0;
            if (handle && *handle) {
                if (port) {
                    error = AdsSyncDelDeviceNotificationReqEx(
                        port,
                        &address,
                        *handle
                        );
                }
                AdsNotificationCallbacks::RemoveCallback(*handle);
                delete handle;
            }

            if (error) {
                throw AdsException(error);
            }
        }

private:
        const AmsAddr address;
        const long port;
    };

    // Type for guarded handles
    using AdsNotificationHandleGuard = std::unique_ptr<uint32_t, NotificationDeleter>;

    // Sends the AddDeviceNotification command to the target machine
    AdsNotificationHandleGuard RegisterNotification(const AdsRoute&              route,
                                                    uint32_t                     indexGroup,
                                                    uint32_t                     indexOffset,
                                                    const AdsNotificationAttrib& notificationAttributes,
                                                    std::function<void()>        callback);

    const AdsRoute m_Route;                             // Route to target machine
    const AdsNotificationHandleGuard m_Handle;          // Guarded Notification handle
    const std::function<void()> m_Callback;             // The callback which is called when a notification event is received
};

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
