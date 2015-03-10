
#include "AdsLib.h"
#include "AmsRouter.h"

static AmsRouter router;

#define ASSERT_PORT(port) \
	do { \
		if ((port) <= 0 || (port) > UINT16_MAX) { \
			return ADSERR_CLIENT_PORTNOTOPEN; \
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

long AdsSyncReadStateReqEx(long port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* devState)
{
	ASSERT_PORT(port);
	if (!pAddr || !adsState || !devState) {
		return ADSERR_DEVICE_INVALIDPARM;
	}
	return router.ReadState((uint16_t)port, pAddr, adsState, devState);
}

long AdsSyncWriteReqEx(long port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, const void* buffer)
{
	ASSERT_PORT(port);
	if (!pAddr || !buffer || !bufferLength) {
		return ADSERR_DEVICE_INVALIDPARM;
	}
	return router.Write((uint16_t)port, pAddr, indexGroup, indexOffset, bufferLength, buffer);
}

long AdsSyncWriteControlReqEx(long port, const AmsAddr* pAddr, uint16_t adsState, uint16_t devState, uint32_t bufferLength, const void* buffer)
{
	ASSERT_PORT(port);
	if (!pAddr) {
		return ADSERR_DEVICE_INVALIDPARM;
	}
	return router.WriteControl((uint16_t)port, pAddr, adsState, devState, bufferLength, buffer);
}

long AdsSyncGetTimeoutEx(long port, uint32_t* timeout)
{
	ASSERT_PORT(port);
	if (!timeout) {
		return ADSERR_DEVICE_INVALIDPARM;
	}
	return router.GetTimeout((uint16_t)port, *timeout);
}

long AdsSyncSetTimeoutEx(long port, uint32_t timeout)
{
	ASSERT_PORT(port);
	return router.SetTimeout((uint16_t)port, timeout);
}