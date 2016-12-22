#include "AdsNotification.h"

static void HandleDeleter(const AdsRoute& route, uint32_t* handle)
{
    uint32_t error = 0;
    if (handle && *handle) {
        error = AdsSyncDelDeviceNotificationReqEx(route.GetLocalPort(), &route.m_Port, *handle);

        delete handle;
    }

    if (error) {
        throw AdsException(error);
    }
}

std::shared_ptr<uint32_t> GetNotificationHandle(const AdsRoute&              route,
                                                uint32_t                     indexGroup,
                                                uint32_t                     indexOffset,
                                                const AdsNotificationAttrib& notificationAttributes,
                                                PAdsNotificationFuncEx       callback)
{
    uint32_t handle = 0;
    auto error = AdsSyncAddDeviceNotificationReqEx(
        route.GetLocalPort(),
        &route.m_Port,
        indexGroup,
        indexOffset,
        &notificationAttributes,
        callback,
        indexOffset,
        &handle);
    if (error || !handle) {
        throw AdsException(error);
    }
    return {new uint32_t {handle}, std::bind(HandleDeleter, route, std::placeholders::_1)};
}

AdsNotification AdsNotification::Register(const AdsRoute&              route,
                                          const std::string&           symbolName,
                                          const AdsNotificationAttrib& notificationAttributes,
                                          PAdsNotificationFuncEx       callback)
{
    AdsHandle symbolHandle {route.m_Port, route.GetLocalPort(), symbolName};

    uint32_t hSymbol = symbolHandle;
    return {GetNotificationHandle(route, ADSIGRP_SYM_VALBYHND, hSymbol, notificationAttributes, callback),
            std::move(symbolHandle)};
}

AdsNotification AdsNotification::Register(const AdsRoute&              route,
                                          uint32_t                     indexGroup,
                                          uint32_t                     indexOffset,
                                          const AdsNotificationAttrib& notificationAttributes,
                                          PAdsNotificationFuncEx       callback)
{
    return {GetNotificationHandle(route, indexGroup, indexOffset, notificationAttributes, callback),
            {indexOffset}};
}
