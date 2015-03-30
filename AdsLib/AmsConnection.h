#ifndef _AMSCONNECTION_H_
#define _AMSCONNECTION_H_

#include "AmsHeader.h"
#include "Sockets.h"
#include "NotificationDispatcher.h"

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
	AmsConnection(NotificationDispatcher &__dispatcher, IpV4 destIp = IpV4{ "" });
	~AmsConnection();

	AmsResponse* Write(Frame& request, const AmsAddr dest, const AmsAddr srcAddr, uint16_t cmdId);
	void Release(AmsResponse* response);
	AmsResponse* GetPending(uint32_t id);

	const IpV4 destIp;
private:
	static const size_t RESPONSE_Q_LENGTH = 128;
	NotificationDispatcher &dispatcher;
	TcpSocket socket;
	std::mutex mutex;
	std::mutex pendingMutex;
	uint32_t invokeId;
	std::thread receiver;
	bool running = true;

	std::array<AmsResponse, RESPONSE_Q_LENGTH> responses;
	std::vector<AmsResponse*> ready;
	std::list<AmsResponse*> pending;

	void ReadJunk(size_t bytesToRead) const;
	bool Read(uint8_t* buffer, size_t bytesToRead) const;
	void Recv();
	void TryRecv();

	template<class T> T Receive() const;
	Frame& ReceiveFrame(Frame &frame, size_t length) const;
	AmsResponse* Reserve(uint32_t id);
};

#endif /* #ifndef _AMSCONNECTION_H_ */