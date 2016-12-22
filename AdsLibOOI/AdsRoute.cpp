#include "AdsRoute.h"
#include "AdsException.h"
#include "AdsLib/AdsLib.h"

static void CloseLocalPort(const long* port)
{
    AdsPortCloseEx(*port);
    delete port;
}

static void DeleteRoute(AmsNetId* pNetId)
{
    AdsDelRoute(*pNetId);
    delete pNetId;
}

static AmsNetId* AddRoute(AmsNetId ams, const char* ip)
{
    const auto error = AdsAddRoute(ams, ip);
    if (error) {
        throw AdsException(error);
    }
    return new AmsNetId {ams};
}

AdsRoute::AdsRoute(const std::string& ipV4, AmsNetId netId, uint16_t port)
    : m_NetId(AddRoute(netId, ipV4.c_str()), DeleteRoute),
    m_Port({netId, port}),
    m_LocalPort(new long { AdsPortOpenEx() }, CloseLocalPort)
{}

AdsHandle AdsRoute::GetHandle(const uint32_t offset) const
{
    return {new uint32_t {offset}, HandleDeleter {}};
}

AdsHandle AdsRoute::GetHandle(const std::string& symbolName) const
{
    uint32_t handle = 0;
    uint32_t bytesRead = 0;
    uint32_t error = ReadWriteReqEx2(
        ADSIGRP_SYM_HNDBYNAME, 0,
        sizeof(handle), &handle,
        symbolName.size(),
        symbolName.c_str(),
        &bytesRead
        );

    if (error || (sizeof(handle) != bytesRead)) {
        throw AdsException(error);
    }

    return AdsHandle {new uint32_t {handle}, SymbolHandleDeleter {*this}};
}

long AdsRoute::GetLocalPort() const
{
    return *m_LocalPort;
}

void AdsRoute::SetTimeout(const uint32_t timeout) const
{
    const auto error = AdsSyncSetTimeoutEx(GetLocalPort(), timeout);
    if (error) {
        throw AdsException(error);
    }
}

uint32_t AdsRoute::GetTimeout() const
{
    uint32_t timeout = 0;
    const auto error = AdsSyncGetTimeoutEx(GetLocalPort(), &timeout);
    if (error) {
        throw AdsException(error);
    }
    return timeout;
}

long AdsRoute::ReadReqEx2(uint32_t group, uint32_t offset, uint32_t length, void* buffer, uint32_t& bytesRead) const
{
    return AdsSyncReadReqEx2(*m_LocalPort, &m_Port, group, offset, length, buffer, &bytesRead);
}

long AdsRoute::ReadWriteReqEx2(uint32_t    indexGroup,
                               uint32_t    indexOffset,
                               uint32_t    readLength,
                               void*       readData,
                               uint32_t    writeLength,
                               const void* writeData,
                               uint32_t*   bytesRead) const
{
    return AdsSyncReadWriteReqEx2(GetLocalPort(),
                                  &m_Port,
                                  indexGroup, indexOffset,
                                  readLength, readData,
                                  writeLength, writeData,
                                  bytesRead
                                  );
}

long AdsRoute::WriteReqEx(uint32_t group, uint32_t offset, uint32_t length, const void* buffer) const
{
    return AdsSyncWriteReqEx(GetLocalPort(), &m_Port, group, offset, length, buffer);
}
