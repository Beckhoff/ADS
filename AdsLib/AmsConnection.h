#ifndef _AMSCONNECTION_H_
#define _AMSCONNECTION_H_

#include "AdsNotification.h"
#include "AmsHeader.h"
#include "AmsPort.h"
#include "Sockets.h"
#include "Router.h"

#include <array>
#include <condition_variable>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

struct AmsResponse
{
	Frame frame;
	uint32_t invokeId;

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

	void CreateNotifyMapping(uint16_t port, AmsAddr destAddr, PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t length, uint32_t hNotify);
	bool DeleteNotifyMapping(const AmsAddr &addr, uint32_t hNotify, uint16_t port);
	void DeleteOrphanedNotifications(const AmsPort & port);
	long __DeleteNotification(const AmsAddr &amsAddr, uint32_t hNotify, const AmsPort &port);

	AmsResponse* Write(Frame& request, const AmsAddr dest, const AmsAddr srcAddr, uint16_t cmdId);
	void Release(AmsResponse* response);
	AmsResponse* GetPending(uint32_t id, uint16_t port);

	template <class T>
	long AdsRequest(Frame& request, const AmsAddr& destAddr, const AmsPort& port, uint16_t cmdId, uint32_t bufferLength = 0, void* buffer = nullptr, uint32_t *bytesRead = nullptr)
	{
		AmsAddr srcAddr;
		const auto status = port.GetLocalAddress(&srcAddr);
		if (status) {
			return status;
		}
		AmsResponse* response = Write(request, destAddr, srcAddr, cmdId);
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

	using NotifyTable = std::map < uint32_t, Notification >;
	using TableRef = std::unique_ptr<NotifyTable>;
	std::map<AmsAddr, TableRef> tableMapping[Router::NUM_PORTS_MAX];
	std::array<std::mutex, Router::NUM_PORTS_MAX> notificationLock;

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
	RingBuffer& GetRing(uint16_t port) { return ringBuffer; };

	const std::map<uint32_t, Notification>* GetNotifyTable(const AmsAddr& amsAddr, uint16_t port);
};

#endif /* #ifndef _AMSCONNECTION_H_ */
