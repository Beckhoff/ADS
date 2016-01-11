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

/** @file */

/**
 * Add new ams route to target system
 * @param ams [in] address of the target system
 * @param ip [in] address of the target system
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsAddRoute(AmsNetId ams, const char* ip);

/**
 * Delete ams route
 * @param ams [in] address of the target system
 */
void AdsDelRoute(AmsNetId ams);

/**
 * The connection (communication port) to the message router is
 * closed. The port to be closed must previously have been opened via
 * an AdsPortOpenEx call.
 * @param port [in] port number of an Ads port that had previously been opened with AdsPortOpenEx.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsPortCloseEx(long port);

/**
 * Establishes a connection (communication port) to the message
 * router. The port number returned by AdsPortOpenEx is required as
 * parameter for further AdsLib function calls.
 * @return port number of a new Ads port or 0 if no more ports available
 */
long AdsPortOpenEx();

/**
 * Returns the local NetId and port number.
 * @param port [in] port number of an Ads port that had previously been opened with AdsPortOpenEx.
 * @param pAddr [out] Pointer to the structure of type AmsAddr.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsGetLocalAddressEx(long port, AmsAddr* pAddr);

/**
 * Reads data synchronously from an ADS server.
 * @param port [in] port number of an Ads port that had previously been opened with AdsPortOpenEx.
 * @param pAddr [in] Structure with NetId and port number of the ADS server.
 * @param indexGroup [in] Index Group.
 * @param indexOffset [in] Index Offset.
 * @param bufferLength [in] Length of the data in bytes.
 * @param buffer [out] Pointer to a data buffer that will receive the data.
 * @param bytesRead [out] pointer to a variable. If successful, this variable will return the number of actually read data bytes.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncReadReqEx2(long           port,
                       const AmsAddr* pAddr,
                       uint32_t       indexGroup,
                       uint32_t       indexOffset,
                       uint32_t       bufferLength,
                       void*          buffer,
                       uint32_t*      bytesRead);

/**
 * Reads the identification and version number of an ADS server.
 * @param port [in] port number of an Ads port that had previously been opened with AdsPortOpenEx.
 * @param pAddr [in] Structure with NetId and port number of the ADS server.
 * @param devName [out] Pointer to a character string of at least 16 bytes, that will receive the name of the ADS device.
 * @param version [out] Address of a variable of type AdsVersion, which will receive the version number, revision number and the build number.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncReadDeviceInfoReqEx(long port, const AmsAddr* pAddr, char* devName, AdsVersion* version);

/**
 * Reads the ADS status and the device status from an ADS server.
 * @param port [in] port number of an Ads port that had previously been opened with AdsPortOpenEx.
 * @param pAddr [in] Structure with NetId and port number of the ADS server.
 * @param adsState [out] Address of a variable that will receive the ADS status (see data type ADSSTATE).
 * @param devState [out] Address of a variable that will receive the device status.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncReadStateReqEx(long port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* devState);

/**
 * Writes data synchronously into an ADS server and receives data back from the ADS server.
 * @param port [in] port number of an Ads port that had previously been opened with AdsPortOpenEx.
 * @param pAddr [in] Structure with NetId and port number of the ADS server.
 * @param indexGroup [in] Index Group.
 * @param indexOffset [in] Index Offset.
 * @param readLength[in] Length, in bytes, of the read buffer <readData>.
 * @param readData [out] Buffer for data read from the ADS server.
 * @param writeLength[in] Length of the data, in bytes, send to the ADS server.
 * @param writeData [in] Buffer with data send to the ADS server.
 * @param bytesRead [out] pointer to a variable. If successful, this variable will return the number of actually read data bytes.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncReadWriteReqEx2(long           port,
                            const AmsAddr* pAddr,
                            uint32_t       indexGroup,
                            uint32_t       indexOffset,
                            uint32_t       readLength,
                            void*          readData,
                            uint32_t       writeLength,
                            const void*    writeData,
                            uint32_t*      bytesRead);

/**
 * Writes data synchronously to an ADS server.
 * @param port [in] port number of an Ads port that had previously been opened with AdsPortOpenEx.
 * @param pAddr [in] Structure with NetId and port number of the ADS server.
 * @param indexGroup [in] Index Group.
 * @param indexOffset [in] Index Offset.
 * @param bufferLength[in] Length of the data, in bytes, send to the ADS server.
 * @param buffer [in] Buffer with data send to the ADS server.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncWriteReqEx(long           port,
                       const AmsAddr* pAddr,
                       uint32_t       indexGroup,
                       uint32_t       indexOffset,
                       uint32_t       bufferLength,
                       const void*    buffer);

/**
 * Changes the ADS status and the device status of an ADS server.
 * @param port [in] port number of an Ads port that had previously been opened with AdsPortOpenEx.
 * @param pAddr [in] Structure with NetId and port number of the ADS server.
 * @param adsState [in] New ADS status.
 * @param devState [in] New device status.
 * @param bufferLength[in] Length of the additional data, in bytes, send to the ADS server.
 * @param buffer [in] Buffer with additional data send to the ADS server.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncWriteControlReqEx(long           port,
                              const AmsAddr* pAddr,
                              uint16_t       adsState,
                              uint16_t       devState,
                              uint32_t       bufferLength,
                              const void*    buffer);

/**
 * A notification is defined within an ADS server (e.g. PLC). When a
 * certain event occurs a function (the callback function) is invoked in
 * the ADS client (C program).
 * @param port [in] port number of an Ads port that had previously been opened with AdsPortOpenEx.
 * @param pAddr [in] Structure with NetId and port number of the ADS server.
 * @param indexGroup [in] Index Group.
 * @param indexOffset [in] Index Offset.
 * @param pAttrib[in] Pointer to the structure that contains further information.
 * @param pFunc [in] Pointer to the structure describing the callback function.
 * @param hUser [in] 32-bit value that is passed to the callback function.
 * @param pNotification [out] Address of the variable that will receive the handle of the notification.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncAddDeviceNotificationReqEx(long                         port,
                                       const AmsAddr*               pAddr,
                                       uint32_t                     indexGroup,
                                       uint32_t                     indexOffset,
                                       const AdsNotificationAttrib* pAttrib,
                                       PAdsNotificationFuncEx       pFunc,
                                       uint32_t                     hUser,
                                       uint32_t*                    pNotification);

/**
 * A notification defined previously is deleted from an ADS server.
 * @param port [in] port number of an Ads port that had previously been opened with AdsPortOpenEx.
 * @param pAddr [in] Structure with NetId and port number of the ADS server.
 * @param hNotification [in] Address of the variable that contains the handle of the notification.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncDelDeviceNotificationReqEx(long port, const AmsAddr* pAddr, uint32_t hNotification);

/**
 * Read the configured timeout for the ADS functions. The standard value is 5000 ms.
 * @param port [in] port number of an Ads port that had previously been opened with AdsPortOpenEx.
 * @param timeout [out] Buffer to store timeout value in ms.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncGetTimeoutEx(long port, uint32_t* timeout);

/**
 * Alters the timeout for the ADS functions. The standard value is 5000 ms.
 * @param port [in] port number of an Ads port that had previously been opened with AdsPortOpenEx.
 * @param timeout [in] Timeout in ms.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncSetTimeoutEx(long port, uint32_t timeout);

#endif /* #ifndef _ADSLIB_H_ */
