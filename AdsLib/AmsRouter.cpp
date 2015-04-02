#include "AmsRouter.h"
#include "AmsHeader.h"
#include "Frame.h"
#include "Log.h"

#include <algorithm>

const uint32_t AmsRouter::DEFAULT_TIMEOUT = 5000;

AmsRouter::AmsRouter(AmsNetId netId)
	: localAddr(netId)
{
	std::fill(portTimeout.begin(), portTimeout.end(), DEFAULT_TIMEOUT);
}

bool AmsRouter::AddRoute(AmsNetId ams, const IpV4& ip)
{
	std::lock_guard<std::mutex> lock(mutex);

	const auto oldConnection = GetConnection(ams);
	auto conn = connections.find(ip);
	if (conn == connections.end()) {
		conn = connections.emplace(ip, std::unique_ptr<AmsConnection>(new AmsConnection{ *this, ip })).first;
	}

	mapping[ams] = conn->second.get();
	DeleteIfLastConnection(oldConnection);	
	return true;
}

void AmsRouter::DelRoute(const AmsNetId& ams)
{
	std::lock_guard<std::mutex> lock(mutex);

	auto route = mapping.find(ams);
	if (route != mapping.end()) {
		const AmsConnection* conn = route->second;
		mapping.erase(route);
		DeleteIfLastConnection(conn);
	}
}

void AmsRouter::SetNetId(AmsNetId ams)
{
	std::lock_guard<std::mutex> lock(mutex);
	localAddr = ams;
}

void AmsRouter::DeleteIfLastConnection(const AmsConnection* conn)
{
	if (conn) {
		for (const auto& r : mapping) {
			if (r.second == conn) {
				return;
			}
		}
		connections.erase(conn->destIp);
	}
}

uint16_t AmsRouter::OpenPort()
{
	std::lock_guard<std::mutex> lock(mutex);

	if (ports.all()) {
		return 0;
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
	DeleteOrphanedNotifications(port);

	std::lock_guard<std::mutex> lock(mutex);
	if (port < PORT_BASE || port >= PORT_BASE + NUM_PORTS_MAX || !ports.test(port - PORT_BASE)) {
		return ADSERR_CLIENT_PORTNOTOPEN;
	}
	ports.reset(port - PORT_BASE);
	return 0;
}

long AmsRouter::GetLocalAddress(uint16_t port, AmsAddr* pAddr)
{
	std::lock_guard<std::mutex> lock(mutex);
	if (port < PORT_BASE || port >= PORT_BASE + NUM_PORTS_MAX) {
		return ADSERR_CLIENT_PORTNOTOPEN;
	}

	if (ports.test(port - PORT_BASE)) {
		memcpy(&pAddr->netId, &localAddr, sizeof(localAddr));
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

AmsConnection* AmsRouter::GetConnection(const AmsNetId& amsDest)
{
	const auto it = __GetConnection(amsDest);
	if (it == connections.end()) {
		return nullptr;
	}
	return it->second.get();
}

std::map<IpV4, std::unique_ptr<AmsConnection>>::iterator AmsRouter::__GetConnection(const AmsNetId& amsDest)
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
	request.prepend<AoERequestHeader>({
		indexGroup,
		indexOffset,
		bufferLength
	});
	return AdsRequest<AoEReadResponseHeader>(request, *pAddr, port, AoEHeader::READ, bufferLength, buffer, bytesRead);
}

long AmsRouter::ReadDeviceInfo(uint16_t port, const AmsAddr* pAddr, char* devName, AdsVersion* version)
{
	static const size_t NAME_LENGTH = 16;
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader));
	uint8_t buffer[sizeof(*version) + NAME_LENGTH];

	const auto status = AdsRequest<AoEResponseHeader>(request, *pAddr, port, AoEHeader::READ_DEVICE_INFO, sizeof(buffer), buffer);
	if (!status) {
		version->version = buffer[0];
		version->revision = buffer[1];
		version->build = qFromLittleEndian<uint16_t>(buffer + offsetof(AdsVersion, build));
		memcpy(devName, buffer + sizeof(*version), NAME_LENGTH);
	}
	return status;
}

long AmsRouter::ReadState(uint16_t port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* devState)
{
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader));
	uint8_t buffer[sizeof(*adsState) + sizeof(*devState)];

	const auto status = AdsRequest<AoEResponseHeader>(request, *pAddr, port, AoEHeader::READ_STATE, sizeof(buffer), buffer);
	if (!status) {
		*adsState = qFromLittleEndian<uint16_t>(buffer);
		*devState = qFromLittleEndian<uint16_t>(buffer + sizeof(*adsState));
	}
	return status;
}

long AmsRouter::ReadWrite(uint16_t port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t readLength, void* readData, uint32_t writeLength, const void* writeData, uint32_t *bytesRead)
{
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AoEReadWriteReqHeader) + writeLength);
	request.prepend(writeData, writeLength);
	request.prepend<AoEReadWriteReqHeader>({
		indexGroup,
		indexOffset,
		readLength,
		writeLength
	});
	return AdsRequest<AoEReadResponseHeader>(request, *pAddr, port, AoEHeader::READ_WRITE, readLength, readData, bytesRead);
}

template <class T>
long AmsRouter::AdsRequest(Frame& request, const AmsAddr& destAddr, uint16_t port, uint16_t cmdId, uint32_t bufferLength, void* buffer, uint32_t *bytesRead)
{
	if (bytesRead) {
		*bytesRead = 0;
	}
	AmsAddr srcAddr;
	const auto status = GetLocalAddress(port, &srcAddr);
	if (status) {
		return status;
	}

	auto ads = GetConnection(destAddr.netId);
	if (!ads) {
		return GLOBALERR_MISSING_ROUTE;
	}

	AmsResponse* response = ads->Write(request, destAddr, srcAddr, cmdId);
	if (response) {
		uint32_t timeout_ms;
		GetTimeout(port, timeout_ms);
		if (response->Wait(timeout_ms)){
			const uint32_t bytesAvailable = std::min<uint32_t>(bufferLength, response->frame.size() - sizeof(T));
			T header(response->frame.data());
			memcpy(buffer, response->frame.data() + sizeof(T), bytesAvailable);
			if (bytesRead) {
				*bytesRead = bytesAvailable;
			}
			ads->Release(response);
			return header.result();
		}
		ads->Release(response);
		return ADSERR_CLIENT_SYNCTIMEOUT;
	}
	return -1;
}

long AmsRouter::Write(uint16_t port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, const void* buffer)
{
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AoERequestHeader) + bufferLength);
	request.prepend(buffer, bufferLength);
	request.prepend<AoERequestHeader>({
		indexGroup,
		indexOffset,
		bufferLength
	});
	return AdsRequest<AoEResponseHeader>(request, *pAddr, port, AoEHeader::WRITE);
}

long AmsRouter::WriteControl(uint16_t port, const AmsAddr* pAddr, uint16_t adsState, uint16_t devState, uint32_t bufferLength, const void* buffer)
{
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AdsWriteCtrlRequest) + bufferLength);
	request.prepend(buffer, bufferLength);
	request.prepend<AdsWriteCtrlRequest>({
		adsState,
		devState,
		bufferLength
	});
	return AdsRequest<AoEResponseHeader>(request, *pAddr, port, AoEHeader::WRITE_CONTROL);
}

long AmsRouter::AddNotification(uint16_t port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, const AdsNotificationAttrib* pAttrib, PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t *pNotification)
{
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(AdsAddDeviceNotificationRequest));
	request.prepend<AdsAddDeviceNotificationRequest>({
		indexGroup,
		indexOffset,
		pAttrib->cbLength,
		pAttrib->nTransMode,
		pAttrib->nMaxDelay,
		pAttrib->nCycleTime
	});

	uint8_t buffer[sizeof(*pNotification)];
	const long status = AdsRequest<AoEResponseHeader>(request, *pAddr, port, AoEHeader::ADD_DEVICE_NOTIFICATION, sizeof(buffer), buffer);
	if (!status) {
		*pNotification = qFromLittleEndian<uint32_t>(buffer);
		CreateNotifyMapping(port, *pAddr, pFunc, hUser, pAttrib->cbLength, *pNotification);
	}
	return status;
}

long AmsRouter::DelNotification(uint16_t port, const AmsAddr* pAddr, uint32_t hNotification)
{
	if (!DeleteNotifyMapping(*pAddr, hNotification, port)) {
		return ADSERR_CLIENT_REMOVEHASH;
	}
	return __DeleteNotification(*pAddr, hNotification, port);
}

void AmsRouter::CreateNotifyMapping(uint16_t port, AmsAddr addr, PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t length, uint32_t hNotify)
{
	std::lock_guard<std::mutex> lock(notificationLock[port - Router::PORT_BASE]);

	auto table = tableMapping[port - Router::PORT_BASE].emplace(addr, TableRef(new NotifyTable())).first->second.get();
	table->emplace(hNotify, Notification{ pFunc, hNotify, hUser, length, addr, port });
}

bool AmsRouter::DeleteNotifyMapping(const AmsAddr &addr, uint32_t hNotify, uint16_t port)
{
	std::lock_guard<std::mutex> lock(notificationLock[port - Router::PORT_BASE]);

	auto table = tableMapping[port - Router::PORT_BASE].find(addr);
	if (table != tableMapping[port - Router::PORT_BASE].end()) {
		return table->second->erase(hNotify);
	}
	return false;
}

void AmsRouter::DeleteOrphanedNotifications(const uint16_t port)
{
	std::unique_lock<std::mutex> lock(notificationLock[port - Router::PORT_BASE]);

	for (auto& table : tableMapping[port - Router::PORT_BASE]) {
		for (auto& n : *table.second.get()) {
			__DeleteNotification(table.first, n.first, port);
		}
	}
	tableMapping[port - Router::PORT_BASE].clear();
}

long AmsRouter::__DeleteNotification(const AmsAddr &amsAddr, uint32_t hNotify, uint16_t port)
{
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(hNotify));
	request.prepend(qToLittleEndian(hNotify));
	return AdsRequest<AoEResponseHeader>(request, amsAddr, port, AoEHeader::DEL_DEVICE_NOTIFICATION);
}

template<class T> T extractLittleEndian(Frame& frame)
{
	const auto value = qFromLittleEndian<T>(frame.data());
	frame.remove(sizeof(T));
	return value;
}

std::ostream& operator<<(std::ostream& out, const AmsNetId& netId)
{
	return out << std::dec << (int)netId.b[0] << '.' << (int)netId.b[1] << '.' << (int)netId.b[2] << '.' << (int)netId.b[3] << '.' << (int)netId.b[4] << '.' << (int)netId.b[5];
}

void AmsRouter::Dispatch(const AmsAddr amsAddr, uint16_t port, size_t expectedSize)
{
	const auto table = tableMapping[port - Router::PORT_BASE].find(amsAddr);
	if (table == tableMapping[port - Router::PORT_BASE].end()) {
		LOG_WARN("Notifcation from unknown source: " << amsAddr.netId);
		return;
	}

	auto &ring = GetRing(port);
	const auto length = ring.ReadFromLittleEndian<uint32_t>();
	if (length != expectedSize) {
		LOG_WARN("Notification length: " << std::dec << length << " doesn't match: " << expectedSize);
		ring.Read(expectedSize);
		return;
	}

	const auto numStamps = ring.ReadFromLittleEndian<uint32_t>();
	for (uint32_t stamp = 0; stamp < numStamps; ++stamp) {
		const auto timestamp = ring.ReadFromLittleEndian<uint64_t>();
		const auto numSamples = ring.ReadFromLittleEndian<uint32_t>();
		for (uint32_t sample = 0; sample < numSamples; ++sample) {
			const auto hNotify = ring.ReadFromLittleEndian<uint32_t>();
			const auto size = ring.ReadFromLittleEndian<uint32_t>();
			auto it = table->second->find(hNotify);
			if (it != table->second->end()) {
				auto &notification = it->second;
				if (size != notification.Size()) {
					LOG_WARN("Notification sample size: " << size << " doesn't match: " << notification.Size());
					ring.Read(size);
					return;
				}
				notification.Notify(timestamp, ring);
			}
			else {
				ring.Read(size);
			}
		}
	}
}
