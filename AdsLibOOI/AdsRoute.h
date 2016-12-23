#pragma once
#include "AdsException.h"
#include "AdsHandle.h"
#include "AdsLib/AdsDef.h"

template<class T>
struct ResourceDeleter {
    ResourceDeleter(const std::function<void(T)> func)
        : FreeResource(func)
    {}

    void operator()(T* resource)
    {
        FreeResource(*resource);
        delete resource;
    }
private:
    const std::function<void(T)> FreeResource;
};
template<typename T>
using AdsResource = std::unique_ptr<T, ResourceDeleter<T> >;

struct AdsRoute {
    AdsRoute(const std::string& ipV4, AmsNetId netId, uint16_t port);

    /** Get handle to access AdsVariable by indexGroup/Offset */
    AdsHandle GetHandle(uint32_t offset) const;

    /** Get handle for access by symbol name */
    AdsHandle GetHandle(const std::string& symbolName) const;

    /** Get notification handle */
    AdsHandle GetHandle(uint32_t                     indexGroup,
                        uint32_t                     indexOffset,
                        const AdsNotificationAttrib& notificationAttributes,
                        PAdsNotificationFuncEx       callback) const;

    long GetLocalPort() const;
    void SetTimeout(const uint32_t timeout) const;
    uint32_t GetTimeout() const;

    long ReadReqEx2(uint32_t group, uint32_t offset, uint32_t length, void* buffer, uint32_t* bytesRead) const;
    long ReadWriteReqEx2(uint32_t    indexGroup,
                         uint32_t    indexOffset,
                         uint32_t    readLength,
                         void*       readData,
                         uint32_t    writeLength,
                         const void* writeData,
                         uint32_t*   bytesRead) const;
    long WriteReqEx(uint32_t group, uint32_t offset, uint32_t length, const void* buffer) const;

    AdsResource<const AmsNetId> m_NetId;
    const AmsAddr m_Addr;
private:
    AdsResource<const long> m_LocalPort;
};
