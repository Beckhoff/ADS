// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2020 Beckhoff Automation GmbH & Co. KG
 */
#pragma once

#define POSIX
#define NULL nullptr
#define AMSPORT_R0_PLC_TC3 851
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
