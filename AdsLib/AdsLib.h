// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2020 Beckhoff Automation GmbH & Co. KG
 */
#pragma once

#if defined(USE_TWINCAT_ROUTER)
#include "TwinCAT/AdsLib.h"
#else
#include "standalone/AdsLib.h"
#endif

/**
 * Reads data synchronously from an ADS server.
 * @param[in] port port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @param[in] pAddr Structure with NetId and port number of the ADS server.
 * @param[in] indexGroup Index Group.
 * @param[in] indexOffset Index Offset.
 * @param[in] bufferLength Length of the data in bytes.
 * @param[out] buffer Pointer to a data buffer that will receive the data.
 * @param[out] bytesRead pointer to a variable. If successful, this variable will return the number of actually read data bytes.
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
 * @param[in] port port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @param[in] pAddr Structure with NetId and port number of the ADS server.
 * @param[out] devName Pointer to a character string of at least 16 bytes, that will receive the name of the ADS device.
 * @param[out] version Address of a variable of type AdsVersion, which will receive the version number, revision number and the build number.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncReadDeviceInfoReqEx(long port, const AmsAddr* pAddr, char* devName, AdsVersion* version);

/**
 * Reads the ADS status and the device status from an ADS server.
 * @param[in] port port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @param[in] pAddr Structure with NetId and port number of the ADS server.
 * @param[out] adsState Address of a variable that will receive the ADS status (see data type [ADSSTATE](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/tcadsdll_enumadsstate.htm?id=17630)).
 * @param[out] devState Address of a variable that will receive the device status.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncReadStateReqEx(long port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* devState);

/**
 * Writes data synchronously into an ADS server and receives data back from the ADS server.
 * @param[in] port  port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @param[in] pAddr Structure with NetId and port number of the ADS server.
 * @param[in] indexGroup Index Group.
 * @param[in] indexOffset Index Offset.
 * @param[in] readLength Length, in bytes, of the read buffer readData.
 * @param[out] readData Buffer for data read from the ADS server.
 * @param[in] writeLength Length of the data, in bytes, send to the ADS server.
 * @param[in] writeData Buffer with data send to the ADS server.
 * @param[out] bytesRead pointer to a variable. If successful, this variable will return the number of actually read data bytes.
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
 * @param[in] port port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @param[in] pAddr Structure with NetId and port number of the ADS server.
 * @param[in] indexGroup Index Group.
 * @param[in] indexOffset Index Offset.
 * @param[in] bufferLength Length of the data, in bytes, send to the ADS server.
 * @param[in] buffer Buffer with data send to the ADS server.
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
 * @param[in] port port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @param[in] pAddr Structure with NetId and port number of the ADS server.
 * @param[in] adsState New ADS status.
 * @param[in] devState New device status.
 * @param[in] bufferLength Length of the additional data, in bytes, send to the ADS server.
 * @param[in] buffer Buffer with additional data send to the ADS server.
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
 * @param[in] port port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @param[in] pAddr Structure with NetId and port number of the ADS server.
 * @param[in] indexGroup Index Group.
 * @param[in] indexOffset Index Offset.
 * @param[in] pAttrib Pointer to the structure that contains further information.
 * @param[in] pFunc Pointer to the structure describing the callback function.
 * @param[in] hUser 32-bit value that is passed to the callback function.
 * @param[out] pNotification Address of the variable that will receive the handle of the notification.
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
 * @param[in] port port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @param[in] pAddr Structure with NetId and port number of the ADS server.
 * @param[in] hNotification Address of the variable that contains the handle of the notification.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncDelDeviceNotificationReqEx(long port, const AmsAddr* pAddr, uint32_t hNotification);

/**
 * Read the configured timeout for the ADS functions. The standard value is 5000 ms.
 * @param[in] port port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @param[out] timeout Buffer to store timeout value in ms.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncGetTimeoutEx(long port, uint32_t* timeout);
