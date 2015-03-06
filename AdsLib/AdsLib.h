#ifndef _ADSLIB_H_
#define _ADSLIB_H_

#include <cstdint>
#include "AdsDef.h"

long AdsPortCloseEx(long port);
long AdsPortOpenEx();
long AdsGetLocalAddressEx(long port, AmsAddr* pAddr);
long AdsSyncReadReqEx2(long port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, void* buffer, uint32_t *bytesRead);

#endif /* #ifndef _ADSLIB_H_ */