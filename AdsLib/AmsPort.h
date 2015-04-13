#ifndef _AMS_PORT_H_
#define _AMS_PORT_H_

#include "AdsDef.h"

#include <mutex>
#include <set>

struct NotificationId
{
	NotificationId(AmsAddr __dest, uint16_t __port, uint32_t __hNotify)
		: dest(__dest),
		port(__port),
		hNotify(__hNotify)
	{}

	bool operator <(const NotificationId &ref) const
	{
		if (hNotify != ref.hNotify) {
			return hNotify < ref.hNotify;
		}
		if (port != ref.port) {
			return port < ref.port;
		}
		return dest < ref.dest;
	}

	const AmsAddr dest;
	const uint16_t port;
	const uint32_t hNotify;
};

struct AmsPort
{
	AmsPort();
	void Close();
	bool IsOpen() const;
	uint16_t Open(uint16_t __port);
	uint32_t tmms;
	uint16_t port;

	void AddNotification(NotificationId hash);
	void DelNotification(NotificationId hash);
	const std::set<NotificationId>& GetNotifications() const;
private:
	static const uint32_t DEFAULT_TIMEOUT = 5000;
	std::set<NotificationId> notifications;
	std::mutex mutex;
};
#endif /* #ifndef _AMS_PORT_H_ */