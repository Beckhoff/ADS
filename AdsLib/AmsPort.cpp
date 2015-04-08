
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

void AmsPort::Close()
{
	port = 0;
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