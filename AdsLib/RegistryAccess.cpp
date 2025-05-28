// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "RegistryAccess.h"
#include "Log.h"
#include "wrap_registry.h"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <map>

#define PARSING_EXCEPTION(msg)                                              \
	do {                                                                \
		std::stringstream ss;                                       \
		if (std::numeric_limits<size_t>::max() == lineNumber) {     \
			ss << "Invalid network data: " << msg << '\n';      \
		} else {                                                    \
			ss << "Invalid file format: " << msg << " in line " \
			   << lineNumber << '\n';                           \
		}                                                           \
		throw std::runtime_error(ss.str());                         \
	} while (0)

namespace bhf
{
namespace ads
{
// First line of valid registry file
constexpr const char *WINDOWS_REGISTRY_HEADER =
	"Windows Registry Editor Version 5.00";

static const std::map<nRegHive, std::string> g_HiveMapping = {
	{ nRegHive::REG_HKEYCURRENTUSER, "HKEY_CURRENT_USER\\" },
	{ nRegHive::REG_HKEYCLASSESROOT, "HKEY_CLASSES_ROOT\\" },
	{ nRegHive::REG_HKEYLOCALMACHINE, "HKEY_LOCAL_MACHINE\\" },
	{ nRegHive::REG_DELETE_HKEYCURRENTUSER, "-HKEY_CURRENT_USER\\" },
	{ nRegHive::REG_DELETE_HKEYCLASSESROOT, "-HKEY_CLASSES_ROOT\\" },
	{ nRegHive::REG_DELETE_HKEYLOCALMACHINE, "-HKEY_LOCAL_MACHINE\\" },
};

static nRegHive GetRegHive(const std::string &key, size_t &keyOffset)
{
	for (const auto &h : g_HiveMapping) {
		if (!key.rfind(h.second, 0)) {
			keyOffset = key.find_first_of('\\') + 1;
			return h.first;
		}
	}
	throw std::runtime_error("could not find supported hive in " + key);
}

static bool HasUTF16BOM(const std::string &line)
{
	constexpr auto ff = static_cast<char>(0xFF);
	constexpr auto fe = static_cast<char>(0xFE);
	if (line.length() < 2) {
		return false;
	}

	if ((line[0] == ff) && (line[1] == fe)) {
		return true;
	}

	return line[0] == fe && line[1] == ff;
}

static uint8_t hex2val(char c)
{
	// we already verify that isxdigit(c)
	return (c < 'A') ? c - '0' : toupper(c) - 'A' + 10;
}

static void VerifyDataLen(const uint32_t type, const size_t dataLen,
			  const size_t lineNumber)
{
	switch (type) {
	case REG_NONE:
		if (!dataLen) {
			PARSING_EXCEPTION(
				"data is non-empty but type is REG_N0NE");
		}
		break;

	case REG_SZ:
	case REG_EXPAND_SZ:
	case REG_BINARY:
	case REG_MULTI_SZ:
		break;

	case REG_DWORD:
		if (sizeof(uint32_t) != dataLen) {
			PARSING_EXCEPTION(
				"not enough data bytes for REG_DWORD");
		}
		break;

	case REG_QWORD:
		if (sizeof(uint64_t) != dataLen) {
			PARSING_EXCEPTION(
				"not enough data bytes for REG_QWORD");
		}
		break;

	default:
		PARSING_EXCEPTION("unsupported type " + std::to_string(type));
	}
}

static void ParseStringData(RegistryEntry &value, const char *&it,
			    std::istream & /*input*/, size_t &lineNumber)
{
	// REG_SZ data should end with a closing quote and null terminator
	while ('\0' != it[0] && '\0' != it[1]) {
		value.PushData(*it);
		++it;
	}
	if ('\"' != *it) {
		PARSING_EXCEPTION("missing closing quotation mark");
	}
	value.PushData('\0');
}

static void ParseDwordData(bhf::ads::RegistryEntry &value, const char *&it,
			   std::istream & /*input*/, size_t &lineNumber)
{
	if ('\0' == *it) {
		PARSING_EXCEPTION(
			"invalid dword hex character length (no data)");
	}

	uint32_t v = 0;
	std::stringstream converter;
	for (auto i = 2 * sizeof(v); i; --i, ++it) {
		if ('\0' == *it) {
			break;
		}
		converter << std::hex << *it;
	}
	if ('\0' != *it) {
		PARSING_EXCEPTION(
			"invalid dword hex character length (too long)");
	}

	converter >> v;
	value.PushData(v);
}

static void ParseHexData(bhf::ads::RegistryEntry &value, const char *&it,
			 std::istream &input, size_t &lineNumber)
{
	std::string line{};
	for (; '\0' != *it;) {
		// Skip commas but ensure there is exactly one
		if (',' == *it) {
			++it;
			if (',' == *it) {
				PARSING_EXCEPTION("two subsequent commas");
			}
			continue;
		}

		// Data is allowed to containe linebreaks. However, we treat comments or empty lines
		// in such a multiline data section as error.
		if ('\\' == *it) {
			++it;
			if ('\0' != *it) {
				PARSING_EXCEPTION(
					"data continuation line should be the las character in line");
			}

			if (!std::getline(input, line)) {
				PARSING_EXCEPTION(
					"data continuation line next line is missing");
			}
			it = line.c_str();
			if (strncmp("  ", it, 2)) {
				PARSING_EXCEPTION(
					"data continuation line should start with two spaces");
			}
			it += 2;
		}

		// We only accept hex data values consisting of two characters (trailing 0)
		for (auto i = 0; i < 2; ++i) {
			if (!isxdigit(it[i])) {
				PARSING_EXCEPTION("invalid hex character '"
						  << it[i] << "'");
			}
		}
		const uint8_t byte = (hex2val(it[0]) << 4) |
				     (hex2val(it[1]) & 0xF);
		value.PushData(byte);
		it += 2;

		// Hex data values should be separated by a comma
		if (*it && (',' != *it)) {
			PARSING_EXCEPTION("two subsequent values");
		}
	}
	VerifyDataLen(value.type, value.dataLen, lineNumber);
}

struct DataMapping {
	std::string prefix;
	std::function<void(bhf::ads::RegistryEntry &, const char *&,
			   std::istream &, size_t &)>
		parse;
};

static const std::map<uint32_t, DataMapping> g_RegTypeMapping = {
	{ REG_BINARY, { "=hex:", ParseHexData } },
	{ REG_DWORD, { "=dword:", ParseDwordData } },
	{ REG_EXPAND_SZ, { "=hex(2):", ParseHexData } },
	{ REG_MULTI_SZ, { "=hex(7):", ParseHexData } },
	{ REG_NONE, { "=hex(0):", ParseHexData } },
	{ REG_QWORD, { "=hex(b):", ParseHexData } },
	{ REG_SZ, { "=\"", ParseStringData } },
};

static void ParseHexArchetype(const char *&it, bhf::ads::RegistryEntry &value,
			      std::istream &input, size_t &lineNumber)
{
	for (const auto &t : g_RegTypeMapping) {
		const auto &prefix = t.second.prefix;
		const auto length = prefix.size();
		if (std::string::npos != prefix.find(it, 0, length)) {
			const auto leType = htole(t.first);
			value.Append(&leType, sizeof(leType));
			value.type = t.first;

			it += length;
			t.second.parse(value, it, input, lineNumber);
			return;
		}
	}
	PARSING_EXCEPTION("invalid data class definition '" << *it << '\n');
}

std::vector<RegistryEntry> RegFileParse(std::istream &input)
{
	std::vector<RegistryEntry> entries{};
	size_t lineNumber = 0;
	// grab the version header as its the only token winreg nags about if not present
	std::string version;
	if (!std::getline(input, version)) {
		PARSING_EXCEPTION("missing version header");
	}
	lineNumber++;

	if (HasUTF16BOM(version)) {
		// try to detect UTF-16 BOM as windows will export it by default
		PARSING_EXCEPTION(
			"UTF-16 BOM detected! Please ensure UTF-8 content or remove the BOM. You can use iconv to convert UTF-16 files beforehand");
	}

	// try to detect CRLF line endings as std::getline will not automatically strip the trailing \r
	if (version[version.length() - 1] == '\r') {
		PARSING_EXCEPTION(
			"carriage return detected! Please ensure \\n (LF, unix) line endings instead of \\r\\n (CRLF, dos). You can use dos2unix to convert the file beforehand");
	}

	// there are older versions as well but this hasn't changed in ages
	if (version.compare(WINDOWS_REGISTRY_HEADER)) {
		PARSING_EXCEPTION("found version header: '"
				  << version << "' expected: '"
				  << WINDOWS_REGISTRY_HEADER << '\'');
	}

	std::string line;
	// TODO: Change entries vector to something like std::vector<std::unique_ptr<RegistryEntry>>.
	//       Then we can replace these two booleans with a pointer to the last key.
	auto keyIsMissing = true;
	auto keyIsForDeletion = false;
	for (lineNumber++; std::getline(input, line); lineNumber++) {
		// Skip empty lines and comments
		if (line.empty() || (line.front() == ';')) {
			continue;
		}

		// keys are enclosed in brackets
		if ((line.front() == '[') && (line.back() == ']')) {
			keyIsMissing = false;
			entries.push_back(RegistryEntry::Create(
				line.substr(1, line.length() - 2)));
			keyIsForDeletion = entries.back().IsForDeletion();
			continue;
		}

		// cannot add value if key is not present
		if (keyIsMissing) {
			PARSING_EXCEPTION("missing associated key");
		}

		// find value
		RegistryEntry value{};
		auto it = line.c_str();
		if ('@' == *it) {
			// (Default) - value stays empty
		} else if ('"' != *it) {
			PARSING_EXCEPTION(
				"missing value defintion (using quotation marks or @ for (Default))");
		} else {
			value.ParseStringValue(it, lineNumber);
		}
		// complete value name with NULL terminator
		value.buffer.push_back('\0');
		++it;

		ParseHexArchetype(it, value, input, lineNumber);
		if (keyIsForDeletion) {
			PARSING_EXCEPTION("associated key will be deleted.");
		}
		entries.push_back(std::move(value));
	}
	return entries;
}

size_t RegistryEntry::Append(const void *data, const size_t length)
{
	auto next = reinterpret_cast<const uint8_t *>(data);
	const auto end = next + length;
	while (next < end) {
		buffer.push_back(*next);
		++next;
	}
	return length;
}

bool RegistryEntry::IsForDeletion() const
{
	switch (hive) {
	case REG_DELETE_HKEYLOCALMACHINE:
	case REG_DELETE_HKEYCURRENTUSER:
	case REG_DELETE_HKEYCLASSESROOT:
		return true;

	default:
		return false;
	}
}

RegistryEntry RegistryEntry::Create(const std::string &line)
{
	size_t keyOffset{};
	RegistryEntry item{};
	item.hive = GetRegHive(line, keyOffset);
	item.keyLen =
		item.Append(line.c_str() + keyOffset, line.size() - keyOffset);

	char i = '\0';
	item.keyLen += item.Append(&i, 1);
	return item;
}

RegistryEntry RegistryEntry::Create(const std::vector<uint8_t> &&buffer,
				    const nRegHive hive, const uint32_t regFlag)
{
	const auto stringLen =
		1 + strnlen(reinterpret_cast<const char *>(buffer.data()),
			    buffer.size());
	if (buffer.size() < stringLen) {
		throw std::runtime_error(
			"missing string terminator for value or key.");
	}

	if (buffer.size() == stringLen) {
		// Registry key has only the name as a NULL terminated string
		if (regFlag != REGFLAG_ENUMKEYS) {
			throw std::runtime_error(
				"Got registry key from network, but we expected somthing else.");
		}
		return RegistryEntry{ buffer, hive, stringLen, 0, 0 };
	}
	if (regFlag != REGFLAG_ENUMVALUE_VTD) {
		throw std::runtime_error(
			"Got registry value from network, but we expected something else.");
	}

	auto bytesLeft = buffer.size() - stringLen;
	uint32_t type = 0;
	if (bytesLeft < sizeof(type)) {
		throw std::runtime_error("not enough bytes for type left");
	}

	for (size_t i = 0; i < sizeof(type); i++) {
		type += buffer[stringLen + i] << i * 8;
		--bytesLeft;
	}
	VerifyDataLen(type, bytesLeft, std::numeric_limits<size_t>::max());
	return RegistryEntry{ buffer, hive, 0, type, bytesLeft };
}

void RegistryEntry::ParseStringValue(const char *&it, size_t &lineNumber)
{
	// Skip opening quote
	for (++it; '\0' != *it; ++it) {
		if ('"' == *it) {
			return;
		}
		// String values are stored unescaped
		if ('\\' == *it) {
			++it;
			if (('\\' != *it) && ('\"' != *it)) {
				PARSING_EXCEPTION(
					"found escape character in front of not escapable char '"
					<< *it << "'");
			}
		}
		buffer.push_back(*it);
	}
	PARSING_EXCEPTION("missing closing quotation mark for value");
}

static size_t WriteEscaped(std::ostream &os, const uint8_t *it,
			   const bool addOpeningQuote = true)
{
	size_t count = 0;
	if (addOpeningQuote) {
		os << '"';
		++count;
	}
	for (; *it != '\0'; ++count, ++it) {
		// Quotes and backslashes need to get escaped with a backslash
		if (strchr("\\\"", *it)) {
			os << '\\';
			++count;
		}
		os << *it;
	}
	os << '"';
	return count + 1;
}

std::ostream &RegistryEntry::Write(std::ostream &os) const
{
	if (keyLen) {
		// RegistryEntry is a registry key, which has no value and is placed in brackets in its own line
		return os << "\n[" << g_HiveMapping.at(hive)
			  << reinterpret_cast<const char *>(buffer.data())
			  << "]\n";
	}

	size_t currentPos = 0;
	if (!buffer[0]) {
		// RegistryEntry is an anonymous registry value
		os << '@';
		currentPos += 1;
	} else {
		// RegistryEntry is a named registry value
		currentPos += WriteEscaped(os, buffer.data());
	}

	// Now, the actual data follows. First we write the data prefix
	const auto &prefix = g_RegTypeMapping.at(type).prefix;
	os << prefix;
	currentPos += prefix.length();

	// The actual data format depends on the type
	if (type == REG_SZ) {
		const auto dataBegin = buffer.data() + buffer.size() - dataLen;
		WriteEscaped(os, dataBegin, false);
	} else if (type == REG_DWORD) {
		uint32_t val = 0;
		for (size_t i = buffer.size() - sizeof(uint32_t);
		     i < buffer.size(); ++i) {
			val <<= 8;
			val += buffer[i];
		}
		os << std::hex << std::setw(8) << std::setfill('0') << val;
	} else {
		if (dataLen > 0) {
			// Windows always exports 25 pairs of hex bytes (delimited by a comma) on each line
			// including two characters at the start so we do the same.
			constexpr auto lineWrap = 25 * 3 + 1;
			auto dataIt = std::prev(buffer.cend(), dataLen);
			os << std::hex << std::setw(2) << std::setfill('0')
			   << +*dataIt++;
			for (; dataIt != buffer.cend(); dataIt++) {
				os << ',';
				currentPos++;
				if (currentPos >= lineWrap) {
					os << "\\\n  ";
					currentPos = 2;
				}
				os << std::hex << std::setw(2)
				   << std::setfill('0') << +*dataIt;
				currentPos += 2;
			}
		}
	}
	return os << '\n';
}

RegistryAccess::RegistryAccess(const std::string &ipV4, AmsNetId netId,
			       uint16_t port)
	: device(ipV4, netId, port ? port : 10000)
{
}

std::vector<RegistryEntry>
RegistryAccess::Enumerate(const RegistryEntry &key, const uint32_t regFlag,
			  const size_t bufferSize) const
{
	std::vector<RegistryEntry> entries;
	for (auto offset = regFlag;
	     (offset & REGFLAG_ENUMVALUE_MASK) == regFlag; ++offset) {
		uint32_t bytesRead = 0;
		std::vector<uint8_t> data(bufferSize);
		const auto ret = device.ReadWriteReqEx2(
			key.hive, offset, bufferSize, data.data(), key.keyLen,
			key.buffer.data(), &bytesRead);
		if (ret == ADSERR_DEVICE_NOTFOUND) {
			// ADS_ERR_DEVICE_NOTFOUND is returned once the enumeration is exhausted.
			return entries;
		} else if (ret) {
			throw AdsException(ret);
		}
		data.resize(bytesRead);
		entries.push_back(RegistryEntry::Create(std::move(data),
							key.hive, regFlag));
	}
	throw std::runtime_error("overflow in offset detected");
}

int RegistryAccess::Export(const std::string &firstKey, std::ostream &os) const
{
	os << bhf::ads::WINDOWS_REGISTRY_HEADER << "\n";

	for (std::list<RegistryEntry> pendingKeys{
		     RegistryEntry::Create(firstKey) };
	     !pendingKeys.empty();) {
		auto key = pendingKeys.front();
		pendingKeys.pop_front();
		key.Write(os);

		// First dump all the values of a key
		for (const auto &value :
		     Enumerate(key, REGFLAG_ENUMVALUE_VTD, 0x800)) {
			value.Write(os);
		}

		// Then find all subkey and put them into a queue to omit recursion
		const auto pendingKeysFront = pendingKeys.cbegin();
		for (auto &newKey : Enumerate(key, REGFLAG_ENUMKEYS, 0x400)) {
			newKey.buffer.insert(newKey.buffer.begin(),
					     key.buffer.cbegin(),
					     key.buffer.cend());
			newKey.buffer[key.buffer.size() - 1] = '\\';
			newKey.keyLen += key.buffer.size();
			pendingKeys.insert(pendingKeysFront, newKey);
		}
	}
	os << '\n';
	return 0;
}

int RegistryAccess::Import(std::istream &is) const
{
	constexpr auto SYSTEMSERVICE_REGISTRY_INVALIDKEY = 1;
	auto entries = RegFileParse(is);
	const auto *key = &entries.front();
	for (auto &next : entries) {
		if (next.keyLen) {
			key = &next;
			if (key->IsForDeletion()) {
				const auto status = device.WriteReqEx(
					key->hive, 0, key->buffer.size(),
					key->buffer.data());

				switch (status) {
				case ADSERR_NOERR:
					break; // OK

				case SYSTEMSERVICE_REGISTRY_INVALIDKEY:
					LOG_WARN(
						__FUNCTION__
						<< "(): failed to delete registry key \""
						<< (g_HiveMapping.at(key->hive)
							    .c_str() +
						    1)
						<< key->buffer.data()
						<< "\". It could not be found in the registry of the target.\n");
					break; // OK: The key could not be found. But that is OK, because we wanted to delete it.

				default:
					LOG_ERROR(
						__FUNCTION__
						<< "(): failed to delete registry key \""
						<< (g_HiveMapping.at(key->hive)
							    .c_str() +
						    1)
						<< key->buffer.data()
						<< "\" with error code: 0x"
						<< std::hex << status << '\n');
					return 1;
				}
			}
			continue;
		}

		// Prepend key path before value name
		next.buffer.insert(next.buffer.begin(), key->buffer.cbegin(),
				   key->buffer.cend());
		const auto status = device.WriteReqEx(
			key->hive, 0, next.buffer.size(), next.buffer.data());
		if (ADSERR_NOERR != status) {
			LOG_ERROR(__FUNCTION__ << "(): failed with: 0x"
					       << std::hex << status << '\n');
			return 1;
		}
	}
	return 0;
}

int RegistryAccess::Verify(std::istream &is, std::ostream &os)
{
	os << WINDOWS_REGISTRY_HEADER << '\n';
	for (const auto &e : RegFileParse(is)) {
		e.Write(os);
	}
	os << '\n';
	return 0;
}
}
}
