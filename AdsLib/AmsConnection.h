#ifndef _AMSCONNECTION_H_
#define _AMSCONNECTION_H_

#include "NotificationDispatcher.h"
#include "Sockets.h"
#include "Router.h"

#include <array>
#include <functional>
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

struct DispatcherList
{
	std::shared_ptr<NotificationDispatcher> Add(const VirtualConnection& connection)
	{
		const auto dispatcher = Get(connection);
		if (dispatcher) {
			return dispatcher;
		}
		return list.emplace(connection, std::make_shared<NotificationDispatcher>(connection.ams, connection.port)).first->second;
	}

	std::shared_ptr<NotificationDispatcher> Get(const VirtualConnection &connection)
	{
		std::lock_guard<std::recursive_mutex> lock(mutex);

		const auto it = list.find(connection);
		if (it != list.end()) {
			return it->second;
		}
		return std::shared_ptr < NotificationDispatcher > {};
	}

	std::map<VirtualConnection, std::shared_ptr<NotificationDispatcher>> list;
	std::recursive_mutex mutex;
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
	DispatcherList dispatcherList;
};

#endif /* #ifndef _AMSCONNECTION_H_ */
