// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "AdsNotification.h"
#include "AdsSyntheticNotification.h"
#include "AmsHeader.h"
#include "Semaphore.h"

#include <atomic>
#include <functional>
#include <map>
#include <set>
#include <thread>
#include <vector>

using DeleteNotificationCallback =
	std::function<long(uint32_t hNotify, uint32_t tmms)>;

struct NotificationDispatcher {
	NotificationDispatcher(DeleteNotificationCallback callback);
	~NotificationDispatcher();
	void Emplace(uint32_t hNotify,
		     std::shared_ptr<Notification> notification);
	long Erase(uint32_t hNotify, uint32_t tmms);
	void EmplaceSynthetic(uint32_t hNotify,
		     std::shared_ptr<SyntheticNotification> notification);
	long EraseSynthetic(uint32_t hNotify);
	void Notify();
	void Run();

	const DeleteNotificationCallback deleteNotification;
	RingBuffer ring;

    private:
	std::map<uint32_t, std::shared_ptr<Notification> > notifications;
	std::map<uint32_t, std::shared_ptr<SyntheticNotification> > syntheticNotifications;
	std::recursive_mutex mutex;
	Semaphore sem;
	std::atomic<bool> stopExecution;
	std::thread thread;

	std::shared_ptr<Notification> Find(uint32_t hNotify);
	std::vector<std::shared_ptr<SyntheticNotification>> FindSynthetic(
		const std::set<VirtualConnection>& connections, uint32_t type);
};
using SharedDispatcher = std::shared_ptr<NotificationDispatcher>;
