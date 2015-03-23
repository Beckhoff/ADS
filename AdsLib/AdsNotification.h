#ifndef _ADS_NOTIFICATION_H_
#define _ADS_NOTIFICATION_H_

#include "AdsDef.h"
#include <memory>

struct Notification
{
	uint16_t port;

	Notification(PAdsNotificationFuncEx __func, uint32_t hNotify, uint32_t __hUser, uint32_t length, AmsAddr __amsAddr, uint16_t __port)
		: callback(__func),
		hUser(__hUser),
		port(__port),
		buffer(new uint8_t[sizeof(AdsNotificationHeader) + length]),
		amsAddr(__amsAddr)
	{
		auto header = reinterpret_cast<AdsNotificationHeader*>(buffer.get());
		header->hNotification = hNotify;
		header->cbSampleSize = length;
	}

	void Notify(uint64_t timestamp, const uint8_t* data)
	{
		auto header = reinterpret_cast<AdsNotificationHeader*>(buffer.get());
		memcpy(header->data, data, header->cbSampleSize);
		header->nTimeStamp = timestamp;
		callback(&amsAddr, header, hUser);
	}

	uint32_t Size() const
	{
		auto header = reinterpret_cast<AdsNotificationHeader*>(buffer.get());
		return header->cbSampleSize;
	}

private:
	PAdsNotificationFuncEx callback;
	std::shared_ptr<uint8_t> buffer;
	uint32_t hUser;
	const AmsAddr amsAddr;
};

#endif /* #ifndef _ADS_NOTIFICATION_H_ */
