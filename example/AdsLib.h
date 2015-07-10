/**
   Copyright (c) 2015 Beckhoff Automation GmbH & Co. KG

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
 */

#ifndef _ADSLIB_H_
#define _ADSLIB_H_

#include "AdsDef.h"

long AdsAddRoute(AmsNetId ams, const char* ip);
void AdsDelRoute(AmsNetId ams);

long AdsPortCloseEx(long port);
long AdsPortOpenEx();
long AdsGetLocalAddressEx(long port, AmsAddr* pAddr);
long AdsSyncReadReqEx2(long           port,
                       const AmsAddr* pAddr,
                       uint32_t       indexGroup,
                       uint32_t       indexOffset,
                       uint32_t       bufferLength,
                       void*          buffer,
                       uint32_t*      bytesRead);
long AdsSyncReadDeviceInfoReqEx(long port, const AmsAddr* pAddr, char* devName, AdsVersion* version);
long AdsSyncReadStateReqEx(long port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* devState);
long AdsSyncReadWriteReqEx2(long           port,
                            const AmsAddr* pAddr,
                            uint32_t       indexGroup,
                            uint32_t       indexOffset,
                            uint32_t       readLength,
                            void*          readData,
                            uint32_t       writeLength,
                            const void*    writeData,
                            uint32_t*      bytesRead);
long AdsSyncWriteReqEx(long           port,
                       const AmsAddr* pAddr,
                       uint32_t       indexGroup,
                       uint32_t       indexOffset,
                       uint32_t       bufferLength,
                       const void*    buffer);
long AdsSyncWriteControlReqEx(long           port,
                              const AmsAddr* pAddr,
                              uint16_t       adsState,
                              uint16_t       devState,
                              uint32_t       bufferLength,
                              const void*    buffer);
long AdsSyncAddDeviceNotificationReqEx(long                         port,
                                       const AmsAddr*               pAddr,
                                       uint32_t                     indexGroup,
                                       uint32_t                     indexOffset,
                                       const AdsNotificationAttrib* pAttrib,
                                       PAdsNotificationFuncEx       pFunc,
                                       uint32_t                     hUser,
                                       uint32_t*                    pNotification);
long AdsSyncDelDeviceNotificationReqEx(long port, const AmsAddr* pAddr, uint32_t hNotification);
long AdsSyncGetTimeoutEx(long port, uint32_t* timeout);
long AdsSyncSetTimeoutEx(long port, uint32_t timeout);

#endif /* #ifndef _ADSLIB_H_ */
