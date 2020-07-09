#include "AdsNotificationOOI.h"
#include "AdsLib.h"

AdsNotification::AdsNotification(const AdsDevice&             route,
                                 const std::string&           symbolName,
                                 const AdsNotificationAttrib& notificationAttributes,
                                 PAdsNotificationFuncEx       callback,
                                 const uint32_t               hUser)
    : m_Symbol(route.GetHandle(symbolName)),
    m_Notification(route.GetHandle(ADSIGRP_SYM_VALBYHND, *m_Symbol, notificationAttributes, callback, hUser))
{}

AdsNotification::AdsNotification(const AdsDevice&             route,
                                 uint32_t                     indexGroup,
                                 uint32_t                     indexOffset,
                                 const AdsNotificationAttrib& notificationAttributes,
                                 PAdsNotificationFuncEx       callback,
                                 const uint32_t               hUser)
    : m_Symbol{route.GetHandle(indexOffset)},
    m_Notification(route.GetHandle(indexGroup, indexOffset, notificationAttributes, callback, hUser))
{}
