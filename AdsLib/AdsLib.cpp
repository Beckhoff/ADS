
#include "AdsLib.h"
#include "AmsRouter.h"

static AmsRouter router;

long AdsPortCloseEx(long port)
{
	if (port <= 0 || port > UINT16_MAX) {
		return ROUTERERR_NOTREGISTERED;
	}
	return router.ClosePort((uint16_t)port);
}

long AdsPortOpenEx()
{
	return router.OpenPort();
}

long AdsGetLocalAddressEx(long port, AmsAddr* pAddr)
{
	if (port <= 0 || port > UINT16_MAX) {
		return ROUTERERR_NOTREGISTERED;
	}
	if (!pAddr) {
		return ADSERR_DEVICE_INVALIDPARM;
	}
	return router.GetLocalAddress((uint16_t)port, pAddr);
}