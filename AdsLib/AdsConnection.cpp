#include "AdsConnection.h"
#include "AmsHeader.h"
#include "AmsRouter.h"
#include "Log.h"

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

bool AdsResponse::Wait(uint32_t timeout_ms)
{
	std::unique_lock<std::mutex> lock(mutex);
	return std::cv_status::no_timeout == cv.wait_for(lock, std::chrono::milliseconds(timeout_ms));
}

AdsConnection::AdsConnection(const NotificationDispatcher &__dispatcher, IpV4 destIp)
	: dispatcher(__dispatcher),
	destIp(destIp),
	socket(destIp, 48898),
	invokeId(0)
{
	for (auto& r : responses) {
		ready.push_back(&r);
	}
	socket.Connect();

	receiver = std::thread(&AdsConnection::TryRecv, this);
}


AdsConnection::~AdsConnection()
{
	running = false;
	receiver.join();
}

AdsResponse* AdsConnection::Write(Frame& request, const AmsAddr destAddr, const AmsAddr srcAddr, uint16_t cmdId)
{
	AoEHeader aoeHeader{ destAddr, srcAddr, cmdId, static_cast<uint32_t>(request.size()), ++invokeId };
	request.prepend<AoEHeader>(aoeHeader);

	AmsTcpHeader header{ static_cast<uint32_t>(request.size()) };
	request.prepend<AmsTcpHeader>(header);

	std::lock_guard<std::mutex> lock(mutex);

	if (ready.empty()) {
		return nullptr;
	}

	auto response = ready.back();
	ready.pop_back();
	
	response->invokeId = aoeHeader.invokeId;
	response->frame.clear();
	
	if (request.size() != socket.write(request)) {
		return nullptr;
	}

	pending.push_back(response);
	return response;
}

AdsResponse* AdsConnection::GetPending(uint32_t id)
{
	for (auto p : pending) {
		if (p->invokeId == id) {
			pending.remove(p);
			return p;
		}
	}
	return nullptr;
}

void AdsConnection::Release(AdsResponse* response)
{
	std::lock_guard<std::mutex> lock(mutex);
	ready.push_back(response);
}

Frame& AdsConnection::ReceiveAmsTcp(Frame &frame)
{
	if (frame.size() < sizeof(AmsTcpHeader)) {
		LOG_ERROR("packet to short to be AMS/TCP");
		return frame.clear();
	}

	const auto header = frame.remove<AmsTcpHeader>();
	if (header.length != frame.size()) {
		LOG_ERROR("received AMS/TCP frame seems corrupt, length: " << header.length << " doesn't match: " << frame.size());
		return frame.clear();
	}
	return frame;
}

void AdsConnection::TryRecv()
{
	try {
		Recv();
	}
	catch (std::runtime_error &e) {
		LOG_INFO(e.what());
	}
}

void AdsConnection::Recv()
{
	static const size_t FRAME_SIZE = 1024;
	Frame frame(FRAME_SIZE);
	while (running) {
		frame.reset(FRAME_SIZE);
		socket.read(frame);
		if (frame.size() > 0 && ReceiveAmsTcp(frame).size() > 0) {
			if (frame.size() >= sizeof(AoEHeader)) {
				const auto header = frame.remove<AoEHeader>();

				if (header.cmdId == AoEHeader::DEVICE_NOTIFICATION) {
					dispatcher.Dispatch(frame, header.sourceAddr);
				} else {
					auto response = GetPending(header.invokeId);
					if (response) {
						switch (header.cmdId) {
						case AoEHeader::READ_DEVICE_INFO:
							frame.remove(sizeof(uint32_t));
							break;
						case AoEHeader::READ:
							frame.remove<AoEReadResponseHeader>();
							break;
						case AoEHeader::WRITE:
							break;
						case AoEHeader::READ_STATE:
							frame.remove(sizeof(uint32_t));
							break;
						case AoEHeader::WRITE_CONTROL:
						case AoEHeader::ADD_DEVICE_NOTIFICATION:
						case AoEHeader::DEL_DEVICE_NOTIFICATION:
							break;
						default:
							frame.clear();
						}
						// prepend() was benchmarked against frame.swap() and was three times faster for small frames -> very workload dependend!
						response->frame.prepend(frame.data(), frame.size());
						response->Notify();
					}
				}
			}
		}
	}
}
