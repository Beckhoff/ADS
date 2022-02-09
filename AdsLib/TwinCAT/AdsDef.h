// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2020 - 2022 Beckhoff Automation GmbH & Co. KG
 */
#pragma once
#ifdef __FreeBSD__
#define POSIX
#define NULL nullptr
#define AMSPORT_R0_PLC_TC3 851
#else
using BOOL = int;
#define TCADSDLL_API __stdcall
using ads_i32 = long;
using ads_ui16 = unsigned short;
using ads_ui32 = unsigned long;
#define __int64 long long
#endif
#include <string>
#include <stdint.h>
#include <TcAdsDef.h>

////////////////////////////////////////////////////////////////////////////////
// ADS Cmd Ids
#define ADSSRVID_INVALID                    0x00
#define ADSSRVID_READDEVICEINFO             0x01
#define ADSSRVID_READ                       0x02
#define ADSSRVID_WRITE                      0x03
#define ADSSRVID_READSTATE                  0x04
#define ADSSRVID_WRITECTRL                  0x05
#define ADSSRVID_ADDDEVICENOTE              0x06
#define ADSSRVID_DELDEVICENOTE              0x07
#define ADSSRVID_DEVICENOTE                 0x08
#define ADSSRVID_READWRITE                  0x09

enum nSystemServiceIndexGroups : uint32_t {
    SYSTEMSERVICE_FOPEN = 120,
    SYSTEMSERVICE_FCLOSE = 121,
    SYSTEMSERVICE_FREAD = 122,
    SYSTEMSERVICE_FWRITE = 123,
    SYSTEMSERVICE_FDELETE = 131,
};
