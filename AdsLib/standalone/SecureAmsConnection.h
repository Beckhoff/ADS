// SPDX-License-Identifier: MIT
#pragma once

#include "AmsConnection.h"
#include "SecureAdsConfig.h"
#include "TlsSocket.h"

#include <atomic>
#include <map>
#include <memory>
#include <thread>

struct SecureAmsConnection : AmsConnectionBase {
	SecureAmsConnection(Router &router, const struct addrinfo *destination,
			    const bhf::ads::SecureAdsConfig &config,
			    const AmsNetId &localNetId);
	~SecureAmsConnection();

	bool
	IsConnectedTo(const struct addrinfo *targetAddresses) const override;
	long AdsRequest(AmsRequest &request, uint32_t timeout) override;
	SharedDispatcher CreateNotifyMapping(
		uint32_t hNotify,
		std::shared_ptr<Notification> notification) override;
	long DeleteNotification(const AmsAddr &amsAddr, uint32_t hNotify,
				uint32_t tmms, uint16_t port) override;

    private:
	friend struct AmsRouter;
	Router &m_Router;
	TlsSocket m_Socket;
	std::thread m_Receiver;
	std::atomic<uint32_t> m_InvokeId{ 0 };
	std::array<AmsResponse, Router::NUM_PORTS_MAX> m_Queue;

	void Handshake(const bhf::ads::SecureAdsConfig &config,
		       const AmsNetId &localNetId);

	template <class T>
	void ReceiveFrame(AmsResponse *response, size_t bytesLeft,
			  uint32_t aoeError) const;
	bool ReceiveNotification(const AoEHeader &header);
	void ReceiveJunk(size_t bytesToRead) const;
	void Receive(void *buffer, size_t bytesToRead,
		     timeval *timeout = nullptr) const;
	void Receive(void *buffer, size_t bytesToRead,
		     const Timepoint &deadline) const;
	template <class T> void Receive(T &buffer) const
	{
		Receive(&buffer, sizeof(T));
	}
	AmsResponse *Write(AmsRequest &request, AmsAddr srcAddr);
	void Recv();
	void TryRecv();
	uint32_t GetInvokeId();
	AmsResponse *Reserve(AmsRequest *request, uint16_t port);
	AmsResponse *GetPending(uint32_t id, uint16_t port);

	std::map<VirtualConnection, SharedDispatcher> m_DispatcherList;
	std::recursive_mutex m_DispatcherListMutex;
	SharedDispatcher DispatcherListAdd(const VirtualConnection &connection);
	SharedDispatcher DispatcherListGet(const VirtualConnection &connection);
};
