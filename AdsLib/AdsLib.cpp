/**
   Copyright (c) 2015 Beckhoff Automation GmbH & Co. KG

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
 */

#include "AdsLib.h"
#include "AmsRouter.h"

static AmsRouter& GetRouter()
{
    static AmsRouter router;
    return router;
}

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
    try {
        return GetRouter().AddRoute(ams, IpV4(ip));
    } catch (const std::bad_alloc&) {
        return GLOBALERR_NO_MEMORY;
    } catch (const std::runtime_error&) {
        return GLOBALERR_TARGET_PORT;
    }
}

void AdsDelRoute(const AmsNetId ams)
{
    GetRouter().DelRoute(ams);
}

long AdsPortCloseEx(long port)
{
    ASSERT_PORT(port);
    return GetRouter().ClosePort((uint16_t)port);
}

long AdsPortOpenEx()
{
    return GetRouter().OpenPort();
}

long AdsGetLocalAddressEx(long port, AmsAddr* pAddr)
{
    ASSERT_PORT_AND_AMSADDR(port, pAddr);
    return GetRouter().GetLocalAddress((uint16_t)port, pAddr);
}

void AdsSetLocalAddress(const AmsNetId ams)
{
    GetRouter().SetLocalAddress(ams);
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

    try {
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
        return GetRouter().AdsRequest(request);
    } catch (const std::bad_alloc&) {
        return GLOBALERR_NO_MEMORY;
    }
}

long AdsSyncReadDeviceInfoReqEx(long port, const AmsAddr* pAddr, char* devName, AdsVersion* version)
{
    ASSERT_PORT_AND_AMSADDR(port, pAddr);
    if (!devName || !version) {
        return ADSERR_CLIENT_INVALIDPARM;
    }

    try {
        static const size_t NAME_LENGTH = 16;
        uint8_t buffer[sizeof(*version) + NAME_LENGTH];
        AmsRequest request {
            *pAddr,
            (uint16_t)port,
            AoEHeader::READ_DEVICE_INFO,
            sizeof(buffer),
            buffer
        };
        const auto status = GetRouter().AdsRequest(request);
        if (!status) {
            version->version = buffer[0];
            version->revision = buffer[1];
            version->build = qFromLittleEndian<uint16_t>(buffer + offsetof(AdsVersion, build));
            memcpy(devName, buffer + sizeof(*version), NAME_LENGTH);
        }
        return status;
    } catch (const std::bad_alloc&) {
        return GLOBALERR_NO_MEMORY;
    }
}

long AdsSyncReadStateReqEx(long port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* devState)
{
    ASSERT_PORT_AND_AMSADDR(port, pAddr);
    if (!adsState || !devState) {
        return ADSERR_CLIENT_INVALIDPARM;
    }

    try {
        uint8_t buffer[sizeof(*adsState) + sizeof(*devState)];
        AmsRequest request {
            *pAddr,
            (uint16_t)port,
            AoEHeader::READ_STATE,
            sizeof(buffer),
            buffer
        };
        const auto status = GetRouter().AdsRequest(request);
        if (!status) {
            *adsState = qFromLittleEndian<uint16_t>(buffer);
            *devState = qFromLittleEndian<uint16_t>(buffer + sizeof(*adsState));
        }
        return status;
    } catch (const std::bad_alloc&) {
        return GLOBALERR_NO_MEMORY;
    }
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
    if ((readLength && !readData) || (writeLength && !writeData)) {
        return ADSERR_CLIENT_INVALIDPARM;
    }

    try {
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
        return GetRouter().AdsRequest(request);
    } catch (const std::bad_alloc&) {
        return GLOBALERR_NO_MEMORY;
    }
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

    try {
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
        return GetRouter().AdsRequest(request);
    } catch (const std::bad_alloc&) {
        return GLOBALERR_NO_MEMORY;
    }
}

long AdsSyncWriteControlReqEx(long           port,
                              const AmsAddr* pAddr,
                              uint16_t       adsState,
                              uint16_t       devState,
                              uint32_t       bufferLength,
                              const void*    buffer)
{
    ASSERT_PORT_AND_AMSADDR(port, pAddr);
    try {
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
        return GetRouter().AdsRequest(request);
    } catch (const std::bad_alloc&) {
        return GLOBALERR_NO_MEMORY;
    }
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

    try {
        uint8_t buffer[sizeof(*pNotification)];
        AmsRequest request {
            *pAddr,
            (uint16_t)port,
            AoEHeader::ADD_DEVICE_NOTIFICATION,
            sizeof(buffer),
            buffer,
            nullptr,
            sizeof(AdsAddDeviceNotificationRequest)
        };
        request.frame.prepend(AdsAddDeviceNotificationRequest {
            indexGroup,
            indexOffset,
            pAttrib->cbLength,
            pAttrib->nTransMode,
            pAttrib->nMaxDelay,
            pAttrib->nCycleTime
        });
        auto notify = std::make_shared<Notification>(pFunc, hUser, pAttrib->cbLength, *pAddr, (uint16_t)port);
        return GetRouter().AddNotification(
            request,
            pNotification,
            notify);
    } catch (const std::bad_alloc&) {
        return GLOBALERR_NO_MEMORY;
    }
}

long AdsSyncDelDeviceNotificationReqEx(long port, const AmsAddr* pAddr, uint32_t hNotification)
{
    ASSERT_PORT_AND_AMSADDR(port, pAddr);
    return GetRouter().DelNotification((uint16_t)port, pAddr, hNotification);
}

long AdsSyncGetTimeoutEx(long port, uint32_t* timeout)
{
    ASSERT_PORT(port);
    if (!timeout) {
        return ADSERR_CLIENT_INVALIDPARM;
    }
    return GetRouter().GetTimeout((uint16_t)port, *timeout);
}

long AdsSyncSetTimeoutEx(long port, uint32_t timeout)
{
    ASSERT_PORT(port);
    return GetRouter().SetTimeout((uint16_t)port, timeout);
}
