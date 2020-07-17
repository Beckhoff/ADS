#pragma once

#include "AdsDevice.h"

typedef void (* PAdsNotificationFuncExConst)(const AmsAddr* pAddr, const AdsNotificationHeader* pNotification,
                                             uint32_t hUser);
typedef void (* PAdsNotificationFuncExLegacy)(AmsAddr* pAddr, AdsNotificationHeader* pNotification, uint32_t hUser);

struct AdsNotification {
    AdsNotification(const AdsDevice&             route,
                    const std::string&           symbolName,
                    const AdsNotificationAttrib& notificationAttributes,
                    PAdsNotificationFuncExConst  callback,
                    uint32_t                     hUser)
        : m_Symbol(route.GetHandle(symbolName)),
        m_Notification(route.GetHandle(ADSIGRP_SYM_VALBYHND, *m_Symbol, notificationAttributes,
                                       reinterpret_cast<PAdsNotificationFuncEx>(callback), hUser))
    {}

    AdsNotification(const AdsDevice&             route,
                    const std::string&           symbolName,
                    const AdsNotificationAttrib& notificationAttributes,
                    PAdsNotificationFuncExLegacy callback,
                    uint32_t                     hUser)
        : m_Symbol(route.GetHandle(symbolName)),
        m_Notification(route.GetHandle(ADSIGRP_SYM_VALBYHND, *m_Symbol, notificationAttributes,
                                       reinterpret_cast<PAdsNotificationFuncEx>(callback), hUser))
    {}

    AdsNotification(const AdsDevice&             route,
                    uint32_t                     indexGroup,
                    uint32_t                     indexOffset,
                    const AdsNotificationAttrib& notificationAttributes,
                    PAdsNotificationFuncExConst  callback,
                    uint32_t                     hUser)
        : m_Symbol{route.GetHandle(indexOffset)},
        m_Notification(route.GetHandle(indexGroup, indexOffset, notificationAttributes,
                                       reinterpret_cast<PAdsNotificationFuncEx>(callback), hUser))
    {}

    AdsNotification(const AdsDevice&             route,
                    uint32_t                     indexGroup,
                    uint32_t                     indexOffset,
                    const AdsNotificationAttrib& notificationAttributes,
                    PAdsNotificationFuncExLegacy callback,
                    uint32_t                     hUser)
        : m_Symbol{route.GetHandle(indexOffset)},
        m_Notification(route.GetHandle(indexGroup, indexOffset, notificationAttributes,
                                       reinterpret_cast<PAdsNotificationFuncEx>(callback), hUser))
    {}

private:
    AdsHandle m_Symbol;
    AdsHandle m_Notification;
};
