
#include "AmsPort.h"
#include <cstring>

AmsPort::AmsPort(const AmsNetId *__localAddr)
	: tmms(DEFAULT_TIMEOUT),
	port(0),
	localAddr(__localAddr)
{}

void AmsPort::operator=(const AmsPort &ref)
{
	memcpy(this, &ref, sizeof(*this));
}

void AmsPort::AddNotification(size_t hash)
{
	std::lock_guard<std::mutex> lock(mutex);
	notifications.insert(hash);
}

void AmsPort::Close()
{
	std::lock_guard<std::mutex> lock(mutex);
	notifications.clear();
	port = 0;
}

void AmsPort::DelNotification(size_t hash)
{
	std::lock_guard<std::mutex> lock(mutex);
	notifications.erase(hash);
}

long AmsPort::GetLocalAddress(AmsAddr* pAddr) const
{
	if (IsOpen()) {
		memcpy(&pAddr->netId, localAddr, sizeof(pAddr->netId));
		pAddr->port = port;
		return 0;
	}
	return ADSERR_CLIENT_PORTNOTOPEN;
}

bool AmsPort::IsOpen() const
{
	return port;
}

uint16_t AmsPort::Open(uint16_t __port)
{
	port = __port;
	return port;
}