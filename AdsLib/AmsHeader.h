// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#define UNUSED(x) (void)(x)

#include "AdsDef.h"
#include "wrap_endian.h"

#include <array>
#include <cstddef>
#include <cstring>

#pragma pack (push, 1)
struct AmsTcpHeader {
    AmsTcpHeader(const uint8_t* frame)
    {
        memcpy(this, frame, sizeof(*this));
    }

    AmsTcpHeader(const uint32_t numBytes = 0)
        : reserved(0),
        leLength(bhf::ads::htole(numBytes))
    {
        UNUSED(reserved);
    }

    uint32_t length() const
    {
        return bhf::ads::letoh(leLength);
    }
private:
    uint16_t reserved;
    uint32_t leLength;
};

struct AoERequestHeader {
    static const uint32_t SDO_UPLOAD = 0xF302;

    AoERequestHeader(uint16_t sdoIndex, uint8_t sdoSubIndex, uint32_t dataLength)
        : AoERequestHeader(SDO_UPLOAD, ((uint32_t)sdoIndex) << 16 | sdoSubIndex, dataLength)
    {}

    AoERequestHeader(uint32_t indexGroup, uint32_t indexOffset, uint32_t dataLength)
        : leGroup(bhf::ads::htole(indexGroup)),
        leOffset(bhf::ads::htole(indexOffset)),
        leLength(bhf::ads::htole(dataLength))
    {}

private:
    const uint32_t leGroup;
    const uint32_t leOffset;
    const uint32_t leLength;
};

struct AoEReadWriteReqHeader : AoERequestHeader {
    AoEReadWriteReqHeader(uint32_t indexGroup, uint32_t indexOffset, uint32_t readLength, uint32_t writeLength)
        : AoERequestHeader(indexGroup, indexOffset, readLength),
        leWriteLength(bhf::ads::htole(writeLength))
    {}
private:
    const uint32_t leWriteLength;
};

struct AdsWriteCtrlRequest {
    AdsWriteCtrlRequest(uint16_t ads, uint16_t dev, uint32_t dataLength)
        : leAdsState(bhf::ads::htole(ads)),
        leDevState(bhf::ads::htole(dev)),
        leLength(bhf::ads::htole(dataLength))
    {}

private:
    const uint16_t leAdsState;
    const uint16_t leDevState;
    const uint32_t leLength;
};

struct AdsAddDeviceNotificationRequest {
    AdsAddDeviceNotificationRequest(uint32_t __group,
                                    uint32_t __offset,
                                    uint32_t __length,
                                    uint32_t __mode,
                                    uint32_t __maxDelay,
                                    uint32_t __cycleTime)
        : leGroup(bhf::ads::htole(__group)),
        leOffset(bhf::ads::htole(__offset)),
        leLength(bhf::ads::htole(__length)),
        leMode(bhf::ads::htole(__mode)),
        leMaxDelay(bhf::ads::htole(__maxDelay)),
        leCycleTime(bhf::ads::htole(__cycleTime)),
        reserved()
    {
        UNUSED(reserved);
    }

private:
    const uint32_t leGroup;
    const uint32_t leOffset;
    const uint32_t leLength;
    const uint32_t leMode;
    const uint32_t leMaxDelay;
    const uint32_t leCycleTime;
    const std::array<uint8_t, 16> reserved;
};

struct AoEHeader {
    static const uint16_t AMS_REQUEST = 0x0004;
    static const uint16_t AMS_RESPONSE = 0x0005;
    static const uint16_t AMS_UDP = 0x0040;
    static const uint16_t INVALID = 0x0000;
    static const uint16_t READ_DEVICE_INFO = 0x0001;
    static const uint16_t READ = 0x0002;
    static const uint16_t WRITE = 0x0003;
    static const uint16_t READ_STATE = 0x0004;
    static const uint16_t WRITE_CONTROL = 0x0005;
    static const uint16_t ADD_DEVICE_NOTIFICATION = 0x0006;
    static const uint16_t DEL_DEVICE_NOTIFICATION = 0x0007;
    static const uint16_t DEVICE_NOTIFICATION = 0x0008;
    static const uint16_t READ_WRITE = 0x0009;

    AoEHeader()
        : leTargetPort(0),
        leSourcePort(0),
        leCmdId(0),
        leStateFlags(0),
        leLength(0),
        leErrorCode(0),
        leInvokeId(0)
    {}

    AoEHeader(const AmsNetId& __targetAddr,
              uint16_t        __targetPort,
              const AmsNetId& __sourceAddr,
              uint16_t        __sourcePort,
              uint16_t        __cmdId,
              uint32_t        __length,
              uint32_t        __invokeId)
        : targetNetId(__targetAddr),
        leTargetPort(bhf::ads::htole(__targetPort)),
        sourceNetId(__sourceAddr),
        leSourcePort(bhf::ads::htole(__sourcePort)),
        leCmdId(bhf::ads::htole(__cmdId)),
        leStateFlags(bhf::ads::htole(AMS_REQUEST)),
        leLength(bhf::ads::htole(__length)),
        leErrorCode(0),
        leInvokeId(bhf::ads::htole(__invokeId))
    {}

    AoEHeader(const uint8_t* frame)
    {
        memcpy(this, frame, sizeof(*this));
    }

    uint16_t cmdId() const
    {
        return bhf::ads::letoh(leCmdId);
    }

    uint32_t errorCode() const
    {
        return bhf::ads::letoh(leErrorCode);
    }

    uint32_t invokeId() const
    {
        return bhf::ads::letoh(leInvokeId);
    }

    uint32_t length() const
    {
        return bhf::ads::letoh(leLength);
    }

    AmsAddr sourceAms() const
    {
        return AmsAddr { sourceAddr(), sourcePort() };
    }

    AmsNetId sourceAddr() const
    {
        return sourceNetId;
    }

    uint16_t sourcePort() const
    {
        return bhf::ads::letoh(leSourcePort);
    }

    uint16_t stateFlags() const
    {
        return bhf::ads::letoh(leStateFlags);
    }

    AmsNetId targetAddr() const
    {
        return targetNetId;
    }

    uint16_t targetPort() const
    {
        return bhf::ads::letoh(leTargetPort);
    }

private:
    AmsNetId targetNetId;
    uint16_t leTargetPort;
    AmsNetId sourceNetId;
    uint16_t leSourcePort;
    uint16_t leCmdId;
    uint16_t leStateFlags;
    uint32_t leLength;
    uint32_t leErrorCode;
    uint32_t leInvokeId;
};

struct AoEResponseHeader {
    AoEResponseHeader()
        : leResult(0)
    {}

    AoEResponseHeader(const uint8_t* frame)
    {
        memcpy(this, frame, sizeof(*this));
    }

    uint32_t result() const
    {
        return bhf::ads::letoh(leResult);
    }
private:
    uint32_t leResult;
};

struct AoEReadResponseHeader : AoEResponseHeader {
    AoEReadResponseHeader()
        : leReadLength(0)
    {}

    AoEReadResponseHeader(const uint8_t* frame)
    {
        memcpy(this, frame, sizeof(*this));
    }

    uint32_t readLength() const
    {
        return bhf::ads::letoh(leReadLength);
    }
private:
    uint32_t leReadLength;
};
#pragma pack (pop)
