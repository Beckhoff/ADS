// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2022 - 2023 Beckhoff Automation GmbH & Co. KG
   Author: Patrick Bruenn <p.bruenn@beckhoff.com>
 */

#include "SymbolAccess.h"
#include "Log.h"
#include <iostream>
#include <limits>
#include <vector>

namespace bhf
{
namespace ads
{
std::pair<std::string, SymbolEntry> SymbolEntry::Parse(const uint8_t *data,
						       size_t lengthLimit)
{
	const auto pHeader = reinterpret_cast<const AdsSymbolEntry *>(data);
	if (sizeof(*pHeader) > lengthLimit) {
		LOG_ERROR(__FUNCTION__
			  << "(): Read data to short to contain symbol info: "
			  << std::dec << lengthLimit << '\n');
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
		LOG_ERROR(__FUNCTION__
			  << "(): Corrupt entry length: " << std::dec
			  << entry.header.entryLength << '\n');
		throw AdsException(ADSERR_DEVICE_INVALIDDATA);
	}
	lengthLimit = entry.header.entryLength - sizeof(entry.header);
	data += sizeof(entry.header);

	if (entry.header.nameLength > lengthLimit - 1) {
		LOG_ERROR(__FUNCTION__ << "(): Corrupt nameLength: " << std::dec
				       << entry.header.nameLength << '\n');
		throw AdsException(ADSERR_DEVICE_INVALIDDATA);
	}
	entry.name = std::string(reinterpret_cast<const char *>(data),
				 entry.header.nameLength);
	lengthLimit -= entry.header.nameLength + 1;
	data += entry.header.nameLength + 1;

	if (entry.header.typeLength > lengthLimit - 1) {
		LOG_ERROR(__FUNCTION__ << "(): Corrupt typeLength: " << std::dec
				       << entry.header.typeLength << '\n');
		throw AdsException(ADSERR_DEVICE_INVALIDDATA);
	}
	entry.typeName = std::string(reinterpret_cast<const char *>(data),
				     entry.header.typeLength);
	lengthLimit -= entry.header.typeLength + 1;
	data += entry.header.typeLength + 1;

	if (entry.header.commentLength > lengthLimit - 1) {
		LOG_ERROR(__FUNCTION__
			  << "(): Corrupt commentLength: " << std::dec
			  << entry.header.commentLength << '\n');
		throw AdsException(ADSERR_DEVICE_INVALIDDATA);
	}
	entry.comment = std::string(reinterpret_cast<const char *>(data),
				    entry.header.commentLength);
	return { entry.name, entry };
}

void SymbolEntry::WriteAsJSON(std::ostream &os) const
{
#define JSONEntryHex(member) \
	"  \"" << #member << "\": \"0x" << std::hex << header.member << "\",\n"
#define JSONEntryString(member) \
	"  \"" << #member << "\": \"" << member << "\",\n"
#define JSONNumber(member) \
	"  \"" << #member << "\": " << std::dec << header.member
#define JSONEntryNumber(member) JSONNumber(member) << ",\n"

	os << "{\n"
	   << JSONEntryString(name) << JSONEntryString(typeName)
	   << JSONEntryString(comment) << JSONEntryNumber(entryLength)
	   << JSONEntryHex(iGroup) << JSONEntryHex(iOffs)
	   << JSONEntryNumber(size) << JSONEntryHex(dataType)
	   << JSONEntryNumber(nameLength) << JSONEntryNumber(typeLength)
	   << JSONNumber(commentLength) << '\n'
	   << "}\n";
}

SymbolAccess::SymbolAccess(const std::string &gw, const AmsNetId netid,
			   const uint16_t port)
	: device(gw, netid, port ? port : uint16_t(AMSPORT_R0_PLC_TC3))
{
}

SymbolEntryMap SymbolAccess::FetchSymbolEntries() const
{
	uint32_t bytesRead = 0;
	struct AdsSymbolUploadInfo {
		uint32_t nSymbols;
		uint32_t nSymSize;
	} uploadInfo;
	auto status = device.ReadReqEx2(ADSIGRP_SYM_UPLOADINFO, 0,
					sizeof(uploadInfo), &uploadInfo,
					&bytesRead);
	if (ADSERR_NOERR != status) {
		LOG_ERROR(__FUNCTION__
			  << "(): Reading symbol info failed with: 0x"
			  << std::hex << status << '\n');
		throw AdsException(status);
	}

	uploadInfo.nSymSize = bhf::ads::letoh(uploadInfo.nSymSize);
	std::vector<uint8_t> symbols(uploadInfo.nSymSize);
	status = device.ReadReqEx2(ADSIGRP_SYM_UPLOAD, 0, uploadInfo.nSymSize,
				   symbols.data(), &bytesRead);
	if (ADSERR_NOERR != status) {
		LOG_ERROR(__FUNCTION__ << "(): Reading symbols failed with: 0x"
				       << std::hex << status << '\n');
		throw AdsException(status);
	}

	const uint8_t *data = symbols.data();
	auto nSymbols = bhf::ads::letoh(uploadInfo.nSymbols);
	auto entries = std::map<std::string, SymbolEntry>{};
	while (nSymbols--) {
		const auto next = entries.insert(bhf::ads::SymbolEntry::Parse(
							 data, bytesRead))
					  .first->second;
		bytesRead -= next.header.entryLength;
		data += next.header.entryLength;
		if (!bytesRead) {
			return entries;
		}
	}
	LOG_ERROR(__FUNCTION__ << "(): nSymbols: " << uploadInfo.nSymbols
			       << " nSymSize:" << uploadInfo.nSymSize << "'\n");
	throw AdsException(ADSERR_DEVICE_INVALIDDATA);
}

int SymbolAccess::Read(const std::string &name, std::ostream &os) const
{
	const auto entries = FetchSymbolEntries();
	const auto it = entries.find(name);
	if (it == entries.end()) {
		LOG_WARN(__FUNCTION__ << "(): symbol '" << name
				      << "' not found\n");
		return ADSERR_DEVICE_SYMBOLNOTFOUND;
	}

	const auto entry = it->second;
	std::vector<uint8_t> readBuffer(entry.header.size);
	uint32_t bytesRead = 0;
	const auto status = device.ReadReqEx2(entry.header.iGroup,
					      entry.header.iOffs,
					      readBuffer.size(),
					      readBuffer.data(), &bytesRead);
	if (ADSERR_NOERR != status) {
		LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex
				       << status << '\n');
		return status;
	}

	switch (entry.header.dataType) {
	case 0x2: //INT
		os << std::dec
		   << letoh(*reinterpret_cast<int16_t *>(readBuffer.data()))
		   << '\n';
		break;

	case 0x3: //DINT
		os << std::dec
		   << letoh(*reinterpret_cast<int32_t *>(readBuffer.data()))
		   << '\n';
		break;

	case 0x4: //REAL
		os << std::dec
		   << letoh(*reinterpret_cast<float *>(readBuffer.data()))
		   << '\n';
		break;

	case 0x5: //LREAL
		os << std::dec
		   << letoh(*reinterpret_cast<double *>(readBuffer.data()))
		   << '\n';
		break;

	case 0x11: // BYTE
	case 0x21: // BOOL
		os << std::dec << (int)readBuffer.data()[0] << '\n';
		break;

	case 0x12: // WORD, UINT
		os << std::dec
		   << letoh(*reinterpret_cast<uint16_t *>(readBuffer.data()))
		   << '\n';
		break;

	case 0x13: // DWORD, UDINT
		os << std::dec
		   << letoh(*reinterpret_cast<uint32_t *>(readBuffer.data()))
		   << '\n';
		break;

	case 0x15: // LWORD, ULINT
		os << std::dec
		   << letoh(*reinterpret_cast<uint64_t *>(readBuffer.data()))
		   << '\n';
		break;

	default:
		LOG_WARN(__FUNCTION__ << "() Unknown type '" << entry.typeName
				      << "' output in binary\n");
		os.write((const char *)readBuffer.data(), bytesRead);
		break;
	}

	return !std::cout.good();
}

template <typename T>
int SymbolAccess::Write(const SymbolEntry &entry, const std::string &v) const
{
	if (!v.size()) {
		LOG_ERROR(__FUNCTION__
			  << "<T>() empty strings are not supported\n");
		return ADSERR_DEVICE_INVALIDDATA;
	}

	const auto asHex = (v.npos != v.rfind("0x", 0));
	std::stringstream converter;
	converter << (asHex ? std::hex : std::dec) << v;

	T value = {};
	converter >> value;
	if (converter.fail()) {
		LOG_ERROR(__FUNCTION__ << "() parsing '" << v << "' failed\n");
		return ADSERR_DEVICE_INVALIDDATA;
	}

	value = bhf::ads::htole(value);
	return device.WriteReqEx(entry.header.iGroup, entry.header.iOffs,
				 sizeof(value), &value);
}

template <>
int SymbolAccess::Write<uint8_t>(const SymbolEntry &entry,
				 const std::string &v) const
{
	if (!v.size()) {
		LOG_ERROR(__FUNCTION__
			  << "<uint8_t>() empty strings are not supported\n");
		return ADSERR_DEVICE_INVALIDDATA;
	}

	const auto asHex = (v.npos != v.rfind("0x", 0));
	std::stringstream converter;
	converter << (asHex ? std::hex : std::dec) << v;

	uint16_t integer_value = {};
	converter >> integer_value;
	if (converter.fail()) {
		LOG_ERROR(__FUNCTION__ << "() parsing '" << v << "' failed\n");
		return ADSERR_DEVICE_INVALIDDATA;
	}
	if (integer_value > std::numeric_limits<uint8_t>::max()) {
		LOG_ERROR(__FUNCTION__
			  << "() '" << v
			  << "' does not fit into a single byte\n");
		return ADSERR_DEVICE_INVALIDDATA;
	}
	auto value = static_cast<uint8_t>(integer_value);
	return device.WriteReqEx(entry.header.iGroup, entry.header.iOffs,
				 sizeof(value), &value);
}

template <>
int SymbolAccess::Write<std::string>(const SymbolEntry &entry,
				     const std::string &value) const
{
	// Copy value to a temporary buffer so we can fill it with null bytes. PLC
	// strings are just an array of random bytes so we have to overwrite the
	// entire array to get "normal" string behaviour. E.g. overwriting a long
	// string with a shorter string, should overwrite the old string entirely!
	auto buffer = value;
	buffer.resize(entry.header.size);
	return device.WriteReqEx(entry.header.iGroup, entry.header.iOffs,
				 buffer.size(), buffer.data());
}

int SymbolAccess::Write(const std::string &name, const std::string &value) const
{
	const auto entries = FetchSymbolEntries();
	const auto it = entries.find(name);
	if (it == entries.end()) {
		LOG_WARN(__FUNCTION__ << "(): symbol '" << name
				      << "' not found\n");
		return ADSERR_DEVICE_SYMBOLNOTFOUND;
	}

	const auto entry = it->second;

	switch (entry.header.dataType) {
	case 0x2: //INT
		return Write<int16_t>(entry, value);

	case 0x3: //DINT
		return Write<int32_t>(entry, value);

	case 0x4: //REAL
		return Write<float>(entry, value);

	case 0x5: //LREAL
		return Write<double>(entry, value);

	case 0x11: // BYTE
	case 0x21: // BOOL
		return Write<uint8_t>(entry, value);

	case 0x12: // WORD, UINT
		return Write<uint16_t>(entry, value);

	case 0x13: // DWORD, UDINT
		return Write<uint32_t>(entry, value);

	case 0x15: // LWORD, ULINT
		return Write<uint64_t>(entry, value);

	default:
		LOG_WARN(__FUNCTION__ << "() Unknown type '" << entry.typeName
				      << "' writting as string\n");
		return Write<std::string>(entry, value);
	}
}

int SymbolAccess::ShowSymbols(std::ostream &os) const
{
	for (const auto &entry : FetchSymbolEntries()) {
		entry.second.WriteAsJSON(os);
	}
	return 0;
}
}
}
