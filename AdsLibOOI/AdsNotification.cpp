#include "AdsNotification.h"
#include "AdsLib/AdsLib.h"

struct NotificationHandleDeleter : public SymbolHandleDeleter {
    NotificationHandleDeleter(const AdsRoute& route)
        : SymbolHandleDeleter(route)
    {}

    virtual void operator()(uint32_t* handle)
    {
        uint32_t error = 0;
        if (handle && *handle) {
            error = AdsSyncDelDeviceNotificationReqEx(m_Route.GetLocalPort(), &m_Route.m_Port, *handle);
        }
        delete handle;

        if (error) {
            throw AdsException(error);
        }
    }
};

AdsHandle GetNotificationHandle(const AdsRoute&              route,
                                uint32_t                     indexGroup,
                                uint32_t                     indexOffset,
                                const AdsNotificationAttrib& notificationAttributes,
                                PAdsNotificationFuncEx       callback)
{
    uint32_t handle = 0;
    auto error = AdsSyncAddDeviceNotificationReqEx(
        route.GetLocalPort(), &route.m_Port,
        indexGroup, indexOffset,
        &notificationAttributes,
        callback,
        indexOffset,
        &handle);
    if (error || !handle) {
        throw AdsException(error);
    }
    return {new uint32_t {handle}, NotificationHandleDeleter {route}};
}

AdsNotification::AdsNotification(const AdsRoute&              route,
                                 const std::string&           symbolName,
                                 const AdsNotificationAttrib& notificationAttributes,
                                 PAdsNotificationFuncEx       callback)
    : m_Symbol(route.GetHandle(symbolName)),
    m_Notification(GetNotificationHandle(route, ADSIGRP_SYM_VALBYHND, *m_Symbol, notificationAttributes, callback))
{}

AdsNotification::AdsNotification(const AdsRoute&              route,
                                 uint32_t                     indexGroup,
                                 uint32_t                     indexOffset,
                                 const AdsNotificationAttrib& notificationAttributes,
                                 PAdsNotificationFuncEx       callback)
    : m_Symbol{route.GetHandle(indexOffset)},
    m_Notification(GetNotificationHandle(route, indexGroup, indexOffset, notificationAttributes, callback))
{}
