#ifndef _NOTIFICATION_DISPATCHER_H_
#define _NOTIFICATION_DISPATCHER_H_

#include "AdsNotification.h"
#include "AmsHeader.h"
#include "Log.h"
#include "Semaphore.h"

#include <map>
#include <thread>

struct NotificationDispatcher;

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

struct NotificationId
{
	NotificationId(uint32_t __hNotify, std::shared_ptr<NotificationDispatcher> __dispatcher)
		: hNotify(__hNotify),
		dispatcher(__dispatcher)
	{}

	bool operator <(const NotificationId &ref) const
	{
		return hNotify < ref.hNotify;
	}

	const uint32_t hNotify;
	const std::shared_ptr<NotificationDispatcher> dispatcher;
};

struct NotificationDispatcher
{
	NotificationDispatcher(AmsProxy &__proxy, AmsAddr __amsAddr, uint16_t __port)
		: ring(4 * 1024 * 1024),
		amsAddr(__amsAddr),
		port(__port),
		thread(&NotificationDispatcher::Run, this),
		proxy(__proxy)
	{}

	~NotificationDispatcher()
	{
		sem.Stop();
		thread.join();
	}

	NotificationId Emplace(PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t length, uint32_t hNotify, std::shared_ptr<NotificationDispatcher> dispatcher)
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);
		notifications.emplace(hNotify, Notification{ pFunc, hNotify, hUser, length, amsAddr, port });
		return NotificationId{ hNotify, dispatcher };
	}

	bool Erase(uint32_t hNotify, uint32_t tmms)
	{
		const auto status = proxy.__DeleteNotification(amsAddr, hNotify, tmms, port);
		std::lock_guard<std::recursive_mutex> lock(mutex);
		return !!notifications.erase(hNotify) && status;
	}

	inline void Notify() { sem.Post(); }

	void Run()
	{
		while (sem.Wait()) {
			const auto length = ring.ReadFromLittleEndian<uint32_t>();
			(void)length;
			const auto numStamps = ring.ReadFromLittleEndian<uint32_t>();
			for (uint32_t stamp = 0; stamp < numStamps; ++stamp) {
				const auto timestamp = ring.ReadFromLittleEndian<uint64_t>();
				const auto numSamples = ring.ReadFromLittleEndian<uint32_t>();
				for (uint32_t sample = 0; sample < numSamples; ++sample) {
					const auto hNotify = ring.ReadFromLittleEndian<uint32_t>();
					const auto size = ring.ReadFromLittleEndian<uint32_t>();
					std::lock_guard<std::recursive_mutex> lock(mutex);
					auto it = notifications.find(hNotify);
					if (it != notifications.end()) {
						auto &notification = it->second;
						if (size != notification.Size()) {
							LOG_WARN("Notification sample size: " << size << " doesn't match: " << notification.Size());
							ring.Read(size);
							return;
						}
						notification.Notify(timestamp, ring);
					}
					else {
						ring.Read(size);
					}
				}
			}
		}
	}

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
#endif /* #ifndef _NOTIFICATION_DISPATCHER_H_ */