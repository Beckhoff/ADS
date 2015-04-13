#ifndef _AMS_PORT_H_
#define _AMS_PORT_H_

#include "AdsDef.h"

#include <mutex>
#include <set>

struct VirtualConnection
{
	AmsAddr ams;
	uint16_t port;

	VirtualConnection(AmsAddr __ams, uint16_t __port)
		: ams(__ams),
		port(__port)
	{}

	bool operator<(const VirtualConnection& ref) const
	{
		if (port != ref.port) {
			return port < ref.port;
		}
		return ams < ref.ams;
	}
};

struct NotificationId
{
	NotificationId(AmsAddr __dest, uint16_t __port, uint32_t __hNotify)
		: connection(__dest, __port),
		hNotify(__hNotify)
	{}

	bool operator <(const NotificationId &ref) const
	{
		if (hNotify != ref.hNotify) {
			return hNotify < ref.hNotify;
		}
		return connection < ref.connection;
	}

	const VirtualConnection connection;
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