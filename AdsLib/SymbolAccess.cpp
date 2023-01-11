// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2022 - 2023 Beckhoff Automation GmbH & Co. KG
   Author: Patrick Bruenn <p.bruenn@beckhoff.com>
 */

#include "SymbolAccess.h"
#include "Log.h"
#include <iostream>
#include <vector>

namespace bhf
{
namespace ads
{
std::pair<std::string, SymbolEntry> SymbolEntry::Parse(const uint8_t* data, size_t lengthLimit)
{
    const auto pHeader = reinterpret_cast<const AdsSymbolEntry*>(data);
    if (sizeof(*pHeader) > lengthLimit) {
        LOG_ERROR(__FUNCTION__ << "(): Read data to short to contain symbol info: " << std::dec << lengthLimit << '\n');
        throw AdsException(ADSERR_DEVICE_INVALIDDATA);
    }
    SymbolEntry entry;
    entry.header.entryLength = letoh(pHeader->entryLength);
    entry.header.iGroup = letoh(pHeader->iGroup);
    entry.header.iOffs = letoh(pHeader->iOffs);
    entry.header.size = letoh(pHeader->size);
    entry.header.dataType = letoh(pHeader->dataType);
    entry.header.flags = letoh(pHeader->flags);
    entry.header.nameLength = letoh(pHeader->nameLength);
    entry.header.typeLength = letoh(pHeader->typeLength);
    entry.header.commentLength = letoh(pHeader->commentLength);

    if (entry.header.entryLength > lengthLimit) {
        LOG_ERROR(
            __FUNCTION__ << "(): Corrupt entry length: " << std::dec << entry.header.entryLength << '\n');
        throw AdsException(ADSERR_DEVICE_INVALIDDATA);
    }
    lengthLimit = entry.header.entryLength - sizeof(entry.header);
    data += sizeof(entry.header);

    if (entry.header.nameLength > lengthLimit - 1) {
        LOG_ERROR(
            __FUNCTION__ << "(): Corrupt nameLength: " << std::dec << entry.header.nameLength << '\n');
        throw AdsException(ADSERR_DEVICE_INVALIDDATA);
    }
    entry.name = std::string(reinterpret_cast<const char*>(data), entry.header.nameLength);
    lengthLimit -= entry.header.nameLength + 1;
    data += entry.header.nameLength + 1;

    if (entry.header.typeLength > lengthLimit - 1) {
        LOG_ERROR(
            __FUNCTION__ << "(): Corrupt typeLength: " << std::dec << entry.header.typeLength << '\n');
        throw AdsException(ADSERR_DEVICE_INVALIDDATA);
    }
    entry.typeName = std::string(reinterpret_cast<const char*>(data), entry.header.typeLength);
    lengthLimit -= entry.header.typeLength + 1;
    data += entry.header.typeLength + 1;

    if (entry.header.commentLength > lengthLimit - 1) {
        LOG_ERROR(
            __FUNCTION__ << "(): Corrupt commentLength: " << std::dec << entry.header.commentLength << '\n');
        throw AdsException(ADSERR_DEVICE_INVALIDDATA);
    }
    entry.comment = std::string(reinterpret_cast<const char*>(data), entry.header.commentLength);
    return {entry.name, entry};
}

SymbolAccess::SymbolAccess(const std::string& gw, const AmsNetId netid, const uint16_t port)
    : device(gw, netid, port ? port : uint16_t(AMSPORT_R0_PLC_TC3))
{}

SymbolEntryMap SymbolAccess::FetchSymbolEntries() const
{
    uint32_t bytesRead = 0;
    struct AdsSymbolUploadInfo {
        uint32_t nSymbols;
        uint32_t nSymSize;
    } uploadInfo;
    auto status = device.ReadReqEx2(ADSIGRP_SYM_UPLOADINFO,
                                    0,
                                    sizeof(uploadInfo),
                                    &uploadInfo,
                                    &bytesRead);
    if (ADSERR_NOERR != status) {
        LOG_ERROR(__FUNCTION__ << "(): Reading symbol info failed with: 0x" << std::hex << status << '\n');
        throw AdsException(status);
    }

    uploadInfo.nSymSize = bhf::ads::letoh(uploadInfo.nSymSize);
    std::vector<uint8_t> symbols(uploadInfo.nSymSize);
    status = device.ReadReqEx2(ADSIGRP_SYM_UPLOAD,
                               0,
                               uploadInfo.nSymSize,
                               symbols.data(),
                               &bytesRead);
    if (ADSERR_NOERR != status) {
        LOG_ERROR(__FUNCTION__ << "(): Reading symbols failed with: 0x" << std::hex << status << '\n');
        throw AdsException(status);
    }

    const uint8_t* data = symbols.data();
    auto nSymbols = bhf::ads::letoh(uploadInfo.nSymbols);
    auto entries = std::map<std::string, SymbolEntry> {};
    while (nSymbols--) {
        const auto next = entries.insert(bhf::ads::SymbolEntry::Parse(data, bytesRead)).first->second;
        bytesRead -= next.header.entryLength;
        data += next.header.entryLength;
        if (!bytesRead) {
            return entries;
        }
    }
    LOG_ERROR(__FUNCTION__ << "(): nSymbols: " << uploadInfo.nSymbols << " nSymSize:" << uploadInfo.nSymSize << "'\n");
    throw AdsException(ADSERR_DEVICE_INVALIDDATA);
}
}
}
