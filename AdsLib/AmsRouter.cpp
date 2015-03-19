#include "AmsRouter.h"
#include "AmsHeader.h"
#include "Frame.h"
#include "Log.h"

#include <algorithm>

AmsRouter::AmsRouter()
#ifdef WIN32
	: localAddr({ { 192, 168, 0, 114, 1, 1 }, 0 })
#else
	: localAddr({ { 192, 168, 0, 164, 1, 1 }, 0 })
#endif
{
	for (auto& t : portTimeout) {
		t = DEFAULT_TIMEOUT;
	}
}

bool AmsRouter::AddRoute(AmsNetId ams, const IpV4& ip)
{
	std::lock_guard<std::mutex> lock(mutex);
	auto route = mapping.find(ams);
	const auto& conn = connections.find(ip);

	if (route == mapping.end()) {
		if (conn == connections.end()) {
			// new route and connection
			connections.emplace(ip, std::unique_ptr<AdsConnection>(new AdsConnection{ *this, ip }));
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
			connections.emplace(ip, std::unique_ptr<AdsConnection>(new AdsConnection{ *this, ip }));
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
	for (const auto &n : CollectOrphanedNotifies(port)) {
		DelNotification(port, &n.first, n.second);
	}

	std::lock_guard<std::mutex> lock(mutex);
	if (port < PORT_BASE || port >= PORT_BASE + NUM_PORTS_MAX) {
		return ADSERR_CLIENT_PORTNOTOPEN;
	}
	ports.reset(port - PORT_BASE);
	return 0;
}

//TODO move into AdsConnection!!!
long AmsRouter::GetLocalAddress(uint16_t port, AmsAddr* pAddr)
{
	std::lock_guard<std::mutex> lock(mutex);
	if (port < PORT_BASE || port >= PORT_BASE + NUM_PORTS_MAX) {
		return ADSERR_CLIENT_PORTNOTOPEN;
	}

	if (ports.test(port - PORT_BASE)) {
		memcpy(&pAddr->netId, &localAddr.netId, sizeof(localAddr.netId));
		pAddr->port = port;
		return 0;
	}
	return ADSERR_CLIENT_PORTNOTOPEN;
}

long AmsRouter::GetTimeout(uint16_t port, uint32_t& timeout)
{
	std::lock_guard<std::mutex> lock(mutex);
	if (port < PORT_BASE || port >= PORT_BASE + NUM_PORTS_MAX) {
		return ADSERR_CLIENT_PORTNOTOPEN;
	}

	timeout = portTimeout[port - PORT_BASE];
	return 0;
}

long AmsRouter::SetTimeout(uint16_t port, uint32_t timeout)
{
	std::lock_guard<std::mutex> lock(mutex);
	if (port < PORT_BASE || port >= PORT_BASE + NUM_PORTS_MAX) {
		return ADSERR_CLIENT_PORTNOTOPEN;
	}

	portTimeout[port - PORT_BASE] = timeout;
	return 0;
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
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AoERequestHeader));
	AoERequestHeader readReq{ indexGroup, indexOffset, bufferLength };
	request.prepend<AoERequestHeader>(readReq);
	return AdsRequest(request, *pAddr, port, AoEHeader::READ, bufferLength, buffer, bytesRead);
}

long AmsRouter::ReadDeviceInfo(uint16_t port, const AmsAddr* pAddr, char* devName, AdsVersion* version)
{
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader));
	uint8_t buffer[sizeof(*version) + 16];
	uint32_t bytesRead;

	const long status = AdsRequest(request, *pAddr, port, AoEHeader::READ_DEVICE_INFO, sizeof(buffer), buffer, &bytesRead);
	if (status) {
		return status;
	}

	version->version = buffer[0];
	version->revision = buffer[1];
	version->build = qFromLittleEndian<uint16_t>(buffer + 2);
	memcpy(devName, buffer + sizeof(*version), sizeof(buffer) - sizeof(*version));
	return 0;
}

long AmsRouter::ReadState(uint16_t port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* devState)
{
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader));
	uint8_t buffer[sizeof(*adsState) + sizeof(*devState)];
	uint32_t bytesRead;

	const long status = AdsRequest(request, *pAddr, port, AoEHeader::READ_STATE, sizeof(buffer), buffer, &bytesRead);
	if (status) {
		return status;
	}

	*adsState = qFromLittleEndian<uint16_t>(buffer + 0);
	*devState = qFromLittleEndian<uint16_t>(buffer + 2);
	return 0;
}

long AmsRouter::AdsRequest(Frame& request, const AmsAddr& destAddr, uint16_t port, uint16_t cmdId, uint32_t bufferLength, void* buffer, uint32_t *bytesRead)
{
	AmsAddr srcAddr;
	const auto status = GetLocalAddress(port, &srcAddr);
	if (status) {
		return status;
	}

	auto ads = GetConnection(destAddr.netId);
	if (!ads) {
		return -1;
	}

	uint32_t timeout_ms;
	GetTimeout(port, timeout_ms);
	AdsResponse* response = ads->Write(request, destAddr, srcAddr, cmdId);
	if (response) {
		if (response->Wait(timeout_ms)){
			*bytesRead = std::min<uint32_t>(bufferLength, response->frame.size());
			memcpy(buffer, response->frame.data(), *bytesRead);
			ads->Release(response);
			return 0;
		}
		return ADSERR_CLIENT_SYNCTIMEOUT;
	}
	return -1;
}

long AmsRouter::Write(uint16_t port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, const void* buffer)
{
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AoERequestHeader) + bufferLength);
	request.prepend(buffer, bufferLength);

	AoERequestHeader header{ indexGroup, indexOffset, bufferLength };
	request.prepend<AoERequestHeader>(header);

	uint8_t errorCode[sizeof(uint32_t)];
	uint32_t bytesRead = 0;
	const long status = AdsRequest(request, *pAddr, port, AoEHeader::WRITE, sizeof(errorCode), &errorCode, &bytesRead);
	if (status) {
		return status;
	}
	return qFromLittleEndian<uint32_t>(errorCode);
}

long AmsRouter::WriteControl(uint16_t port, const AmsAddr* pAddr, uint16_t adsState, uint16_t devState, uint32_t bufferLength, const void* buffer)
{
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AdsWriteCtrlRequest) + bufferLength);
	request.prepend(buffer, bufferLength);

	AdsWriteCtrlRequest header{ adsState, devState, bufferLength };
	request.prepend<AdsWriteCtrlRequest>(header);

	uint8_t errorCode[sizeof(uint32_t)];
	uint32_t bytesRead = 0;
	const long status = AdsRequest(request, *pAddr, port, AoEHeader::WRITE_CONTROL, sizeof(errorCode), &errorCode, &bytesRead);
	if (status) {
		return status;
	}
	return qFromLittleEndian<uint32_t>(errorCode);
}

long AmsRouter::AddNotification(uint16_t port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, const AdsNotificationAttrib* pAttrib, PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t *pNotification)
{
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AdsAddDeviceNotificationRequest));
	request.prepend<AdsAddDeviceNotificationRequest>(AdsAddDeviceNotificationRequest{
		indexGroup,
		indexOffset,
		pAttrib->cbLength,
		pAttrib->nTransMode,
		pAttrib->nMaxDelay,
		pAttrib->nCycleTime
	});

	uint8_t response[8];
	uint32_t bytesRead;
	const long status = AdsRequest(request, *pAddr, port, AoEHeader::ADD_DEVICE_NOTIFICATION, sizeof(response), &response, &bytesRead);
	if (status) {
		return status;
	}

	*pNotification = qFromLittleEndian<uint32_t>(response + 4);
	const auto result = qFromLittleEndian<uint32_t>(response);
	if (result) {
		return result;
	}
	return CreateNotifyMapping(port, *pAddr, pFunc, hUser, *pNotification);
}

long AmsRouter::DelNotification(uint16_t port, const AmsAddr* pAddr, uint32_t hNotification)
{
	DeleteNotifyMapping(port, *pAddr, hNotification);
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AdsDelDeviceNotificationRequest));
	request.prepend<AdsDelDeviceNotificationRequest>(qToLittleEndian<AdsDelDeviceNotificationRequest>(hNotification));

	uint8_t errorCode[sizeof(uint32_t)];
	uint32_t bytesRead = 0;
	const long status = AdsRequest(request, *pAddr, port, AoEHeader::DEL_DEVICE_NOTIFICATION, sizeof(errorCode), &errorCode, &bytesRead);
	if (status) {
		return status;
	}
	return qFromLittleEndian<uint32_t>(errorCode);
}

long AmsRouter::CreateNotifyMapping(uint16_t port, const AmsAddr &addr, PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t hNotify)
{
	std::lock_guard<std::mutex> lock(notificationLock);

	auto table = tableMapping.emplace(addr, TableRef(new NotifyTable())).first->second.get();
	table->emplace(hNotify, Notification{ pFunc, hUser, port });
	return 0;
}

void AmsRouter::DeleteNotifyMapping(uint16_t port, const AmsAddr &addr, uint32_t hNotify)
{
	std::lock_guard<std::mutex> lock(notificationLock);

	auto table = tableMapping.find(addr);
	if (table != tableMapping.end()) {
		table->second->erase(hNotify);
	}
}

std::vector<AmsRouter::NotifyPair> AmsRouter::CollectOrphanedNotifies(const uint16_t port)
{
	std::vector<NotifyPair> orphanedNotifies{};
	std::unique_lock<std::mutex> lock(notificationLock);

	for (const auto &mapping : tableMapping) {
		auto &table = mapping.second.operator*();
		for (auto it: table) {
			if (it.second.port == port) {
				orphanedNotifies.emplace_back(NotifyPair{ mapping.first, it.first });
			}
		}
	}
	return orphanedNotifies;
}

template<class T> T extractLittleEndian(Frame& frame)
{
	const auto value = qFromLittleEndian<T>(frame.data());
	frame.remove(sizeof(T));
	return value;
}

#define netIdToStream(netId) \
	std::dec << (int)netId.b[0] << '.' << (int)netId.b[1] << '.' << (int)netId.b[2] << '.' << (int)netId.b[3] << '.' << (int)netId.b[4] << '.' << (int)netId.b[5]	

void AmsRouter::Dispatch(Frame &frame, const AmsAddr amsAddr) const
{
	static const int FREQUENCY = 10;
	static int i = 0;
	if (!(++i % FREQUENCY)) {

		const auto table = tableMapping.find(amsAddr);
		if (table == tableMapping.end()) {
			LOG_WARN("Notifcation from unknown source: " << netIdToStream(amsAddr.netId));
			return;
		}

		LOG_INFO("Dispatching: " << netIdToStream(amsAddr.netId));

		const auto length = extractLittleEndian<uint32_t>(frame);
		auto numStamps = extractLittleEndian<uint32_t>(frame);
		LOG_INFO("frameLength: " << frame.size() << " length: " << length << " numStamps: " << numStamps);

		while (numStamps-- > 0) {
			const auto timestamp = extractLittleEndian<uint64_t>(frame);
			auto numSamples = extractLittleEndian<uint32_t>(frame);
			LOG_INFO("Timespam: " << timestamp << " numSamples: " << numSamples);
			while (numSamples-- > 0) {
				const auto hNotify = extractLittleEndian<uint32_t>(frame);
				const auto size = extractLittleEndian<uint32_t>(frame);
				LOG_INFO("hNotify: " << hNotify << " size: " << size);
			}
		}
	}
}