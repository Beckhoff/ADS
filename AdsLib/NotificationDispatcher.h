#ifndef _NOTIFICATION_DISPATCHER_H_
#define _NOTIFICATION_DISPATCHER_H_

#include "AdsNotification.h"
#include "AmsHeader.h"
#include "Log.h"
#include "Semaphore.h"

#include <map>
#include <thread>

struct NotificationId;

struct AmsProxy
{
	virtual long __DeleteNotification(const AmsAddr &amsAddr, uint32_t hNotify, uint32_t tmms, uint16_t port) = 0;
};

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

struct NotificationDispatcher
{
	NotificationDispatcher(AmsProxy &__proxy, AmsAddr __amsAddr, uint16_t __port);

	~NotificationDispatcher();
	NotificationId Emplace(PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t length, uint32_t hNotify, std::shared_ptr<NotificationDispatcher> dispatcher);
	bool Erase(uint32_t hNotify, uint32_t tmms);
	inline void Notify() { sem.Post(); }
	void Run();

	RingBuffer ring;
	const AmsAddr amsAddr;
private:
	std::map<uint32_t, Notification> notifications;
	std::recursive_mutex mutex;
	const uint16_t port;
	Semaphore sem;
	std::thread thread;
	AmsProxy &proxy;
};

struct NotificationId
{
	NotificationId(uint32_t __hNotify, std::shared_ptr<NotificationDispatcher> __dispatcher)
		: hNotify(__hNotify),
		dispatcher(__dispatcher)
	{}

	bool operator <(const NotificationId &ref) const
	{
		if (hNotify != ref.hNotify) {
			return hNotify < ref.hNotify;
		}
		return dispatcher->amsAddr < ref.dispatcher->amsAddr;
	}

	const uint32_t hNotify;
	const std::shared_ptr<NotificationDispatcher> dispatcher;
};
#endif /* #ifndef _NOTIFICATION_DISPATCHER_H_ */