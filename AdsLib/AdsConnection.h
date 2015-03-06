#ifndef _ADSCONNECTION_H_
#define _ADSCONNECTION_H_

#include "AdsDef.h"
#include "Frame.h"
#include "Sockets.h"

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
	void Wait();
private:
	std::mutex mutex;
	std::condition_variable cv;
};

struct AdsConnection
{
	AdsConnection(AmsAddr destination, IpV4 destIp, uint16_t destPort);
	~AdsConnection();

	AdsResponse* Write(Frame& request, const AmsAddr dest, const AmsAddr srcAddr, uint16_t cmdId);
	void Release(AdsResponse* response);

private:
	const AmsAddr destAddr;
	TcpSocket socket;
	std::mutex mutex;
	uint32_t invokeId;
	std::thread receiver;
	bool running = true;

	std::array<AdsResponse, 16> responses;
	std::list<AdsResponse*> ready;
	std::list<AdsResponse*> pending;

	void Recv();
	AdsResponse* GetPending(uint32_t id);
};

#endif /* #ifndef _ADSCONNECTION_H_ */