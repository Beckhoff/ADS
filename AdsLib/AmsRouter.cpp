#include "AmsRouter.h"
#include "AmsHeader.h"
#include "Frame.h"

#include <algorithm>

AmsRouter::AmsRouter()
	: localAddr({ { 192, 168, 0, 114, 1, 1 }, 0 }),
	running(true)
{
}

AmsRouter::~AmsRouter()
{
	for (uint16_t port = PORT_BASE; port < PORT_BASE + NUM_PORTS_MAX; ++port) {
		ClosePort(port);
	}
}

bool AmsRouter::AddRoute(AmsNetId ams, const IpV4& ip)
{
	std::lock_guard<std::mutex> lock(mutex);
	auto& route = mapping.find(ams);
	const auto& conn = connections.find(ip);

	if (route == mapping.end()) {
		if (conn == connections.end()) {
			// new route and connection
			connections.emplace(ip, std::unique_ptr<AdsConnection>(new AdsConnection{ ip }));
			mapping[ams] = connections[ip].get();
		}
		else {
			//new route to known ip
			mapping[ams] = conn->second.get();
		}
	}
	else {
		if (route->second->destIp == ip) {
			// route already exists
			return true;
		}
		auto oldConnection = route->second;
		if (conn == connections.end()) {
			connections.emplace(ip, std::unique_ptr<AdsConnection>(new AdsConnection{ ip }));
			route->second = connections[ip].get();
		}
		else {
			route->second = conn->second.get();
		}
		DeleteIfLastConnection(oldConnection);
	}
	return true;
}

void AmsRouter::DelRoute(const AmsNetId& ams)
{
	std::lock_guard<std::mutex> lock(mutex);
	auto route = mapping.find(ams);

	if (route != mapping.end()) {
		const AdsConnection* conn = route->second;
		mapping.erase(route);

		DeleteIfLastConnection(conn);
	}
}

void AmsRouter::DeleteIfLastConnection(const AdsConnection* conn)
{
	for (const auto& r : mapping) {
		if (r.second == conn) {
			return;
		}
	}
	connections.erase(conn->destIp);
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
	}
	return 0;
}


//TODO move into AdsConnection!!!
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

AdsConnection* AmsRouter::GetConnection(const AmsNetId& amsDest)
{
	auto it = __GetConnection(amsDest);
	if (it == connections.end()) {
		return nullptr;
	}
	return it->second.get();
}

std::map<IpV4, std::unique_ptr<AdsConnection>>::iterator AmsRouter::__GetConnection(const AmsNetId& amsDest)
{
	const auto it = mapping.find(amsDest);
	if (it != mapping.end()) {
		return connections.find(it->second->destIp);
	}
	return connections.end();
}

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

	auto ads = GetConnection(pAddr->netId);
	if (!ads) {
		return -1;
	}

	AdsResponse* response = ads->Write(request, *pAddr, srcAddr, AoEHeader::READ);

	if (response) {
		response->Wait();

		const auto header = response->frame.remove<AoEReadResponseHeader>();

		*bytesRead = std::min<uint32_t>(bufferLength, response->frame.size());
		memcpy(buffer, response->frame.data(), *bytesRead);
		ads->Release(response);
		return 0;
	}

	
	return -1;
}