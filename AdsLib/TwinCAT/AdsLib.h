// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2020 Beckhoff Automation GmbH & Co. KG
 */
#pragma once

#include "AdsDef.h"
#include <TcAdsAPI.h>
long AdsAddRoute(AmsNetId, const char*);
void AdsDelRoute(AmsNetId);
void AdsSetLocalAddress(AmsNetId);
