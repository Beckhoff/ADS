// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2021 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "AdsLib.h"
#include "Log.h"
#include "wrap_endian.h"
#include <cstring>

namespace bhf
{
namespace ads
{
static bool PrependUdpLenTagId(Frame& frame, const uint16_t length, const uint16_t tagId)
{
    frame.prepend(htole(length));
    frame.prepend(htole(tagId));
    return true;
}

static bool PrependUdpTag(Frame& frame, const std::string& value, const uint16_t tagId)
{
    const uint16_t length = value.length() + 1;
    frame.prepend(value.data(), length);
    return PrependUdpLenTagId(frame, length, tagId);
}

static bool PrependUdpTag(Frame& frame, const AmsNetId& value, const uint16_t tagId)
{
    const uint16_t length = sizeof(value);
    frame.prepend(&value, length);
    return PrependUdpLenTagId(frame, length, tagId);
}

enum UdpTag : uint16_t {
    PASSWORD = 2,
    COMPUTERNAME = 5,
    NETID = 7,
    ROUTENAME = 12,
    USERNAME = 13,
};

enum UdpServiceId : uint32_t {
    SERVERINFO = 1,
    ADDROUTE = 6,
    RESPONSE = 0x80000000,
};

static long SendRecv(const std::string& remote, Frame& f, const uint32_t serviceId)
{
    f.prepend(htole(serviceId));

    static const uint32_t invokeId = 0;
    f.prepend(htole(invokeId));

    static const uint32_t UDP_COOKIE = 0x71146603;
    f.prepend(htole(UDP_COOKIE));

    const auto addresses = GetListOfAddresses(remote, "48899");
    UdpSocket s{addresses.get()};
    s.write(f);
    f.reset();

    static constexpr auto headerLength = sizeof(serviceId) + sizeof(invokeId) + sizeof(UDP_COOKIE);
    timeval timeout { 5, 0 };
    s.read(f, &timeout);
    if (headerLength > f.capacity()) {
        LOG_ERROR(__FUNCTION__ << "(): frame too short to be AMS response '0x" << std::hex << f.capacity() << "'\n");
        return ADSERR_DEVICE_INVALIDSIZE;
    }

    const auto cookie = f.pop_letoh<uint32_t>();
    if (UDP_COOKIE != cookie) {
        LOG_ERROR(__FUNCTION__ << "(): response contains invalid cookie '" << cookie << "'\n");
        return ADSERR_DEVICE_INVALIDDATA;
    }
    const auto invoke = f.pop_letoh<uint32_t>();
    if (invokeId != invoke) {
        LOG_ERROR(__FUNCTION__ << "(): response contains invalid invokeId '" << invoke << "'\n");
        return ADSERR_DEVICE_INVALIDDATA;
    }
    const auto service = f.pop_letoh<uint32_t>();
    if ((UdpServiceId::RESPONSE | serviceId) != service) {
        LOG_ERROR(__FUNCTION__ << "(): response contains invalid serviceId '" << std::hex << service << "'\n");
        return ADSERR_DEVICE_INVALIDDATA;
    }
    return 0;
}

long AddRemoteRoute(const std::string& remote,
                    AmsNetId           destNetId,
                    const std::string& destAddr,
                    const std::string& routeName,
                    const std::string& remoteUsername,
                    const std::string& remotePassword)
{
    Frame f { 256 };
    uint32_t tagCount = 0;
    tagCount += PrependUdpTag(f, destAddr, UdpTag::COMPUTERNAME);
    tagCount += PrependUdpTag(f, remotePassword, UdpTag::PASSWORD);
    tagCount += PrependUdpTag(f, remoteUsername, UdpTag::USERNAME);
    tagCount += PrependUdpTag(f, destNetId, UdpTag::NETID);
    tagCount += PrependUdpTag(f, routeName.empty() ? destAddr : routeName, UdpTag::ROUTENAME);
    f.prepend(htole(tagCount));

    const auto myAddr = AmsAddr { destNetId, 0 };
    f.prepend(&myAddr, sizeof(myAddr));

    const auto status = SendRecv(remote, f, UdpServiceId::ADDROUTE);
    if (status) {
        return status;
    }

    // We expect at least the AmsAddr and count fields
    if (sizeof(AmsAddr) + sizeof(uint32_t) > f.capacity()) {
        LOG_ERROR(__FUNCTION__ << "(): frame too short to be AMS response '0x" << std::hex << f.capacity() << "'\n");
        return ADSERR_DEVICE_INVALIDSIZE;
    }

    // ignore AmsAddr in response
    f.remove(sizeof(AmsAddr));

    // process UDP discovery tags
    auto count = f.pop_letoh<uint32_t>();
    while (count--) {
        uint16_t tag;
        uint16_t len;
        if (sizeof(tag) + sizeof(len) > f.capacity()) {
            LOG_ERROR(__FUNCTION__ << "(): frame too short to be AMS response '0x" << std::hex << f.capacity() <<
                      "'\n");
            return ADSERR_DEVICE_INVALIDSIZE;
        }

        tag = f.pop_letoh<uint16_t>();
        len = f.pop_letoh<uint16_t>();
        if (1 != tag) {
            LOG_WARN(__FUNCTION__ << "(): response contains tagId '0x" << std::hex << tag << "' -> ignoring\n");
            f.remove(len);
            continue;
        }
        if (sizeof(uint32_t) != len) {
            LOG_ERROR(__FUNCTION__ << "(): response contains invalid tag length '" << std::hex << len << "'\n");
            return ADSERR_DEVICE_INVALIDSIZE;
        }
        return f.pop_letoh<uint32_t>();
    }
    return ADSERR_DEVICE_INVALIDDATA;
}

long GetRemoteAddress(const std::string& remote, AmsNetId& netId)
{
    Frame f { 128 };

    const uint32_t tagCount = 0;
    f.prepend(htole(tagCount));

    const auto myAddr = AmsAddr { {}, 0 };
    f.prepend(&myAddr, sizeof(myAddr));

    const auto status = SendRecv(remote, f, UdpServiceId::SERVERINFO);
    if (status) {
        return status;
    }

    // We expect at least the AmsAddr
    if (sizeof(netId) > f.capacity()) {
        LOG_ERROR(__FUNCTION__ << "(): frame too short to be AMS response '0x" << std::hex << f.capacity() << "'\n");
        return ADSERR_DEVICE_INVALIDSIZE;
    }
    memcpy(&netId, f.data(), sizeof(netId));
    return 0;
}
}
}
