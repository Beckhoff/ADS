#ifndef _NOTIFICATION_DISPATCHER_H_
#define _NOTIFICATION_DISPATCHER_H_

#include "AdsNotification.h"
#include "AmsHeader.h"
#include "Semaphore.h"

#include <map>
#include <thread>

struct AmsProxy
{
	virtual long DeleteNotification(const AmsAddr &amsAddr, uint32_t hNotify, uint32_t tmms, uint16_t port) = 0;
};

struct NotificationDispatcher
{
	NotificationDispatcher(AmsProxy &__proxy, AmsAddr __amsAddr, uint16_t __port);
	~NotificationDispatcher();
	bool operator<(const NotificationDispatcher &ref) const;
	void Emplace(PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t length, uint32_t hNotify);
	bool Erase(uint32_t hNotify, uint32_t tmms);
	inline void Notify() { sem.Post(); }
	void Run();

	const AmsAddr amsAddr;
	RingBuffer ring;
private:
	std::map<uint32_t, Notification> notifications;
	std::recursive_mutex mutex;
	const uint16_t port;
	AmsProxy &proxy;
	Semaphore sem;
	std::thread thread;
};

#endif /* #ifndef _NOTIFICATION_DISPATCHER_H_ */