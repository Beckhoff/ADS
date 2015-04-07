#ifndef _AMSCONNECTION_H_
#define _AMSCONNECTION_H_

#include "AmsHeader.h"
#include "Sockets.h"
#include "Router.h"

#include <array>
#include <condition_variable>
#include <list>
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

	AmsResponse* Write(Frame& request, const AmsAddr dest, const AmsAddr srcAddr, uint16_t cmdId);
	void Release(AmsResponse* response);
	AmsResponse* GetPending(uint32_t id, uint16_t port);

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
	bool ReceiveNotification(const AoEHeader& header) const;
	Frame& ReceiveFrame(Frame &frame, size_t length) const;
	AmsResponse* Reserve(uint32_t id, uint16_t port);
};

#endif /* #ifndef _AMSCONNECTION_H_ */
