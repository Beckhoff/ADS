#ifndef _AMSCONNECTION_H_
#define _AMSCONNECTION_H_

#include "AdsNotification.h"
#include "AmsHeader.h"
#include "AmsPort.h"
#include "Semaphore.h"
#include "Sockets.h"
#include "Router.h"

#include <array>
#include <functional>
#include <list>
#include <map>
#include <thread>
#include <vector>

struct AmsResponse
{
	Frame frame;
	uint32_t invokeId;
	uint32_t extra;

	AmsResponse();
	void Notify();

	// return true if notified before timeout
	bool Wait(uint32_t timeout_ms);
private:
	std::mutex mutex;
	std::condition_variable cv;
};

#include "Log.h"
struct NotificationDispatcher
{
	NotificationDispatcher(AmsAddr __amsAddr, uint16_t __port)
		: ring(4*1024*1024),
		amsAddr(__amsAddr),
		port(__port),
		thread(&NotificationDispatcher::Run, this)
	{

	}

	~NotificationDispatcher()
	{
		sem.Stop();
		thread.join();
	}

	void Run()
	{
		while (sem.Wait()) {
			const auto length = ring.ReadFromLittleEndian<uint32_t>();
			const auto numStamps = ring.ReadFromLittleEndian<uint32_t>();
			for (uint32_t stamp = 0; stamp < numStamps; ++stamp) {
				const auto timestamp = ring.ReadFromLittleEndian<uint64_t>();
				const auto numSamples = ring.ReadFromLittleEndian<uint32_t>();
				for (uint32_t sample = 0; sample < numSamples; ++sample) {
					const auto hNotify = ring.ReadFromLittleEndian<uint32_t>();
					const auto size = ring.ReadFromLittleEndian<uint32_t>();
					std::lock_guard<std::recursive_mutex> lock(notificationsLock);
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
	Semaphore sem;
	std::map<uint32_t, Notification> notifications;
	std::recursive_mutex notificationsLock;
private:
	const AmsAddr amsAddr;
	const uint16_t port;
	std::thread thread;
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

struct AmsConnection
{
	AmsConnection(Router &__router, IpV4 destIp = IpV4{ "" });
	~AmsConnection();

	NotificationId CreateNotifyMapping(uint16_t port, AmsAddr destAddr, PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t length, uint32_t hNotify);
	bool DeleteNotifyMapping(NotificationId hash);
	void DeleteOrphanedNotifications(AmsPort & port);
	long __DeleteNotification(const AmsAddr &amsAddr, uint32_t hNotify, const AmsPort &port);

	AmsResponse* Write(Frame& request, const AmsAddr dest, const AmsAddr srcAddr, uint16_t cmdId, uint32_t extra = 0);
	void Release(AmsResponse* response);
	AmsResponse* GetPending(uint32_t id, uint16_t port);

	template <class T>
	long AdsRequest(Frame& request, const AmsAddr& destAddr, const AmsPort& port, uint16_t cmdId, uint32_t extra = 0, uint32_t bufferLength = 0, void* buffer = nullptr, uint32_t *bytesRead = nullptr)
	{
		AmsAddr srcAddr;
		const auto status = router.GetLocalAddress(port.port, &srcAddr);
		if (status) {
			return status;
		}
		AmsResponse* response = Write(request, destAddr, srcAddr, cmdId, extra);
		if (response) {
			if (response->Wait(port.tmms)){
				const uint32_t bytesAvailable = std::min<uint32_t>(bufferLength, response->frame.size() - sizeof(T));
				T header(response->frame.data());
				memcpy(buffer, response->frame.data() + sizeof(T), bytesAvailable);
				if (bytesRead) {
					*bytesRead = bytesAvailable;
				}
				Release(response);
				return header.result();
			}
			Release(response);
			return ADSERR_CLIENT_SYNCTIMEOUT;
		}
		return -1;
	}

	const IpV4 destIp;
private:
	Router &router;
	TcpSocket socket;
	uint32_t invokeId;
	std::thread receiver;
	std::array<AmsResponse, Router::NUM_PORTS_MAX> queue;

	void ReceiveJunk(size_t bytesToRead) const;
	void Receive(uint8_t* buffer, size_t bytesToRead) const;
	void Recv();
	void TryRecv();

	template<class T> T Receive() const;
	bool ReceiveNotification(const AoEHeader& header);
	Frame& ReceiveFrame(Frame &frame, size_t length) const;
	AmsResponse* Reserve(uint32_t id, uint16_t port);

	std::map<VirtualConnection, std::unique_ptr<NotificationDispatcher>> dispatcher;
	std::recursive_mutex notificationsLock;
};

#endif /* #ifndef _AMSCONNECTION_H_ */
