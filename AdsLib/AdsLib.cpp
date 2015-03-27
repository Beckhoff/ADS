
#include "AdsLib.h"
#include "AmsRouter.h"

static AmsRouter router;

#define ASSERT_PORT(port) do { \
	if ((port) <= 0 || (port) > UINT16_MAX) { \
		return ADSERR_CLIENT_PORTNOTOPEN; \
		} \
} while (false)

#define ASSERT_PORT_AND_AMSADDR(port, pAddr) do { \
	ASSERT_PORT(port); \
	if (!pAddr) { \
		return ADSERR_CLIENT_NOAMSADDR; \
	} \
} while (false)

long AdsAddRoute(const AmsNetId ams, const IpV4 ip)
{
	return router.AddRoute(ams, ip);
}

void AdsDelRoute(const AmsNetId ams)
{
	router.DelRoute(ams);
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
	ASSERT_PORT_AND_AMSADDR(port, pAddr);
	return router.GetLocalAddress((uint16_t)port, pAddr);
}

long AdsSyncReadReqEx2(long port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, void* buffer, uint32_t *bytesRead)
{
	ASSERT_PORT_AND_AMSADDR(port, pAddr);
	if (!buffer) {
		return ADSERR_CLIENT_INVALIDPARM;
	}
	return router.Read((uint16_t)port, pAddr, indexGroup, indexOffset, bufferLength, buffer, bytesRead);
}

long AdsSyncReadDeviceInfoReqEx(long port, const AmsAddr* pAddr, char* devName, AdsVersion* version)
{
	ASSERT_PORT_AND_AMSADDR(port, pAddr);
	if (!devName || !version) {
		return ADSERR_CLIENT_INVALIDPARM;
	}
	return router.ReadDeviceInfo((uint16_t)port, pAddr, devName, version);
}

long AdsSyncReadStateReqEx(long port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* devState)
{
	ASSERT_PORT_AND_AMSADDR(port, pAddr);
	if (!adsState || !devState) {
		return ADSERR_CLIENT_INVALIDPARM;
	}
	return router.ReadState((uint16_t)port, pAddr, adsState, devState);
}

long AdsSyncReadWriteReqEx2(long port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t readLength, void* readData, uint32_t writeLength, const void* writeData, uint32_t *bytesRead)
{
	ASSERT_PORT_AND_AMSADDR(port, pAddr);
	if (!readData || !writeData) {
		return ADSERR_CLIENT_INVALIDPARM;
	}
	return router.ReadWrite((uint16_t)port, pAddr, indexGroup, indexOffset, readLength, readData, writeLength, writeData, bytesRead);
}

long AdsSyncWriteReqEx(long port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, const void* buffer)
{
	ASSERT_PORT_AND_AMSADDR(port, pAddr);
	if (!buffer) {
		return ADSERR_CLIENT_INVALIDPARM;
	}
	return router.Write((uint16_t)port, pAddr, indexGroup, indexOffset, bufferLength, buffer);
}

long AdsSyncWriteControlReqEx(long port, const AmsAddr* pAddr, uint16_t adsState, uint16_t devState, uint32_t bufferLength, const void* buffer)
{
	ASSERT_PORT_AND_AMSADDR(port, pAddr);
	return router.WriteControl((uint16_t)port, pAddr, adsState, devState, bufferLength, buffer);
}

long AdsSyncAddDeviceNotificationReqEx(long port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, const AdsNotificationAttrib* pAttrib, PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t *pNotification)
{
	ASSERT_PORT_AND_AMSADDR(port, pAddr);
	if (!pAttrib || !pFunc || !pNotification) {
		return ADSERR_CLIENT_INVALIDPARM;
	}
	return router.AddNotification(port, pAddr, indexGroup, indexOffset, pAttrib, pFunc, hUser, pNotification);
}

long AdsSyncDelDeviceNotificationReqEx(long port, const AmsAddr* pAddr, uint32_t hNotification)
{
	ASSERT_PORT_AND_AMSADDR(port, pAddr);
	return router.DelNotification(port, pAddr, hNotification);
}

long AdsSyncGetTimeoutEx(long port, uint32_t* timeout)
{
	ASSERT_PORT(port);
	if (!timeout) {
		return ADSERR_CLIENT_INVALIDPARM;
	}
	return router.GetTimeout((uint16_t)port, *timeout);
}

long AdsSyncSetTimeoutEx(long port, uint32_t timeout)
{
	ASSERT_PORT(port);
	return router.SetTimeout((uint16_t)port, timeout);
}
