// SPDX-License-Identifier: MIT
/** @file
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */
#pragma once

#include "AdsDef.h"

#ifdef BHF_ADS_EXPORT_C
extern "C" {
#endif
/**
 * Adds a standalone route with a timeout for the initial TCP connection.
 * @param[in] ams NetId of the target system.
 * @param[in] ip IP address or hostname, optionally including a TCP port.
 * @param[in] timeout Timeout in ms; 0 preserves AdsAddRoute()'s OS timeout.
 * The timeout starts after name resolution and covers waiting for another
 * connection attempt to this NetId and all resolved TCP addresses. It does
 * not change ADS request timeouts or cancel another thread's connection.
 * Existing connections are reused as with AdsAddRoute().
 * @return 0 on success, GLOBALERR_TARGET_PORT on connection failure or timeout,
 *         or the same other error codes as AdsAddRoute().
 */
long AdsAddRouteEx(AmsNetId ams, const char *ip, uint32_t timeout);

/**
 * The connection (communication port) to the message router is
 * closed. The port to be closed must previously have been opened via
 * an AdsPortOpenEx() call.
 * @param[in] port port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @return [ADS Return Code](https://infosys.beckhoff.com/content/1031/tcadscommon/html/ads_returncodes.htm?id=1666172286265530469)
 */
long AdsPortCloseEx(long port);

/**
 * Establishes a connection (communication port) to the message
 * router. The port number returned by AdsPortOpenEx() is required as
 * parameter for further AdsLib function calls.
 * @return port number of a new Ads port or 0 if no more ports available
 */
long AdsPortOpenEx();

/**
 * Returns the local NetId and port number.
 * @param[in] port port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @param[out] pAddr Pointer to the structure of type AmsAddr.
 * @return [ADS Return Code](https://infosys.beckhoff.com/content/1031/tcadscommon/html/ads_returncodes.htm?id=1666172286265530469)
 */
long AdsGetLocalAddressEx(long port, AmsAddr *pAddr);

/**
 * Alters the timeout for the ADS functions. The standard value is 5000 ms.
 * @param[in] port port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @param[in] timeout Timeout in ms.
 * @return [ADS Return Code](https://infosys.beckhoff.com/content/1031/tcadscommon/html/ads_returncodes.htm?id=1666172286265530469)
 */
long AdsSyncSetTimeoutEx(long port, uint32_t timeout);
#ifdef BHF_ADS_EXPORT_C
}
#endif
