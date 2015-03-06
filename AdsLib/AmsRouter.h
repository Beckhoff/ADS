#ifndef _AMS_ROUTER_H_
#define _AMS_ROUTER_H_

#include "AdsDef.h"
#include "AdsConnection.h"

#include <bitset>
#include <map>
#include <mutex>
#include <thread>

struct AmsRouter
{
	AmsRouter();
	~AmsRouter();

	uint16_t OpenPort();
	long ClosePort(uint16_t port);
	long GetLocalAddress(uint16_t port, AmsAddr* pAddr);
	long Read(uint16_t port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, void* buffer, uint32_t *bytesRead);

	bool AddRoute(AmsAddr ams, const IpV4& ip);

private:
	static const size_t NUM_PORTS_MAX = 8;
	static const uint16_t PORT_BASE = 30000;
	static_assert(PORT_BASE + NUM_PORTS_MAX <= UINT16_MAX, "Port limit is out of range");

	std::bitset<NUM_PORTS_MAX> ports;
	const AmsAddr localAddr;
	std::mutex mutex;
	std::map<IpV4, std::unique_ptr<AdsConnection>> connections;
	std::map<AmsAddr, AdsConnection*> mapping;
	std::thread worker;
	bool running;

	std::map<IpV4, std::unique_ptr<AdsConnection>>::iterator GetConnection(const AmsAddr& pAddr);
	void Recv();

};
#endif /* #ifndef _AMS_ROUTER_H_ */
