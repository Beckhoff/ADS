// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "AmsConnection.h"
#include "SecureAdsConfig.h"
#include <unordered_set>

struct AmsRouter : Router {
	AmsRouter(AmsNetId netId = AmsNetId{});

	uint16_t OpenPort();
	long ClosePort(uint16_t port);
	long GetLocalAddress(uint16_t port, AmsAddr *pAddr);
	void SetLocalAddress(AmsNetId netId);
	long GetTimeout(uint16_t port, uint32_t &timeout);
	long SetTimeout(uint16_t port, uint32_t timeout);
	long AddNotification(AmsRequest &request, uint32_t *pNotification,
			     std::shared_ptr<Notification> notify);
	long DelNotification(uint16_t port, const AmsAddr *pAddr,
			     uint32_t hNotification);

	[[deprecated]]
	long AddRoute(AmsNetId ams, const IpV4 &ip);
	long AddRoute(AmsNetId ams, const std::string &host);
	long AddSecureRoute(AmsNetId ams, const std::string &host,
			    const bhf::ads::SecureAdsConfig &config);
	void DelRoute(const AmsNetId &ams);
	AmsConnectionBase *GetConnection(const AmsNetId &pAddr);
	long AdsRequest(AmsRequest &request);

    private:
	AmsNetId localAddr;
	std::recursive_mutex mutex;
	std::condition_variable_any connection_attempt_events;
	std::map<AmsNetId, std::tuple<> > connection_attempts;
	std::unordered_set<std::unique_ptr<AmsConnectionBase> > connections;
	std::map<AmsNetId, AmsConnectionBase *> mapping;

	void
	AwaitConnectionAttempts(const AmsNetId &ams,
				std::unique_lock<std::recursive_mutex> &lock);
	void DeleteIfLastConnection(const AmsConnectionBase *conn);

	std::array<AmsPort, NUM_PORTS_MAX> ports;
};
