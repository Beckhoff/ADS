#include "AmsPort.h"

namespace std {
	bool operator==(const AmsAddr& lhs, const AmsAddr& rhs)
	{
		return 0 == memcmp(&lhs, &rhs, sizeof(lhs));
	}
}

AmsPort::AmsPort()
	: tmms(DEFAULT_TIMEOUT),
	port(0)
{}

void AmsPort::AddNotification(NotificationId hash)
{
	std::lock_guard<std::mutex> lock(mutex);
	notifications.insert(hash);
}

void AmsPort::Close()
{
	std::lock_guard<std::mutex> lock(mutex);

	auto it = std::begin(notifications);
	while (it != std::end(notifications)) {
		it->second->Erase(it->first, tmms);
		it = notifications.erase(it);
	}
	port = 0;
}

bool AmsPort::DelNotification(const AmsAddr &ams, uint32_t hNotify)
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto it = notifications.begin(); it != notifications.end(); ++it) {
		if (it->first == hNotify) {
			if (std::ref(it->second->amsAddr) == ams) {
				it->second->Erase(hNotify, tmms);
				notifications.erase(it);
				return true;
			}
		}
	}
	return false;
}

bool AmsPort::IsOpen() const
{
	return !!port;
}

uint16_t AmsPort::Open(uint16_t __port)
{
	port = __port;
	return port;
}