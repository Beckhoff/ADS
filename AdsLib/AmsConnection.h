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

struct NotificationDispatcher
{
	NotificationDispatcher()
		: ring(4*1024*1024),
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
// TODO implement this
#if 0
					const NotificationId hash{ amsAddr, port, hNotify };
					std::lock_guard<std::recursive_mutex> lock(notificationsLock);
					auto it = notifications.find(hash);
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
#endif
					{
						ring.Read(size);
					}
				}
			}
		}
	}


	RingBuffer ring;
	Semaphore sem;
private:
	std::thread thread;
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

	NotificationDispatcher dispatcher;
	
	inline RingBuffer& GetRing(uint16_t) { return dispatcher.ring; };

	std::map < NotificationId, Notification > notifications;
	std::recursive_mutex notificationsLock;
};

#endif /* #ifndef _AMSCONNECTION_H_ */
