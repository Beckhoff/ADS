// SPDX-License-Identifier: MIT
#pragma once

#include "AdsDef.h"
#include <cstdint>

namespace TlsConnectInfo
{
static const uint8_t VERSION = 1;
static const uint16_t FLAG_RESPONSE = 0x0001;
static const uint16_t FLAG_AMS_ALLOWED = 0x0002;
static const uint16_t FLAG_SERVER_INFO = 0x0004;
static const uint16_t FLAG_OWN_FILE = 0x0008;
static const uint16_t FLAG_SELF_SIGNED = 0x0010;
static const uint16_t FLAG_IP_ADDR = 0x0020;
static const uint16_t FLAG_IGNORE_CN = 0x0040;
static const uint16_t FLAG_ADD_REMOTE = 0x0080;
}

#pragma pack(push, 1)
struct TlsConnectInfoBase {
	uint16_t totalLength;
	uint16_t flags;
	uint8_t version;
	uint8_t error;
	AmsNetId netId;
	uint8_t userLen;
	uint8_t pwdLen;
	uint8_t reserved[18];
	char hostname[32];
};
#pragma pack(pop)

static_assert(sizeof(TlsConnectInfoBase) == 64,
	      "TlsConnectInfoBase wire layout must be 64 bytes");
