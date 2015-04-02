#include "AmsConnection.h"
#include "AmsHeader.h"
#include "AmsRouter.h"
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

void AmsConnection::Read(uint8_t* buffer, size_t bytesToRead) const
{
	while (bytesToRead) {
		const size_t bytesRead = socket.read(buffer, bytesToRead);
		bytesToRead -= bytesRead;
		buffer += bytesRead;
	}
}

void AmsConnection::ReadJunk(size_t bytesToRead) const
{
	uint8_t buffer[1024];
	while (bytesToRead > sizeof(buffer)) {
		Read(buffer, sizeof(buffer));
		bytesToRead -= sizeof(buffer);
	}
	Read(buffer, bytesToRead);
}

template<class T> T AmsConnection::Receive() const
{
	uint8_t buffer[sizeof(T)];
	Read(buffer, sizeof(buffer));
	return T{ buffer };
}

Frame& AmsConnection::ReceiveFrame(Frame &frame, size_t bytesLeft) const
{
	if (bytesLeft > frame.capacity()) {
		LOG_WARN("Frame to long: " << std::dec << bytesLeft << '<' << frame.capacity());
		ReadJunk(bytesLeft);
		return frame.clear();
	}
	Read(frame.rawData(), bytesLeft);
	return frame.limit(bytesLeft);
}

void AmsConnection::Read(const AoEHeader& header) const
{
	auto &ring = router.GetRing(header.targetPort());

	auto bytesLeft = header.length();
	auto chunk = ring.WriteChunk();
	while (bytesLeft > chunk) {
		Read(ring.write, chunk);
		ring.Write(chunk);
		chunk = ring.WriteChunk();
		bytesLeft -= chunk;
		Sleep(0);
	}
	Read(ring.write, bytesLeft);
	ring.Write(bytesLeft);
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
			ReadJunk(amsTcp.length());
			continue;
		}

		const auto header = Receive<AoEHeader>();
		if (header.cmdId() == AoEHeader::DEVICE_NOTIFICATION) {
			Read(header);
			router.Dispatch(header.sourceAddr(), header.targetPort(), header.length() - sizeof(uint32_t));
			continue;
		}

		auto response = GetPending(header.invokeId(), header.targetPort());
		if (!response) {
			LOG_WARN("No response pending");
			ReadJunk(header.length());
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
