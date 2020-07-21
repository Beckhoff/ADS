// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2020 Beckhoff Automation GmbH & Co. KG
 */

#include "AdsLib.h"

long AdsAddRoute(AmsNetId, const char*)
{
    return 0;
}

void AdsDelRoute(AmsNetId)
{}

void AdsSetLocalAddress(const AmsNetId)
{}

long AdsSyncDelDeviceNotificationReqEx(long port, const AmsAddr* pAddr, uint32_t hNotification)
{
    return AdsSyncDelDeviceNotificationReqEx((ads_i32)port, (AmsAddr*)pAddr, hNotification);
}

long AdsSyncGetTimeoutEx(long port, uint32_t* timeout)
{
    return AdsSyncGetTimeoutEx((ads_i32)port, (ads_i32*)timeout);
}

long AdsSyncSetTimeoutEx(long port, uint32_t timeout)
{
    return AdsSyncSetTimeoutEx((ads_i32)port, (ads_i32)timeout);
}

long AdsSyncReadDeviceInfoReqEx(long port, const AmsAddr* pAddr, char* devName, AdsVersion* version)
{
    return AdsSyncReadDeviceInfoReqEx((ads_i32)port, (AmsAddr*)pAddr, devName, version);
}

long AdsSyncReadReqEx2(long           port,
                       const AmsAddr* pAddr,
                       uint32_t       indexGroup,
                       uint32_t       indexOffset,
                       uint32_t       bufferLength,
                       void*          buffer,
                       uint32_t*      bytesRead)
{
    return AdsSyncReadReqEx2((ads_i32)port,
                             (AmsAddr*)pAddr,
                             indexGroup,
                             indexOffset,
                             bufferLength,
                             buffer,
                             bytesRead);
}

long AdsSyncReadStateReqEx(long port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* devState)
{
    return AdsSyncReadStateReqEx((ads_i32)port, (AmsAddr*)pAddr, adsState, devState);
}

long AdsSyncReadWriteReqEx2(long           port,
                            const AmsAddr* pAddr,
                            uint32_t       indexGroup,
                            uint32_t       indexOffset,
                            uint32_t       readLength,
                            void*          readData,
                            uint32_t       writeLength,
                            const void*    writeData,
                            uint32_t*      bytesRead)
{
    return AdsSyncReadWriteReqEx2((ads_i32)port,
                                  (AmsAddr*)pAddr,
                                  indexGroup,
                                  indexOffset,
                                  readLength,
                                  readData,
                                  writeLength,
                                  (void*)writeData,
                                  bytesRead);
}

long AdsSyncAddDeviceNotificationReqEx(long                         port,
                                       const AmsAddr*               pAddr,
                                       uint32_t                     indexGroup,
                                       uint32_t                     indexOffset,
                                       const AdsNotificationAttrib* pAttrib,
                                       PAdsNotificationFuncEx       pFunc,
                                       uint32_t                     hUser,
                                       uint32_t*                    pNotification)
{
    return AdsSyncAddDeviceNotificationReqEx((ads_i32)port,
                                             (AmsAddr*)pAddr,
                                             indexGroup,
                                             indexOffset,
                                             (AdsNotificationAttrib*)pAttrib,
                                             pFunc,
                                             hUser,
                                             pNotification);
}

long AdsSyncWriteControlReqEx(long           port,
                              const AmsAddr* pAddr,
                              uint16_t       adsState,
                              uint16_t       devState,
                              uint32_t       bufferLength,
                              const void*    buffer)
{
    return AdsSyncWriteControlReqEx((ads_i32)port,
                                    (AmsAddr*)pAddr,
                                    adsState,
                                    devState,
                                    bufferLength,
                                    (void*)buffer);
}

long AdsSyncWriteReqEx(long           port,
                       const AmsAddr* pAddr,
                       uint32_t       indexGroup,
                       uint32_t       indexOffset,
                       uint32_t       bufferLength,
                       const void*    buffer)
{
    return AdsSyncWriteReqEx((ads_i32)port,
                             (AmsAddr*)pAddr,
                             indexGroup,
                             indexOffset,
                             bufferLength,
                             (void*)buffer);
}
