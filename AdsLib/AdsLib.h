#ifndef _ADSLIB_H_
#define _ADSLIB_H_

#include <cstdint>
#include "AdsDef.h"

long AdsAddRoute(AmsNetId ams, const char* ip);
void AdsDelRoute(AmsNetId ams);
void AdsSetNetId(AmsNetId ams);

long AdsPortCloseEx(long port);
long AdsPortOpenEx();
long AdsGetLocalAddressEx(long port, AmsAddr* pAddr);
long AdsSyncReadReqEx2(long port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, void* buffer, uint32_t *bytesRead);
long AdsSyncReadDeviceInfoReqEx(long port, const AmsAddr* pAddr, char* devName, AdsVersion* version);
long AdsSyncReadStateReqEx(long port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* devState);
long AdsSyncReadWriteReqEx2(long port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t readLength, void* readData, uint32_t writeLength, const void* writeData, uint32_t *bytesRead);
long AdsSyncWriteReqEx(long port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, const void* buffer);
long AdsSyncWriteControlReqEx(long port, const AmsAddr* pAddr, uint16_t adsState, uint16_t devState, uint32_t bufferLength, const void* buffer);
long AdsSyncAddDeviceNotificationReqEx(long port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, const AdsNotificationAttrib* pAttrib, PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t *pNotification);
long AdsSyncDelDeviceNotificationReqEx(long port, const AmsAddr* pAddr, uint32_t hNotification);
long AdsSyncGetTimeoutEx(long port, uint32_t *timeout);
long AdsSyncSetTimeoutEx(long port, uint32_t timeout);

#endif /* #ifndef _ADSLIB_H_ */