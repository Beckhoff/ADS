#include "AmsRouter.h"
#include "AmsHeader.h"
#include "Frame.h"

AmsRouter::AmsRouter()
	: localAddr({ { 192, 168, 0, 114, 1, 1 }, 0 }),
	connection(AmsAddr({ { 192, 168, 0, 114, 1, 1 }, 0 }), IpV4("192.168.0.232"), 48898),
	running(true)
{
}

AmsRouter::~AmsRouter()
{
	for (uint16_t port = PORT_BASE; port < PORT_BASE + NUM_PORTS_MAX; ++port) {
		ClosePort(port);
	}
}

uint16_t AmsRouter::OpenPort()
{
	std::lock_guard<std::mutex> lock(mutex);

	if (ports.all()) {
		return 0; // we can't pass ROUTERERR_NOMOREQUEUES due to API limitation...
	}

	for (uint16_t i = 0; i < ports.size(); ++i) {
		if (!ports[i]) {
			ports.set(i);
			return PORT_BASE + i;
		}
	}
	return 0;
}

long AmsRouter::ClosePort(uint16_t port)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (port < PORT_BASE || port >= PORT_BASE + NUM_PORTS_MAX) {
		return ROUTERERR_NOTREGISTERED;
	}
	ports.reset(port - PORT_BASE);

	if (ports.none()) {
		running = false;
		//worker.join();
	}
	return 0;
}

long AmsRouter::GetLocalAddress(uint16_t port, AmsAddr* pAddr)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (port < PORT_BASE || port >= PORT_BASE + NUM_PORTS_MAX) {
		return ROUTERERR_NOTREGISTERED;
	}

	if (ports.test(port - PORT_BASE)) {
		memcpy(&pAddr->netId, &localAddr.netId, sizeof(localAddr.netId));
		pAddr->port = port;
		return 0;
	}
	return ROUTERERR_NOTREGISTERED;
}

AdsConnection& AmsRouter::GetConnection(const AmsAddr& pAddr)
{
	return connection;
}


#include <iostream>
long AmsRouter::Read(uint16_t port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, void* buffer, uint32_t *bytesRead)
{
	AmsAddr srcAddr;
	const auto status = GetLocalAddress(port, &srcAddr);
	if (status) {
		return status;
	}

	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AoERequestHeader));
	AoERequestHeader readReq{ indexGroup, indexOffset, bufferLength };
	request.prepend<AoERequestHeader>(readReq);
	
	AoEHeader aoeHeader{ *pAddr, srcAddr, AoEHeader::READ, request.size(), 1 };
	request.prepend<AoEHeader>(aoeHeader);

	AdsConnection& ads = GetConnection(*pAddr);

	AdsResponse* response = ads.Write(request);

	if (response) {
		response->Wait();

		std::cout << "response locked->frame received" << std::endl;
		return 0;
	}

	
	return -1;
}