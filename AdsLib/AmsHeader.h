#ifndef AMSHEADER_H
#define AMSHEADER_H

#include "NetId.h"
#include "wrap_endian.h"

#include "AdsDef.h"
#include <cstddef>

#pragma pack (push, 1)
struct AmsTcpHeader
{
	AmsTcpHeader(const uint32_t length = 0)
	{
		*((uint16_t*)(&buffer[0])) = 0;
		*((uint32_t*)(&buffer[2])) = qToLittleEndian<uint32_t>(length);
	}

	AmsTcpHeader(const uint8_t *frame)
	{
		memcpy(buffer, frame, sizeof(buffer));
	}

	uint32_t length() const
	{
		return qFromLittleEndian<uint32_t>(buffer + sizeof(uint16_t));
	}

private:
	uint8_t buffer[sizeof(uint16_t) + sizeof(uint32_t)];
};

struct AoERequestHeader
{
	static const uint32_t SDO_UPLOAD = 0xF302;

	uint32_t group;
	uint32_t offset;
	uint32_t length;

	AoERequestHeader(uint16_t sdoIndex, uint8_t sdoSubIndex, uint32_t dataLength)
		: group(SDO_UPLOAD),
		offset(((uint32_t)sdoIndex) << 16 | sdoSubIndex),
		length(dataLength)
	{
	}

	AoERequestHeader(uint32_t indexGroup, uint32_t indexOffset, uint32_t dataLength)
		: group(indexGroup),
		offset(indexOffset),
		length(dataLength)
	{
	}
};

struct AdsWriteCtrlRequest
{
	uint16_t adsState;
	uint16_t devState;
	uint32_t length;

	AdsWriteCtrlRequest(uint16_t ads, uint16_t dev, uint32_t dataLength)
		: adsState(qToLittleEndian(ads)),
		devState(qToLittleEndian(dev)),
		length(qToLittleEndian(dataLength))
	{
	}
};

struct AdsAddDeviceNotificationRequest
{
	uint32_t group;
	uint32_t offset;
	uint32_t length;
	uint32_t mode;
	uint32_t maxDelay;
	uint32_t cycleTime;
	const uint8_t reserved[16];

	AdsAddDeviceNotificationRequest(uint32_t __group, uint32_t __offset, uint32_t __length, uint32_t __mode, uint32_t __maxDelay, uint32_t __cycleTime)
		: group(__group),
		offset(__offset),
		length(__length),
		mode(__mode),
		maxDelay(__maxDelay),
		cycleTime(__cycleTime),
		reserved()
	{}
};

using AdsDelDeviceNotificationRequest = uint32_t;

struct AoEHeader
{
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

    AmsAddr targetAddr;
    AmsAddr sourceAddr;
    const uint16_t cmdId;
    const uint16_t stateFlags;
    const uint32_t length;
    const uint32_t errorCode;
	const uint32_t invokeId;

	AoEHeader()
		: targetAddr({ { 0, 0, 0, 0, 0, 0 }, 0 }),
		sourceAddr({ { 0, 0, 0, 0, 0, 0 }, 0 }),
		cmdId(0),
		stateFlags(0),
		length(0),
		errorCode(0),
		invokeId(0)
	{
	}

	AoEHeader(const AmsAddr &__targetAddr, const AmsAddr &__sourceAddr, uint16_t __cmdId, uint32_t __length, uint32_t __invokeId)
		: targetAddr(__targetAddr),
		sourceAddr(__sourceAddr),
		cmdId(__cmdId),
		stateFlags(AMS_REQUEST),
		length(__length),
		errorCode(0),
		invokeId(__invokeId)
	{
	}

    AoEHeader(const uint8_t *frame)
        : targetAddr(frame),
          sourceAddr(frame + offsetof(AoEHeader, sourceAddr)),
          cmdId     (qFromLittleEndian<uint16_t>(frame + offsetof(AoEHeader, cmdId))),
          stateFlags(qFromLittleEndian<uint16_t>(frame + offsetof(AoEHeader, stateFlags))),
          length    (qFromLittleEndian<uint32_t>(frame + offsetof(AoEHeader, length))),
          errorCode (qFromLittleEndian<uint32_t>(frame + offsetof(AoEHeader, errorCode))),
          invokeId  (qFromLittleEndian<uint32_t>(frame + offsetof(AoEHeader, invokeId)))
    {
		memcpy(&targetAddr, frame, sizeof(targetAddr));
		memcpy(&targetAddr, frame, sizeof(targetAddr));
    }
};

using AoEWriteResponseHeader = uint32_t;

struct AoEResponseHeader
{
	uint32_t result;
	AoEResponseHeader()
		: result(0)
	{}

	AoEResponseHeader(const uint8_t *frame)
		: result(qFromLittleEndian<uint32_t>(frame))
	{}
};

struct AoEAddNotificationResponseHeader : AoEResponseHeader
{
	uint32_t hNotify;
	AoEAddNotificationResponseHeader()
		: hNotify(0)
	{}

	AoEAddNotificationResponseHeader(const uint8_t *frame)
		: AoEResponseHeader(frame),
		hNotify(qFromLittleEndian<uint32_t>(frame + sizeof(AoEResponseHeader)))
	{}
};

struct AoEReadResponseHeader
{
    const uint32_t result;
    const uint32_t readLength;

	AoEReadResponseHeader()
		: result(0),
		readLength(0)
	{}

    AoEReadResponseHeader(const uint8_t *frame)
        : result(qFromLittleEndian<uint32_t>(frame)),
          readLength(qFromLittleEndian<uint32_t>(frame + sizeof(result)))
	{}
};
#pragma pack (pop)
#endif // AMSHEADER_H
