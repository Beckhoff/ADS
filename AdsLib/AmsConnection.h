#ifndef _AMSCONNECTION_H_
#define _AMSCONNECTION_H_

#include "AdsNotification.h"
#include "AmsHeader.h"
#include "AmsPort.h"
#include "Sockets.h"
#include "Router.h"

#include <array>
#include <condition_variable>
#include <functional>
#include <list>
#include <map>
#include <mutex>
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

struct AmsConnection
{
	AmsConnection(Router &__router, IpV4 destIp = IpV4{ "" });
	~AmsConnection();

	size_t CreateNotifyMapping(uint16_t port, AmsAddr destAddr, PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t length, uint32_t hNotify);
	bool DeleteNotifyMapping(size_t hash);
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
	static size_t Hash(uint32_t hNotify, AmsAddr srcAddr, uint16_t port);

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

	void Dispatch(AmsAddr amsAddr, uint16_t port, size_t expectedSize);
	
	RingBuffer ringBuffer;
	inline RingBuffer& GetRing(uint16_t) { return ringBuffer; };

	std::map < size_t, Notification > notifications;
	std::recursive_mutex notificationsLock;
};

#endif /* #ifndef _AMSCONNECTION_H_ */
