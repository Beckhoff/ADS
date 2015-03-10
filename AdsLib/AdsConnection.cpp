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

AdsConnection::AdsConnection(IpV4 destIp)
	: destIp(destIp),
	socket(destIp, 48898),
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

AdsResponse* AdsConnection::Write(Frame& request, const AmsAddr destAddr, const AmsAddr srcAddr, uint16_t cmdId)
{
	AoEHeader aoeHeader{ destAddr, srcAddr, cmdId, request.size(), ++invokeId };
	request.prepend<AoEHeader>(aoeHeader);

	AmsTcpHeader header{ request.size() };
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

Frame& ReceiveAmsTcp(Frame &frame)
{
	if (frame.size() < sizeof(AmsTcpHeader)) {
		//LOG_ERROR("packet to short to be AMS/TCP");
		return frame.clear();
	}

	const auto header = frame.remove<AmsTcpHeader>();
	if (header.length != frame.size()) {
		//LOG_ERROR("received AMS/TCP frame seems corrupt, length: " << header.length << " doesn't match: " << frame.size());
		return frame.clear();
	}
	return frame;
}

void AdsConnection::Recv()
{
	while (running) {
		Frame frame(2048);
		socket.read(frame);
		if (ReceiveAmsTcp(frame).size() > 0) {
			if (frame.size() >= sizeof(AoEHeader)) {
				const auto header = frame.remove<AoEHeader>();

				auto response = GetPending(header.invokeId);
				if (response) {
					switch (header.cmdId) {
					case AoEHeader::READ_DEVICE_INFO:
						frame.remove<uint32_t>();
						break;
					case AoEHeader::READ:
						frame.remove<AoEReadResponseHeader>();
						break;
					case AoEHeader::WRITE:
						break;
					case AoEHeader::READ_STATE:
						frame.remove<uint32_t>();
						break;
					default:
						frame.clear();
					}
					//TODO avoid memcpy
					response->frame.prepend(frame.data(), frame.size());
					response->Notify();
					frame.clear();
				}
			}
		}
	}
}
