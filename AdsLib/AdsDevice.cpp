// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2016 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "AdsDevice.h"
#include "AdsException.h"
#include "AdsLib.h"
static AmsNetId* AddRoute(AmsNetId ams, const char* ip)
{
    const auto error = bhf::ads::AddLocalRoute(ams, ip);
    if (error) {
        throw AdsException(error);
    }
    return new AmsNetId {ams};
}

AdsDevice::AdsDevice(const std::string& ipV4, AmsNetId netId, uint16_t port)
    : m_NetId(AddRoute(netId, ipV4.c_str()), {[](AmsNetId ams){bhf::ads::DelLocalRoute(ams); return 0; }}),
    m_Addr({netId, port}),
    m_LocalPort(new long { AdsPortOpenEx() }, {AdsPortCloseEx})
{}

long AdsDevice::DeleteNotificationHandle(uint32_t handle) const
{
    if (handle) {
        return AdsSyncDelDeviceNotificationReqEx(GetLocalPort(), &m_Addr, handle);
    }
    return 0;
}

long AdsDevice::CloseFile(uint32_t handle) const
{
    return ReadWriteReqEx2(SYSTEMSERVICE_FCLOSE, handle, 0, nullptr, 0, nullptr, nullptr);
}

long AdsDevice::DeleteSymbolHandle(uint32_t handle) const
{
    return WriteReqEx(ADSIGRP_SYM_RELEASEHND, 0, sizeof(handle), &handle);
}

DeviceInfo AdsDevice::GetDeviceInfo() const
{
    DeviceInfo info;
    auto error = AdsSyncReadDeviceInfoReqEx(GetLocalPort(),
                                            &m_Addr,
                                            &info.name[0],
                                            &info.version);

    if (error) {
        throw AdsException(error);
    }
    return info;
}


std::map<std::string,std::string> AdsDevice::GetDeviceAdsVariables() const
{
    long 			nErr;
    uint32_t			uiIndex;
    uint32_t			bytesReadInfo = 0;
    uint32_t			bytesReadEntry = 0;
    char* 			pchSymbols = NULL;
    AdsSymbolUploadInfo  adsSymbolUploadInfo;
    AdsSymbolEntry*      	pAdsSymbolEntry;
    std::map<std::string,std::string>     lADSVariables;


    // Read the length of the variable declaration
    nErr = ReadReqEx2(ADSIGRP_SYM_UPLOADINFO, 0x0, sizeof(AdsSymbolEntry), &adsSymbolUploadInfo, &bytesReadInfo);
    if(nErr != 0 && bytesReadInfo != sizeof(adsSymbolUploadInfo)) {
      throw AdsException(nErr);
    }
    pchSymbols = new char[adsSymbolUploadInfo.nSymSize];

    // Read information about the PLC variables 
    nErr = ReadReqEx2(ADSIGRP_SYM_UPLOAD, 0x0, adsSymbolUploadInfo.nSymSize, pchSymbols, &bytesReadEntry);
    if(nErr != 0 ) {
      throw AdsException(nErr);
    }

    //Output information about the PLC variables
    pAdsSymbolEntry = (AdsSymbolEntry*)pchSymbols;


    for (uiIndex = 0; uiIndex < bytesReadEntry;  uiIndex = uiIndex + pAdsSymbolEntry->entryLength)
    {

        pAdsSymbolEntry = (AdsSymbolEntry*) (pchSymbols + uiIndex);

        //Check if pAdsSymbolEntry is well formated
        if(pAdsSymbolEntry->entryLength + uiIndex < bytesReadEntry )
        {
            std::string nameVar;
            nameVar = GetNameVar(pAdsSymbolEntry );
            std::string typeVar;
            typeVar = GetTypeVar(pAdsSymbolEntry );

            lADSVariables[nameVar] = typeVar;
        }

    }

    return lADSVariables;
}


std::string AdsDevice::GetNameVar(AdsSymbolEntry* pAdsSymbolEntry) const
{
    char *  data=(char*)( pAdsSymbolEntry) +sizeof(AdsSymbolEntry);
    std::string nameVar(data);
    return nameVar;
}

std::string AdsDevice::GetTypeVar(AdsSymbolEntry* pAdsSymbolEntry) const
{

    char *  data= (char*)pAdsSymbolEntry + sizeof(AdsSymbolEntry) + pAdsSymbolEntry->nameLength +1;
    std::string typeVar(data);
    return typeVar;
}

std::string AdsDevice::GetComment(AdsSymbolEntry* pAdsSymbolEntry) const
{

    char *  data= (char*)pAdsSymbolEntry + sizeof(AdsSymbolEntry) + (pAdsSymbolEntry->nameLength +1) + (pAdsSymbolEntry->typeLength +1);
    std::string Comment(data);
    return Comment;
}

AdsHandle AdsDevice::GetHandle(const uint32_t offset) const
{
    return {new uint32_t {offset}, {[](uint32_t){ return 0; }}};
}

AdsHandle AdsDevice::GetHandle(const std::string& symbolName) const
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

    handle = bhf::ads::letoh(handle);
    return {new uint32_t {handle}, {std::bind(&AdsDevice::DeleteSymbolHandle, this, std::placeholders::_1)}};
}

AdsHandle AdsDevice::GetHandle(const uint32_t               indexGroup,
                               const uint32_t               indexOffset,
                               const AdsNotificationAttrib& notificationAttributes,
                               PAdsNotificationFuncEx       callback,
                               const uint32_t               hUser) const
{
    uint32_t handle = 0;
    auto error = AdsSyncAddDeviceNotificationReqEx(
        *m_LocalPort, &m_Addr,
        indexGroup, indexOffset,
        &notificationAttributes,
        callback,
        hUser,
        &handle);
    if (error || !handle) {
        throw AdsException(error);
    }
    handle = bhf::ads::letoh(handle);
    return {new uint32_t {handle}, {std::bind(&AdsDevice::DeleteNotificationHandle, this, std::placeholders::_1)}};
}

long AdsDevice::GetLocalPort() const
{
    return *m_LocalPort;
}

AdsDeviceState AdsDevice::GetState() const
{
    uint16_t state[2];
    auto error = AdsSyncReadStateReqEx(GetLocalPort(),
                                       &m_Addr,
                                       &state[0],
                                       &state[1]);

    if (error) {
        throw AdsException(error);
    }

    for (auto i: state) {
        if (i >= ADSSTATE::ADSSTATE_MAXSTATES) {
            throw std::out_of_range("Unknown ADSSTATE(" + std::to_string(i) + ')');
        }
    }
    return {static_cast<ADSSTATE>(state[0]), static_cast<ADSSTATE>(state[1])};
}

void AdsDevice::SetState(const ADSSTATE AdsState, const ADSSTATE DeviceState) const
{
    auto error = AdsSyncWriteControlReqEx(GetLocalPort(),
                                          &m_Addr,
                                          AdsState,
                                          DeviceState,
                                          0, nullptr);

    if (error) {
        throw AdsException(error);
    }
}

void AdsDevice::SetTimeout(const uint32_t timeout) const
{
    const auto error = AdsSyncSetTimeoutEx(GetLocalPort(), timeout);
    if (error) {
        throw AdsException(error);
    }
}

uint32_t AdsDevice::GetTimeout() const
{
    uint32_t timeout = 0;
    const auto error = AdsSyncGetTimeoutEx(GetLocalPort(), &timeout);
    if (error) {
        throw AdsException(error);
    }
    return timeout;
}

AdsHandle AdsDevice::OpenFile(const std::string& filename, const uint32_t flags) const
{
    uint32_t bytesRead = 0;
    uint32_t handle;
    const auto error = ReadWriteReqEx2(SYSTEMSERVICE_FOPEN,
                                       flags,
                                       sizeof(handle),
                                       &handle,
                                       filename.length(),
                                       filename.c_str(),
                                       &bytesRead);

    if (error) {
        throw AdsException(error);
    }
    handle = bhf::ads::letoh(handle);
    return {new uint32_t {handle}, {std::bind(&AdsDevice::CloseFile, this, std::placeholders::_1)}};
}

long AdsDevice::ReadReqEx2(uint32_t group, uint32_t offset, uint32_t length, void* buffer, uint32_t* bytesRead) const
{
    return AdsSyncReadReqEx2(*m_LocalPort, &m_Addr, group, offset, length, buffer, bytesRead);
}

long AdsDevice::ReadWriteReqEx2(uint32_t    indexGroup,
                                uint32_t    indexOffset,
                                uint32_t    readLength,
                                void*       readData,
                                uint32_t    writeLength,
                                const void* writeData,
                                uint32_t*   bytesRead) const
{
    return AdsSyncReadWriteReqEx2(GetLocalPort(),
                                  &m_Addr,
                                  indexGroup, indexOffset,
                                  readLength, readData,
                                  writeLength, writeData,
                                  bytesRead
                                  );
}

long AdsDevice::WriteReqEx(uint32_t group, uint32_t offset, uint32_t length, const void* buffer) const
{
    return AdsSyncWriteReqEx(GetLocalPort(), &m_Addr, group, offset, length, buffer);
}
