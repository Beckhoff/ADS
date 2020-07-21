// SPDX-License-Identifier: MIT
/** @file
   Copyright (c) 2015 - 2020 Beckhoff Automation GmbH & Co. KG
 */
#pragma once

#include "AdsDef.h"

/**
 * Add new ams route to target system
 * @param[in] ams address of the target system
 * @param[in] ip address of the target system
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsAddRoute(AmsNetId ams, const char* ip);

/**
 * Delete ams route that had previously been added with AdsAddRoute().
 * @param[in] ams address of the target system
 */
void AdsDelRoute(AmsNetId ams);

/**
 * The connection (communication port) to the message router is
 * closed. The port to be closed must previously have been opened via
 * an AdsPortOpenEx() call.
 * @param[in] port port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
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
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsGetLocalAddressEx(long port, AmsAddr* pAddr);

/**
 * Change local NetId
 * @param[in] ams local AmsNetId
 */
void AdsSetLocalAddress(AmsNetId ams);

/**
 * Alters the timeout for the ADS functions. The standard value is 5000 ms.
 * @param[in] port port number of an Ads port that had previously been opened with AdsPortOpenEx().
 * @param[in] timeout Timeout in ms.
 * @return [ADS Return Code](http://infosys.beckhoff.de/content/1033/tc3_adsdll2/html/ads_returncodes.htm?id=17663)
 */
long AdsSyncSetTimeoutEx(long port, uint32_t timeout);
