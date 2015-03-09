
#include "AdsLib.h"
#include "AmsRouter.h"

static AmsRouter router;

#define ASSERT_PORT(port) \
	do { \
		if ((port) <= 0 || (port) > UINT16_MAX) { \
			return ROUTERERR_NOTREGISTERED; \
				} \
	} while (0)

long AdsAddRoute(const AmsNetId ams, const IpV4 ip)
{
	return router.AddRoute(ams, ip);
}

long AdsPortCloseEx(long port)
{
	ASSERT_PORT(port);
	return router.ClosePort((uint16_t)port);
}

long AdsPortOpenEx()
{
	return router.OpenPort();
}

long AdsGetLocalAddressEx(long port, AmsAddr* pAddr)
{
	ASSERT_PORT(port);
	if (!pAddr) {
		return ADSERR_DEVICE_INVALIDPARM;
	}
	return router.GetLocalAddress((uint16_t)port, pAddr);
}

long AdsSyncReadReqEx2(long port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, void* buffer, uint32_t *bytesRead)
{
	ASSERT_PORT(port);
	if (!pAddr || !buffer || !bytesRead || !bufferLength) {
		return ADSERR_DEVICE_INVALIDPARM;
	}
	return router.Read((uint16_t)port, pAddr, indexGroup, indexOffset, bufferLength, buffer, bytesRead);
}

long AdsSyncReadDeviceInfoReqEx(long port, const AmsAddr* pAddr, char* devName, AdsVersion* version)
{
	ASSERT_PORT(port);
	if (!pAddr || !devName || !version) {
		return ADSERR_DEVICE_INVALIDPARM;
	}
	return router.ReadDeviceInfo((uint16_t)port, pAddr, devName, version);
}

long AdsSyncReadStateReqEx(long port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* deviceState)
{
	ASSERT_PORT(port);
	if (!pAddr || !adsState || !deviceState) {
		return ADSERR_DEVICE_INVALIDPARM;
	}
	return router.ReadState((uint16_t)port, pAddr, adsState, deviceState);
}