// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2021 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "RouterAccess.h"
#include "Log.h"
#include "wrap_endian.h"
#include <array>
#include <iostream>

namespace bhf
{
namespace ads
{
struct SearchPciBusReq {
    SearchPciBusReq(const uint64_t quad)
        : leVendorID(bhf::ads::htole<uint16_t>(quad >> 48))
        , leDeviceID(bhf::ads::htole<uint16_t>(quad >> 32))
        , leSubVendorID(bhf::ads::htole<uint16_t>(quad >> 16))
        , leSubSystemID(bhf::ads::htole<uint16_t>(quad))
    {}
private:
    const uint16_t leVendorID;
    const uint16_t leDeviceID;
    const uint16_t leSubVendorID;
    const uint16_t leSubSystemID;
};

struct SearchPciSlotResNew {
    static constexpr size_t MAXBASEADDRESSES = 6;
    const std::array<uint32_t, MAXBASEADDRESSES> leBaseAddresses;
    const uint32_t leSize[MAXBASEADDRESSES];
    const uint32_t leBusNumber;
    const uint32_t leSlotNumber;
    const uint16_t leBoardIrq;
    const uint16_t lePciRegViaPorts;
};

struct SearchPciBusResNew {
    static constexpr size_t MAXSLOTRESPONSE = 64;
    uint32_t leFound;
    std::array<SearchPciSlotResNew, MAXSLOTRESPONSE> slot;
    uint32_t nFound() const
    {
        return bhf::ads::letoh(leFound);
    }
};

std::ostream& operator<<(std::ostream& os, const SearchPciSlotResNew& slot)
{
    return os << std::dec << bhf::ads::letoh(slot.leBusNumber) << ':' <<
           bhf::ads::letoh(slot.leSlotNumber) << " @ 0x" <<
           bhf::ads::letoh(slot.leBaseAddresses[0]);
}

RouterAccess::RouterAccess(const std::string& gw, const AmsNetId netid, const uint16_t port)
    : device(gw, netid, port ? port : 1)
{}

bool RouterAccess::PciScan(const uint64_t pci_id, std::ostream& os) const
{
#define ROUTERADSGRP_ACCESS_HARDWARE 0x00000005
#define ROUTERADSOFFS_A_HW_SEARCHPCIBUS 0x00000003

    SearchPciBusResNew res {};
    uint32_t bytesRead;

    const auto req = SearchPciBusReq {pci_id};
    const auto status = device.ReadWriteReqEx2(
        ROUTERADSGRP_ACCESS_HARDWARE,
        ROUTERADSOFFS_A_HW_SEARCHPCIBUS,
        sizeof(res), &res,
        sizeof(req), &req,
        &bytesRead
        );

    if (ADSERR_NOERR != status) {
        LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex << status << '\n');
        return false;
    }

    if (res.slot.size() < res.nFound()) {
        LOG_WARN(__FUNCTION__
                 << "(): data seems corrupt. Slot count 0x" << std::hex << res.nFound() << " exceedes maximum 0x" << res.slot.size() <<
                 " -> truncating\n");
    }

    auto limit = std::min<size_t>(res.slot.size(), res.nFound());
    os << "PCI devices found: " << std::dec << limit << '\n';
    for (const auto& slot: res.slot) {
        if (!limit--) {
            break;
        }
        os << slot << '\n';
    }
    return !os.good();
}
}
}
