#include "AdsNotification.h"

namespace AdsNotification
{
static void HandleDeleter(const AdsRoute& route, uint32_t* handle)
{
    uint32_t error = 0;
    if (handle && *handle) {
        auto amsAddr = route->GetSymbolsAmsAddr();
        error = AdsSyncDelDeviceNotificationReqEx(route->GetLocalPort(), &amsAddr, *handle);

        delete handle;
    }

    if (error) {
        throw AdsException(error);
    }
}

Handle Register(const AdsRoute&              route,
                uint32_t                     indexGroup,
                uint32_t                     indexOffset,
                const AdsNotificationAttrib& notificationAttributes,
                PAdsNotificationFuncEx       callback)
{
    auto amsAddr = route->GetSymbolsAmsAddr();
    uint32_t handle = 0;
    auto error = AdsSyncAddDeviceNotificationReqEx(
        route->GetLocalPort(),
        &amsAddr,
        indexGroup,
        indexOffset,
        &notificationAttributes,
        callback,
        indexOffset,
        &handle);
    if (error || !handle) {
        throw AdsException(error);
    }

    return {new uint32_t { handle }, std::bind(HandleDeleter, route, std::placeholders::_1)};
}
}
