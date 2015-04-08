#include "AmsConnection.h"
#include "AmsHeader.h"
#include "Log.h"

AmsResponse::AmsResponse()
	: frame(4096),
	invokeId(0)
{
}

void AmsResponse::Notify()
{
	invokeId = 0;
	cv.notify_all();
}

bool AmsResponse::Wait(uint32_t timeout_ms)
{
	std::unique_lock<std::mutex> lock(mutex);
	return cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&]() { return !invokeId; });
}

AmsConnection::AmsConnection(Router &__router, IpV4 destIp)
	: destIp(destIp),
	router(__router),
	socket(destIp, 48898),
	invokeId(0)
{
	socket.Connect();
	receiver = std::thread(&AmsConnection::TryRecv, this);
}

AmsConnection::~AmsConnection()
{
	socket.Shutdown();
	receiver.join();
}

void AmsConnection::CreateNotifyMapping(uint16_t port, AmsAddr addr, PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t length, uint32_t hNotify)
{
	std::lock_guard<std::mutex> lock(notificationLock[port - Router::PORT_BASE]);

	auto table = tableMapping[port - Router::PORT_BASE].emplace(addr, TableRef(new NotifyTable())).first->second.get();
	table->emplace(hNotify, Notification{ pFunc, hNotify, hUser, length, addr, port });
}

bool AmsConnection::DeleteNotifyMapping(const AmsAddr &addr, uint32_t hNotify, uint16_t port)
{
	std::lock_guard<std::mutex> lock(notificationLock[port - Router::PORT_BASE]);

	auto table = tableMapping[port - Router::PORT_BASE].find(addr);
	if (table != tableMapping[port - Router::PORT_BASE].end()) {
		return table->second->erase(hNotify);
	}
	return false;
}

void AmsConnection::DeleteOrphanedNotifications(const AmsPort &port)
{
	std::unique_lock<std::mutex> lock(notificationLock[port.port - Router::PORT_BASE]);

	for (auto& table : tableMapping[port.port - Router::PORT_BASE]) {
		for (auto& n : *table.second.get()) {
			__DeleteNotification(table.first, n.first, port);
		}
	}
	tableMapping[port.port - Router::PORT_BASE].clear();
}

long AmsConnection::__DeleteNotification(const AmsAddr &amsAddr, uint32_t hNotify, const AmsPort &port)
{
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(hNotify));
	request.prepend(qToLittleEndian(hNotify));
	return AdsRequest<AoEResponseHeader>(request, amsAddr, port, AoEHeader::DEL_DEVICE_NOTIFICATION);
}

AmsResponse* AmsConnection::Write(Frame& request, const AmsAddr destAddr, const AmsAddr srcAddr, uint16_t cmdId)
{
	AoEHeader aoeHeader{ destAddr, srcAddr, cmdId, static_cast<uint32_t>(request.size()), ++invokeId };
	request.prepend<AoEHeader>(aoeHeader);

	AmsTcpHeader header{ static_cast<uint32_t>(request.size()) };
	request.prepend<AmsTcpHeader>(header);

	auto response = Reserve(aoeHeader.invokeId(), srcAddr.port);
	if (response && request.size() != socket.write(request)) {
		Release(response);
		return nullptr;
	}
	return response;
}

AmsResponse* AmsConnection::GetPending(uint32_t id, uint16_t port)
{
	const auto currentId = queue[port - Router::PORT_BASE].invokeId;
	if (currentId == id) {
		return &queue[port - Router::PORT_BASE];
	}
	LOG_WARN("InvokeId missmatch: waiting for 0x" << std::hex << currentId << " received 0x" << id);
	return nullptr;
}

AmsResponse* AmsConnection::Reserve(uint32_t id, uint16_t port)
{
	if (queue[port - Router::PORT_BASE].invokeId) {
		LOG_WARN("Port: " << port << " already in use");
		return nullptr;
	}
	queue[port - Router::PORT_BASE].invokeId = id;
	return &queue[port - Router::PORT_BASE];
}

void AmsConnection::Release(AmsResponse* response)
{
	response->frame.reset();
	response->invokeId = 0;
}

void AmsConnection::Receive(uint8_t* buffer, size_t bytesToRead) const
{
	while (bytesToRead) {
		const size_t bytesRead = socket.read(buffer, bytesToRead);
		bytesToRead -= bytesRead;
		buffer += bytesRead;
	}
}

void AmsConnection::ReceiveJunk(size_t bytesToRead) const
{
	uint8_t buffer[1024];
	while (bytesToRead > sizeof(buffer)) {
		Receive(buffer, sizeof(buffer));
		bytesToRead -= sizeof(buffer);
	}
	Receive(buffer, bytesToRead);
}

template<class T> T AmsConnection::Receive() const
{
	uint8_t buffer[sizeof(T)];
	Receive(buffer, sizeof(buffer));
	return T{ buffer };
}

Frame& AmsConnection::ReceiveFrame(Frame &frame, size_t bytesLeft) const
{
	if (bytesLeft > frame.capacity()) {
		LOG_WARN("Frame to long: " << std::dec << bytesLeft << '<' << frame.capacity());
		ReceiveJunk(bytesLeft);
		return frame.clear();
	}
	Receive(frame.rawData(), bytesLeft);
	return frame.limit(bytesLeft);
}

bool AmsConnection::ReceiveNotification(const AoEHeader& header)
{
	auto &ring = GetRing(header.targetPort());
	auto bytesLeft = header.length();
	if (bytesLeft > ring.BytesFree()) {
		ReceiveJunk(bytesLeft);
		LOG_WARN("port " << std::dec << header.targetPort() << " receive buffer was full");
		return false;
	}

	auto chunk = ring.WriteChunk();
	while (bytesLeft > chunk) {
		Receive(ring.write, chunk);
		ring.Write(chunk);
		bytesLeft -= chunk;
		chunk = ring.WriteChunk();
	}
	Receive(ring.write, bytesLeft);
	ring.Write(bytesLeft);
	return true;
}

void AmsConnection::TryRecv()
{
	try {
		Recv();
	}
	catch (std::runtime_error &e) {
		LOG_INFO(e.what());
	}
}

void AmsConnection::Recv()
{
	for (;;) {
		const auto amsTcp = Receive<AmsTcpHeader>();
		if (amsTcp.length() < sizeof(AoEHeader)) {
			LOG_WARN("Frame to short to be AoE");
			ReceiveJunk(amsTcp.length());
			continue;
		}

		const auto header = Receive<AoEHeader>();
		if (header.cmdId() == AoEHeader::DEVICE_NOTIFICATION) {
#if 1
			if (ReceiveNotification(header)) {
				Dispatch(header.sourceAddr(), header.targetPort(), header.length());
			}
#else
			ReceiveJunk(header.length());
#endif
			continue;
		}

		auto response = GetPending(header.invokeId(), header.targetPort());
		if (!response) {
			LOG_WARN("No response pending");
			ReceiveJunk(header.length());
			continue;
		}

		ReceiveFrame(response->frame, header.length());
		switch (header.cmdId()) {
		case AoEHeader::READ_DEVICE_INFO:
		case AoEHeader::READ:
		case AoEHeader::WRITE:
		case AoEHeader::READ_STATE:
		case AoEHeader::WRITE_CONTROL:
		case AoEHeader::ADD_DEVICE_NOTIFICATION:
		case AoEHeader::DEL_DEVICE_NOTIFICATION:
		case AoEHeader::READ_WRITE:
			break;
		default:
			LOG_WARN("Unkown AMS command id");
			response->frame.clear();
		}
		response->Notify();
	}
}

const std::map<uint32_t, Notification>* AmsConnection::GetNotifyTable(const AmsAddr& amsAddr, uint16_t port)
{
	const auto table = tableMapping[port - Router::PORT_BASE].find(amsAddr);
	if (table == tableMapping[port - Router::PORT_BASE].end()) {
		return nullptr;
	}
	return table->second.get();
}

void AmsConnection::Dispatch(const AmsAddr amsAddr, uint16_t port, size_t expectedSize)
{
	std::unique_lock<std::mutex> lock(notificationLock[port - Router::PORT_BASE]);
	auto table = GetNotifyTable(amsAddr, port);
	auto &ring = GetRing(port);
	
	if (!table) {
		LOG_WARN("Notification from unknown source: " << std::dec
			<< (int)amsAddr.netId.b[0] << '.' << (int)amsAddr.netId.b[1] << '.' << (int)amsAddr.netId.b[2] << '.'
			<< (int)amsAddr.netId.b[3] << '.' << (int)amsAddr.netId.b[4] << '.' << (int)amsAddr.netId.b[5] << '.');
		ring.Read(expectedSize);
		return;
	}

	const auto length = ring.ReadFromLittleEndian<uint32_t>();
	if (length != expectedSize - sizeof(length)) {
		LOG_WARN("Notification length: " << std::dec << length << " doesn't match: " << expectedSize);
		ring.Read(expectedSize - sizeof(length));
		return;
	}

	const auto numStamps = ring.ReadFromLittleEndian<uint32_t>();
	for (uint32_t stamp = 0; stamp < numStamps; ++stamp) {
		const auto timestamp = ring.ReadFromLittleEndian<uint64_t>();
		const auto numSamples = ring.ReadFromLittleEndian<uint32_t>();
		for (uint32_t sample = 0; sample < numSamples; ++sample) {
			const auto hNotify = ring.ReadFromLittleEndian<uint32_t>();
			const auto size = ring.ReadFromLittleEndian<uint32_t>();
			auto it = table->find(hNotify);
			if (it != table->end()) {
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
