#ifndef _ADSCONNECTION_H_
#define _ADSCONNECTION_H_

#include "AmsHeader.h"
#include "Sockets.h"
#include "NotificationDispatcher.h"

#include <array>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>



struct AdsResponse
{
	Frame frame;
	uint32_t invokeId;

	AdsResponse();
	void Notify();

	// return true if notified before timeout
	bool Wait(uint32_t timeout_ms);
private:
	std::mutex mutex;
	std::condition_variable cv;
};

struct AdsConnection
{
	AdsConnection(NotificationDispatcher &__dispatcher, IpV4 destIp = IpV4{ "" });
	~AdsConnection();

	AdsResponse* Write(Frame& request, const AmsAddr dest, const AmsAddr srcAddr, uint16_t cmdId);
	void Release(AdsResponse* response);



	
	const IpV4 destIp;
private:
	NotificationDispatcher &dispatcher;
	TcpSocket socket;
	std::mutex mutex;
	uint32_t invokeId;
	std::thread receiver;
	bool running = true;

	std::array<AdsResponse, 16> responses;
	std::list<AdsResponse*> ready;
	std::list<AdsResponse*> pending;

	void ReadJunk(size_t bytesToRead) const;
	bool Read(uint8_t* buffer, size_t bytesToRead) const;
	void Recv();
	void TryRecv();

	template<class T> T Receive() const;
	Frame& ReceiveFrame(Frame &frame, size_t length) const;
	AdsResponse* GetPending(uint32_t id);
};

#endif /* #ifndef _ADSCONNECTION_H_ */