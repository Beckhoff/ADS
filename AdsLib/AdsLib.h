#ifndef _ADSLIB_H_
#define _ADSLIB_H_

#include <cstdint>
#include "AdsDef.h"
#include "Sockets.h"

long AdsAddRoute(AmsNetId ams, IpV4 ip);

long AdsPortCloseEx(long port);
long AdsPortOpenEx();
long AdsGetLocalAddressEx(long port, AmsAddr* pAddr);
long AdsSyncReadReqEx2(long port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, void* buffer, uint32_t *bytesRead);
long AdsSyncReadDeviceInfoReqEx(long port, const AmsAddr* pAddr, char* devName, AdsVersion* version);
long AdsSyncReadStateReqEx(long port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* deviceState);

#endif /* #ifndef _ADSLIB_H_ */