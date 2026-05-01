// SPDX-License-Identifier: MIT
#include "SecureAmsConnection.h"
#include "TlsConnectInfo.h"
#include "Log.h"
#include "wrap_endian.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

// ===== Constructor / Destructor =====

SecureAmsConnection::SecureAmsConnection(
	Router &router, const struct addrinfo *destination,
	const bhf::ads::SecureAdsConfig &config, const AmsNetId &localNetId)
	: m_Router(router)
	, m_Socket(destination, config)
{
	ownIp = m_Socket.Connect();
	Handshake(config, localNetId);
	m_Receiver = std::thread(&SecureAmsConnection::TryRecv, this);
}

SecureAmsConnection::~SecureAmsConnection()
{
	m_Socket.Shutdown();
	m_Receiver.join();
}

// ===== Handshake =====

void SecureAmsConnection::Handshake(const bhf::ads::SecureAdsConfig &config,
				    const AmsNetId &localNetId)
{
	char hostname[32] = {};
	gethostname(hostname, sizeof(hostname) - 1);

	TlsConnectInfoBase req = {};
	req.version = TlsConnectInfo::VERSION;
	memcpy(&req.netId, &localNetId, sizeof(AmsNetId));
	memcpy(req.hostname, hostname,
	       std::min(strlen(hostname), sizeof(req.hostname) - 1));

	const bool isSSC =
		(config.mode == bhf::ads::SecureAdsConfig::Mode::SSC);
	const bool firstTime = isSSC && !config.username.empty();

	if (firstTime) {
		const auto userLen = static_cast<uint8_t>(
			std::min(config.username.size(), size_t(255)));
		const auto pwdLen = static_cast<uint8_t>(
			std::min(config.password.size(), size_t(255)));

		req.flags = bhf::ads::htole<uint16_t>(
			TlsConnectInfo::FLAG_ADD_REMOTE |
			TlsConnectInfo::FLAG_SELF_SIGNED |
			TlsConnectInfo::FLAG_IP_ADDR |
			TlsConnectInfo::FLAG_IGNORE_CN);
		req.userLen = userLen;
		req.pwdLen = pwdLen;
		req.totalLength =
			bhf::ads::htole<uint16_t>(static_cast<uint16_t>(
				sizeof(TlsConnectInfoBase) + userLen + pwdLen));

		m_Socket.write_raw(reinterpret_cast<const uint8_t *>(&req),
				   sizeof(req));
		m_Socket.write_raw(reinterpret_cast<const uint8_t *>(
					   config.username.c_str()),
				   userLen);
		m_Socket.write_raw(reinterpret_cast<const uint8_t *>(
					   config.password.c_str()),
				   pwdLen);
	} else {
		req.flags = bhf::ads::htole<uint16_t>(
			isSSC ? TlsConnectInfo::FLAG_SELF_SIGNED : uint16_t(0));
		req.totalLength = bhf::ads::htole<uint16_t>(
			static_cast<uint16_t>(sizeof(TlsConnectInfoBase)));

		m_Socket.write_raw(reinterpret_cast<const uint8_t *>(&req),
				   sizeof(req));
	}

	TlsConnectInfoBase resp = {};
	m_Socket.read_raw(reinterpret_cast<uint8_t *>(&resp), sizeof(resp));

	const auto respFlags = bhf::ads::letoh(resp.flags);
	if (!(respFlags & TlsConnectInfo::FLAG_RESPONSE)) {
		throw std::runtime_error(
			"SecureADS handshake: server response missing Response flag");
	}
	if (resp.error != 0) {
		throw std::runtime_error(
			"SecureADS handshake error code: " +
			std::to_string(static_cast<int>(resp.error)));
	}
}

// ===== Public interface =====

bool SecureAmsConnection::IsConnectedTo(
	const struct addrinfo *targetAddresses) const
{
	return m_Socket.IsConnectedTo(targetAddresses);
}

long SecureAmsConnection::AdsRequest(AmsRequest &request,
				     const uint32_t timeout)
{
	AmsAddr srcAddr;
	const auto status = m_Router.GetLocalAddress(request.port, &srcAddr);
	if (status) {
		return status;
	}
	request.SetDeadline(timeout);
	AmsResponse *response = Write(request, srcAddr);
	if (response) {
		const auto errorCode = response->Wait();
		response->Release();
		return errorCode;
	}
	return -1;
}

SharedDispatcher SecureAmsConnection::CreateNotifyMapping(
	uint32_t hNotify, std::shared_ptr<Notification> notification)
{
	auto dispatcher = DispatcherListAdd(notification->connection);
	notification->hNotify(hNotify);
	dispatcher->Emplace(hNotify, notification);
	return dispatcher;
}

long SecureAmsConnection::DeleteNotification(const AmsAddr &amsAddr,
					     uint32_t hNotify, uint32_t tmms,
					     uint16_t port)
{
	AmsRequest request{ amsAddr,
			    port,
			    AoEHeader::DEL_DEVICE_NOTIFICATION,
			    0,
			    nullptr,
			    nullptr,
			    sizeof(hNotify) };
	request.frame.prepend(bhf::ads::htole(hNotify));
	return AdsRequest(request, tmms);
}

// ===== Write — no AmsTcpHeader prepend =====

AmsResponse *SecureAmsConnection::Write(AmsRequest &request,
					const AmsAddr srcAddr)
{
	const AoEHeader aoeHeader{ request.destAddr.netId,
				   request.destAddr.port,
				   srcAddr.netId,
				   srcAddr.port,
				   request.cmdId,
				   static_cast<uint32_t>(request.frame.size()),
				   GetInvokeId() };
	request.frame.prepend<AoEHeader>(aoeHeader);

	auto response = Reserve(&request, srcAddr.port);
	if (!response) {
		return nullptr;
	}

	response->invokeId.store(aoeHeader.invokeId());
	if (request.frame.size() != m_Socket.write(request.frame)) {
		response->Release();
		return nullptr;
	}
	return response;
}

// ===== Recv — reads AoEHeader directly, no AmsTcpHeader =====

void SecureAmsConnection::Recv()
{
	AoEHeader aoeHeader;
	for (;;) {
		Receive(aoeHeader);

		if (aoeHeader.cmdId() == AoEHeader::DEVICE_NOTIFICATION) {
			ReceiveNotification(aoeHeader);
			continue;
		}

		auto response = GetPending(aoeHeader.invokeId(),
					   aoeHeader.targetPort());
		if (!response) {
			LOG_WARN("No response pending");
			ReceiveJunk(aoeHeader.length());
			continue;
		}

		switch (aoeHeader.cmdId()) {
		case AoEHeader::READ_DEVICE_INFO:
		case AoEHeader::WRITE:
		case AoEHeader::READ_STATE:
		case AoEHeader::WRITE_CONTROL:
		case AoEHeader::ADD_DEVICE_NOTIFICATION:
		case AoEHeader::DEL_DEVICE_NOTIFICATION:
			ReceiveFrame<AoEResponseHeader>(response,
							aoeHeader.length(),
							aoeHeader.errorCode());
			continue;

		case AoEHeader::READ:
		case AoEHeader::READ_WRITE:
			ReceiveFrame<AoEReadResponseHeader>(
				response, aoeHeader.length(),
				aoeHeader.errorCode());
			continue;

		default:
			LOG_WARN("Unknown AMS command id");
			response->Notify(ADSERR_CLIENT_SYNCRESINVALID);
			ReceiveJunk(aoeHeader.length());
		}
	}
}

void SecureAmsConnection::TryRecv()
{
	try {
		Recv();
	} catch (const std::runtime_error &e) {
		LOG_INFO(e.what());
	}
}

// ===== Receive helpers =====

void SecureAmsConnection::Receive(void *buffer, size_t bytesToRead,
				  timeval *timeout) const
{
	auto pos = reinterpret_cast<uint8_t *>(buffer);
	while (bytesToRead) {
		const size_t n = m_Socket.read(pos, bytesToRead, timeout);
		bytesToRead -= n;
		pos += n;
	}
}

void SecureAmsConnection::Receive(void *buffer, size_t bytesToRead,
				  const Timepoint &deadline) const
{
	const auto now = std::chrono::steady_clock::now();
	const auto usec = std::chrono::duration_cast<std::chrono::microseconds>(
				  deadline - now)
				  .count();
	if (usec <= 0) {
		throw TlsSocket::TimeoutEx("deadline reached already!!!");
	}
	timeval timeout{ (long)(usec / 1000000), (int)(usec % 1000000) };
	Receive(buffer, bytesToRead, &timeout);
}

void SecureAmsConnection::ReceiveJunk(size_t bytesToRead) const
{
	uint8_t buffer[1024];
	while (bytesToRead > sizeof(buffer)) {
		Receive(buffer, sizeof(buffer));
		bytesToRead -= sizeof(buffer);
	}
	Receive(buffer, bytesToRead);
}

template <class T>
void SecureAmsConnection::ReceiveFrame(AmsResponse *const response,
				       size_t bytesLeft,
				       uint32_t aoeError) const
{
	AmsRequest *const request = response->request.load();
	const auto responseId = response->invokeId.load();
	T header;

	if (aoeError) {
		response->Notify(aoeError);
		ReceiveJunk(bytesLeft);
		return;
	}
	if (bytesLeft > sizeof(header) + request->bufferLength) {
		LOG_WARN("Frame too long: "
			 << std::dec << bytesLeft << '>'
			 << sizeof(header) + request->bufferLength);
		response->Notify(ADSERR_DEVICE_INVALIDSIZE);
		ReceiveJunk(bytesLeft);
		return;
	}

	try {
		Receive(&header, sizeof(header), request->deadline);
		bytesLeft -= sizeof(header);
		Receive(request->buffer, bytesLeft, request->deadline);
		if (request->bytesRead) {
			*(request->bytesRead) =
				static_cast<std::remove_pointer<decltype(
					AmsRequest::bytesRead)>::type>(
					bytesLeft);
		}
		response->Notify(header.result());
	} catch (const TlsSocket::TimeoutEx &) {
		LOG_WARN("InvokeId " << std::dec << responseId << " timed out");
		response->Notify(ADSERR_CLIENT_SYNCTIMEOUT);
		ReceiveJunk(bytesLeft);
	}
}

bool SecureAmsConnection::ReceiveNotification(const AoEHeader &header)
{
	const auto dispatcher = DispatcherListGet(
		VirtualConnection{ header.targetPort(), header.sourceAms() });
	if (!dispatcher) {
		ReceiveJunk(header.length());
		LOG_WARN("No dispatcher found for notification");
		return false;
	}

	auto &ring = dispatcher->ring;
	auto bytesLeft = header.length();
	if (bytesLeft + sizeof(bytesLeft) > ring.BytesFree()) {
		ReceiveJunk(bytesLeft);
		LOG_WARN("port " << std::dec << header.targetPort()
				 << " receive buffer was full");
		return false;
	}

	for (size_t i = 0; i < sizeof(bytesLeft); ++i) {
		*ring.write = (bytesLeft >> (8 * i)) & 0xFF;
		ring.Write(1);
	}

	auto chunk = ring.WriteChunk();
	while (bytesLeft > chunk) {
		Receive(ring.write, chunk);
		ring.Write(chunk);
		bytesLeft -= static_cast<decltype(bytesLeft)>(chunk);
		chunk = ring.WriteChunk();
	}
	Receive(ring.write, bytesLeft);
	ring.Write(bytesLeft);
	dispatcher->Notify();
	return true;
}

// ===== Dispatcher helpers =====

SharedDispatcher
SecureAmsConnection::DispatcherListAdd(const VirtualConnection &connection)
{
	const auto dispatcher = DispatcherListGet(connection);
	if (dispatcher) {
		return dispatcher;
	}
	std::lock_guard<std::recursive_mutex> lock(m_DispatcherListMutex);
	return m_DispatcherList
		.emplace(connection,
			 std::make_shared<NotificationDispatcher>(std::bind(
				 &SecureAmsConnection::DeleteNotification, this,
				 connection.second, std::placeholders::_1,
				 std::placeholders::_2, connection.first)))
		.first->second;
}

SharedDispatcher
SecureAmsConnection::DispatcherListGet(const VirtualConnection &connection)
{
	std::lock_guard<std::recursive_mutex> lock(m_DispatcherListMutex);
	const auto it = m_DispatcherList.find(connection);
	if (it != m_DispatcherList.end()) {
		return it->second;
	}
	return {};
}

// ===== Invoke / queue helpers =====

uint32_t SecureAmsConnection::GetInvokeId()
{
	uint32_t result;
	do {
		result = m_InvokeId.fetch_add(1);
	} while (!result);
	return result;
}

AmsResponse *SecureAmsConnection::Reserve(AmsRequest *request,
					  const uint16_t port)
{
	AmsRequest *isFree = nullptr;
	if (!m_Queue[port - Router::PORT_BASE].request.compare_exchange_strong(
		    isFree, request)) {
		LOG_WARN("Port: " << port << " already in use as " << isFree);
		return nullptr;
	}
	return &m_Queue[port - Router::PORT_BASE];
}

AmsResponse *SecureAmsConnection::GetPending(const uint32_t id,
					     const uint16_t port)
{
	const uint16_t portIndex = port - Router::PORT_BASE;
	if (portIndex >= Router::NUM_PORTS_MAX) {
		LOG_WARN("Port 0x" << std::hex << port << " is out of range");
		return nullptr;
	}
	auto currentId = id;
	if (m_Queue[portIndex].invokeId.compare_exchange_strong(currentId, 0)) {
		return &m_Queue[portIndex];
	}
	LOG_WARN("InvokeId mismatch: waiting for 0x" << std::hex << currentId
						     << " received 0x" << id);
	return nullptr;
}
