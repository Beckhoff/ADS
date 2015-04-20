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
        if (!(pAddr)) { \
            return ADSERR_CLIENT_NOAMSADDR; \
        } \
} while (false)

long AdsAddRoute(const AmsNetId ams, const char* ip)
{
    return router.AddRoute(ams, IpV4(ip));
}

void AdsDelRoute(const AmsNetId ams)
{
    router.DelRoute(ams);
}

void AdsSetNetId(AmsNetId ams)
{
    router.SetNetId(ams);
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

long AdsSyncReadReqEx2(long           port,
                       const AmsAddr* pAddr,
                       uint32_t       indexGroup,
                       uint32_t       indexOffset,
                       uint32_t       bufferLength,
                       void*          buffer,
                       uint32_t*      bytesRead)
{
    ASSERT_PORT_AND_AMSADDR(port, pAddr);
    if (!buffer) {
        return ADSERR_CLIENT_INVALIDPARM;
    }

    AmsRequest request {
        *pAddr,
        (uint16_t)port,
        AoEHeader::READ,
        bufferLength,
        buffer,
        bytesRead,
        sizeof(AoERequestHeader)
    };
    request.frame.prepend(AoERequestHeader {
        indexGroup,
        indexOffset,
        bufferLength
    });
    return router.AdsRequest<AoEReadResponseHeader>(request);
}

long AdsSyncReadDeviceInfoReqEx(long port, const AmsAddr* pAddr, char* devName, AdsVersion* version)
{
    ASSERT_PORT_AND_AMSADDR(port, pAddr);
    if (!devName || !version) {
        return ADSERR_CLIENT_INVALIDPARM;
    }

    static const size_t NAME_LENGTH = 16;
    uint8_t buffer[sizeof(*version) + NAME_LENGTH];
    AmsRequest request {
        *pAddr,
        (uint16_t)port,
        AoEHeader::READ_DEVICE_INFO,
        sizeof(buffer),
        buffer
    };
    const auto status = router.AdsRequest<AoEResponseHeader>(request);
    if (!status) {
        version->version = buffer[0];
        version->revision = buffer[1];
        version->build = qFromLittleEndian<uint16_t>(buffer + offsetof(AdsVersion, build));
        memcpy(devName, buffer + sizeof(*version), NAME_LENGTH);
    }
    return status;
}

long AdsSyncReadStateReqEx(long port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* devState)
{
    ASSERT_PORT_AND_AMSADDR(port, pAddr);
    if (!adsState || !devState) {
        return ADSERR_CLIENT_INVALIDPARM;
    }

    uint8_t buffer[sizeof(*adsState) + sizeof(*devState)];
    AmsRequest request {
        *pAddr,
        (uint16_t)port,
        AoEHeader::READ_STATE,
        sizeof(buffer),
        buffer
    };
    const auto status = router.AdsRequest<AoEResponseHeader>(request);
    if (!status) {
        *adsState = qFromLittleEndian<uint16_t>(buffer);
        *devState = qFromLittleEndian<uint16_t>(buffer + sizeof(*adsState));
    }
    return status;
}

long AdsSyncReadWriteReqEx2(long           port,
                            const AmsAddr* pAddr,
                            uint32_t       indexGroup,
                            uint32_t       indexOffset,
                            uint32_t       readLength,
                            void*          readData,
                            uint32_t       writeLength,
                            const void*    writeData,
                            uint32_t*      bytesRead)
{
    ASSERT_PORT_AND_AMSADDR(port, pAddr);
    if (!readData || !writeData) {
        return ADSERR_CLIENT_INVALIDPARM;
    }

    AmsRequest request {
        *pAddr,
        (uint16_t)port,
        AoEHeader::READ_WRITE,
        readLength,
        readData,
        bytesRead,
        sizeof(AoEReadWriteReqHeader) + writeLength
    };
    request.frame.prepend(writeData, writeLength);
    request.frame.prepend(AoEReadWriteReqHeader {
        indexGroup,
        indexOffset,
        readLength,
        writeLength
    });
    return router.AdsRequest<AoEReadResponseHeader>(request);
}

long AdsSyncWriteReqEx(long           port,
                       const AmsAddr* pAddr,
                       uint32_t       indexGroup,
                       uint32_t       indexOffset,
                       uint32_t       bufferLength,
                       const void*    buffer)
{
    ASSERT_PORT_AND_AMSADDR(port, pAddr);
    if (!buffer) {
        return ADSERR_CLIENT_INVALIDPARM;
    }

    AmsRequest request {
        *pAddr,
        (uint16_t)port,
        AoEHeader::WRITE,
        0, nullptr, nullptr,
        sizeof(AoERequestHeader) + bufferLength,
    };
    request.frame.prepend(buffer, bufferLength);
    request.frame.prepend<AoERequestHeader>({
        indexGroup,
        indexOffset,
        bufferLength
    });
    return router.AdsRequest<AoEReadResponseHeader>(request);
}

long AdsSyncWriteControlReqEx(long           port,
                              const AmsAddr* pAddr,
                              uint16_t       adsState,
                              uint16_t       devState,
                              uint32_t       bufferLength,
                              const void*    buffer)
{
    ASSERT_PORT_AND_AMSADDR(port, pAddr);
    AmsRequest request {
        *pAddr,
        (uint16_t)port,
        AoEHeader::WRITE_CONTROL,
        0, nullptr, nullptr,
        sizeof(AdsWriteCtrlRequest) + bufferLength
    };
    request.frame.prepend(buffer, bufferLength);
    request.frame.prepend<AdsWriteCtrlRequest>({
        adsState,
        devState,
        bufferLength
    });
    return router.AdsRequest<AoEResponseHeader>(request);
}

long AdsSyncAddDeviceNotificationReqEx(long                         port,
                                       const AmsAddr*               pAddr,
                                       uint32_t                     indexGroup,
                                       uint32_t                     indexOffset,
                                       const AdsNotificationAttrib* pAttrib,
                                       PAdsNotificationFuncEx       pFunc,
                                       uint32_t                     hUser,
                                       uint32_t*                    pNotification)
{
    ASSERT_PORT_AND_AMSADDR(port, pAddr);
    if (!pAttrib || !pFunc || !pNotification) {
        return ADSERR_CLIENT_INVALIDPARM;
    }
    return router.AddNotification((uint16_t)port, pAddr, indexGroup, indexOffset, pAttrib, pFunc, hUser, pNotification);
}

long AdsSyncDelDeviceNotificationReqEx(long port, const AmsAddr* pAddr, uint32_t hNotification)
{
    ASSERT_PORT_AND_AMSADDR(port, pAddr);
    return router.DelNotification((uint16_t)port, pAddr, hNotification);
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
