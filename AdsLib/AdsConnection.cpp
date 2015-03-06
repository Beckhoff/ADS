#include "AdsConnection.h"
#include "AmsHeader.h"
#include <iostream>
AdsResponse::AdsResponse()
	: frame(2048),
	invokeId(0)
{
}

void AdsResponse::Notify()
{
	std::unique_lock<std::mutex> lock(mutex);
	cv.notify_one();
}

void AdsResponse::Wait()
{
	std::unique_lock<std::mutex> lock(mutex);
	cv.wait(lock);
}

AdsConnection::AdsConnection(AmsAddr destination, IpV4 destIp, uint16_t destPort)
	: destAddr(destination),
	socket(destIp, destPort),
	invokeId(0)
{
	for (auto& r : responses) {
		ready.push_back(&r);
	}
	socket.Connect();

	receiver = std::thread(&AdsConnection::Recv, this);
}


AdsConnection::~AdsConnection()
{
	running = false;
	receiver.detach();
}

AdsResponse* AdsConnection::Write(Frame& request)
{
	AmsTcpHeader header{ request.size() };
	request.prepend<AmsTcpHeader>(header);

	std::lock_guard<std::mutex> lock(mutex);

	if (ready.empty()) {
		return nullptr;
	}

	auto response = ready.back();
	ready.pop_back();
	
	response->invokeId = ++invokeId;
	
	if (request.size() != socket.write(request)) {
		return nullptr;
	}

	pending.push_back(response);
	return response;
}



void AdsConnection::Recv()
{
	Frame frame(2048);

	while (running) {
		socket.read(frame);
		if (frame.size() > 0) {
			auto response = pending.back();
			pending.pop_back();

			//TODO avoid memcpy
			response->frame.prepend(frame.data(), frame.size());
			response->Notify();
		}
	}
}
