#include "AmsRouter.h"

AmsRouter::AmsRouter()
{
}

uint16_t AmsRouter::OpenPort()
{
	return 0;
}

long AmsRouter::ClosePort(uint16_t port)
{
	return ROUTERERR_NOTREGISTERED;
}

long AmsRouter::GetLocalAddress(uint16_t port, AmsAddr* pAddr)
{
	return ROUTERERR_NOTREGISTERED;
}

