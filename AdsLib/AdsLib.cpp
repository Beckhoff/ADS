
#include "AdsLib.h"
#include "AmsRouter.h"

static AmsRouter router;

long AdsAddRoute(const AmsNetId ams, const IpV4 ip)
{
	return router.AddRoute(ams, ip);
}

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

long AdsSyncReadReqEx2(long port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, void* buffer, uint32_t *bytesRead)
{
	if (port <= 0 || port > UINT16_MAX) {
		return ROUTERERR_NOTREGISTERED;
	}
	if (!pAddr || !buffer || !bytesRead || !bufferLength) {
		return ADSERR_DEVICE_INVALIDPARM;
	}
	return router.Read((uint16_t)port, pAddr, indexGroup, indexOffset, bufferLength, buffer, bytesRead);
}