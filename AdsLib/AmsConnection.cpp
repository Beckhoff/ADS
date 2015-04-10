#include "AmsConnection.h"
#include "AmsHeader.h"
#include "Log.h"

AmsResponse::AmsResponse()
	: frame(4096),
	invokeId(0),
	extra(0)
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
	invokeId(0),
	ringBuffer(4 * 1024 * 1024)
{
	socket.Connect();
	receiver = std::thread(&AmsConnection::TryRecv, this);
}

AmsConnection::~AmsConnection()
{
	socket.Shutdown();
	receiver.join();
}

size_t AmsConnection::CreateNotifyMapping(uint16_t port, AmsAddr addr, PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t length, uint32_t hNotify)
{
	const auto hash = Hash(hNotify, addr, port);
	std::lock_guard<std::recursive_mutex> lock(notificationsLock);
	notifications.emplace(hash, Notification{ pFunc, hNotify, hUser, length, addr, port });
	return hash;
}

bool AmsConnection::DeleteNotifyMapping(size_t hash)
{
	std::lock_guard<std::recursive_mutex> lock(notificationsLock);
	return notifications.erase(hash);
}

void AmsConnection::DeleteOrphanedNotifications(AmsPort &port)
{
	for (auto hash : port.GetNotifications()) {
		std::unique_lock<std::recursive_mutex> lock(notificationsLock);
		auto it = notifications.find(hash);
		if (it != notifications.end()) {
			const auto amsAddr = it->second.amsAddr;
			const auto hNotify = it->second.hNotify();
			notifications.erase(hash);
			lock.unlock();
			__DeleteNotification(amsAddr, hNotify, port);
		}
	}
}

long AmsConnection::__DeleteNotification(const AmsAddr &amsAddr, uint32_t hNotify, const AmsPort &port)
{
	Frame request(sizeof(AmsTcpHeader) + sizeof(AoEHeader) + sizeof(hNotify));
	request.prepend(qToLittleEndian(hNotify));
	return AdsRequest<AoEResponseHeader>(request, amsAddr, port, AoEHeader::DEL_DEVICE_NOTIFICATION);
}

AmsResponse* AmsConnection::Write(Frame& request, const AmsAddr destAddr, const AmsAddr srcAddr, uint16_t cmdId, uint32_t extra)
{
	AoEHeader aoeHeader{ destAddr, srcAddr, cmdId, static_cast<uint32_t>(request.size()), ++invokeId };
	request.prepend<AoEHeader>(aoeHeader);

	AmsTcpHeader header{ static_cast<uint32_t>(request.size()) };
	request.prepend<AmsTcpHeader>(header);

	auto response = Reserve(aoeHeader.invokeId(), srcAddr.port);

	if (!response) {
		return nullptr;
	}

	response->extra = extra;
	if (request.size() != socket.write(request)) {
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
			if (ReceiveNotification(header)) {
				Dispatch(header.sourceAddr(), header.targetPort(), header.length());
			}
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

template<typename T>
static void hash_combine(size_t & seed, T const& v)
{
	seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std {
	template<>
	struct hash < AmsAddr >
	{
		size_t operator()(const AmsAddr &ams) const
		{
			size_t value = 0;
			hash_combine(value, ams.netId.b[0]);
			hash_combine(value, ams.netId.b[1]);
			hash_combine(value, ams.netId.b[2]);
			hash_combine(value, ams.netId.b[3]);
			hash_combine(value, ams.netId.b[4]);
			hash_combine(value, ams.netId.b[5]);
			hash_combine(value, ams.port);
			return value;
		}
	};
}

std::ostream& operator <<(std::ostream& out, const AmsAddr &ams)
{
	return out << std::dec << (int)ams.netId.b[0] << '.' << (int)ams.netId.b[1] << '.' << (int)ams.netId.b[2] << '.'
		<< (int)ams.netId.b[3] << '.' << (int)ams.netId.b[4] << '.' << (int)ams.netId.b[5] << ':'
		<< ams.port;
}

size_t AmsConnection::Hash(uint32_t hNotify, AmsAddr srcAddr, uint16_t port)
{
	size_t value = 0;
	hash_combine<uint32_t>(value, hNotify);
	hash_combine(value, srcAddr);
	hash_combine(value, port);
	return value;
}

void AmsConnection::Dispatch(const AmsAddr amsAddr, uint16_t port, size_t expectedSize)
{
	auto &ring = GetRing(port);

//TODO move dispatching into seperate thread!
#if 1
	ring.Read(expectedSize);
	return;
#endif

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

			const auto hash = Hash(hNotify, amsAddr, port);
			std::lock_guard<std::recursive_mutex> lock(notificationsLock);
			auto it = notifications.find(hash);
			if (it != notifications.end()) {
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
