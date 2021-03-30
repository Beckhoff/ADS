// SPDX-License-Identifier: MIT
/**
    Copyright (C) 2021 Beckhoff Automation GmbH & Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
 */

#include "AdsDevice.h"
#include "AdsLib.h"
#include "Log.h"
#include "ParameterList.h"
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <vector>

int usage(const char* const errorMessage = nullptr)
{
    if (errorMessage) {
        LOG_ERROR(errorMessage);
    }
    std::cout <<
        R"(
USAGE:
	<target[:port]> [OPTIONS...] <command> [CMD_OPTIONS...] [<command_parameter>...]

	target: AmsNetId, hostname or IP address of your target
	port: AmsPort if omitted the default is command specific

OPTIONS:
	--gw=<hostname> or IP address of your AmsNetId target (mandatory in standalone mode)

COMMANDS:
	addroute [CMD_OPTIONS...]
		Add an ADS route to a remote TwinCAT system. CMD_OPTIONS are:
		--addr=<hostname> or IP address of the routes destination
		--netid=<AmsNetId> of the routes destination
		--password=<password> for the user on the remote TwinCAT system
		--username=<user> on the remote TwinCAT system (optional, defaults to Administrator)
		--routename=<name> of the new route on the remote TwinCAT system (optional, defaults to --addr)
	examples:
		Use Administrator account to add a route with the same name as destinations address
		$ adstool 192.168.0.231 addroute --addr=192.168.0.1 --netid=192.168.0.1.1.1 --password=1

		Use 'guest' account to add a route with a selfdefined name
		$ adstool 192.168.0.231 addroute --addr=192.168.0.1 --netid=192.168.0.1.1.1 --password=1 --username=guest --routename=Testroute

	netid
		Read the AmsNetId from a remote TwinCAT router
		$ adstool 192.168.0.231 netid

	raw [--read=<number_of_bytes>] <IndexGroup> <IndexOffset>
		This command gives low level access to:
		- AdsSyncReadReqEx2()
		- AdsSyncReadWriteReqEx2()
		- AdsSyncWriteReqEx()
		Read/write binary data at every offset with every length. Data
		to write is provided through stdin. Length of the data to write
		is determined through the number of bytes provided. If --read
		is not provided, the underlying method used will be pure write
		request (AdsSyncWriteReqEx()). If no data is provided on stdin,
		--read is mandatory and a pure read request (AdsSyncReadReqEx2())
		is send. If both, data through stdin and --read, are available,
		a readwrite request will be send (AdsSyncReadWriteReqEx2()).

                Read 10 bytes from TC3 PLC index group 0x4040 offset 0x1 into a file:
		$ adstool 5.24.37.144.1.1:851 raw --read=10 "0x4040" "0x1" > read.bin

		Write data from file to TC3 PLC index group 0x4040 offset 0x1:
		$ adstool 5.24.37.144.1.1 raw "0x4040" "0x1" < read.bin

		Write data from write.bin to TC3 PLC index group 0xF003 offset 0x0
		and read result into read.bin:
		$ adstool 5.24.37.144.1.1 raw --read=4 "0xF003" "0x0" < write.bin > read.bin

	state [<value>]
		Read or write the ADS state of the device at AmsPort (default 10000).
		ADS states are documented here:
		https://infosys.beckhoff.com/index.php?content=../content/1031/tcadswcf/html/tcadswcf.tcadsservice.enumerations.adsstate.html
	examples:
		Check if TwinCAT is in RUN:
		$ adstool 5.24.37.144.1.1 state
		5

		Set TwinCAT to CONFIG mode:
		$ adstool 5.24.37.144.1.1 state 16
)";
    exit(1);
}

typedef int (* CommandFunc)(const AmsNetId, const uint16_t, const std::string&, bhf::Commandline&);
using CommandMap = std::map<const std::string, CommandFunc>;

int RunAddRoute(const IpV4 remote, bhf::Commandline& args)
{
    bhf::ParameterList params = {
        {"--addr"},
        {"--netid"},
        {"--password"},
        {"--username", false, "Administrator"},
        {"--routename"},
    };
    args.Parse(params);

    return bhf::ads::AddRemoteRoute(remote,
                                    make_AmsNetId(params.Get<std::string>("--netid")),
                                    params.Get<std::string>("--addr"),
                                    params.Get<std::string>("--routename"),
                                    params.Get<std::string>("--username"),
                                    params.Get<std::string>("--password")
                                    );
}

int RunNetId(const IpV4 remote)
{
    AmsNetId netId;
    bhf::ads::GetRemoteAddress(remote, netId);
    std::cout << netId << '\n';
    return 0;
}

int RunRaw(const AmsNetId netid, const uint16_t port, const std::string& gw, bhf::Commandline& args)
{
    bhf::ParameterList params = {
        {"--read"},
    };
    args.Parse(params);

    const auto group = args.Pop<uint32_t>("IndexGroup is missing");
    const auto offset = args.Pop<uint32_t>("IndexOffset is missing");
    const auto readLen = params.Get<uint64_t>("--read");

    LOG_ERROR("read: >" << readLen << "< group: >" << std::hex << group << "<offset:>" << offset << "<");

    std::vector<uint8_t> readBuffer(readLen);
    std::vector<uint8_t> writeBuffer;

    if (!isatty(fileno(stdin))) {
        char next_byte;
        while (std::cin.read(&next_byte, 1)) {
            writeBuffer.push_back(next_byte);
        }
    }

    if (!readBuffer.size() && !writeBuffer.size()) {
        LOG_ERROR("write- and read-size is zero!\n");
        return -1;
    }

    auto device = AdsDevice { gw, netid, port ? port : uint16_t(AMSPORT_R0_PLC_TC3) };
    long status = -1;
    uint32_t bytesRead = 0;
    if (!writeBuffer.size()) {
        status = device.ReadReqEx2(group,
                                   offset,
                                   readBuffer.size(),
                                   readBuffer.data(),
                                   &bytesRead);
    } else if (!readBuffer.size()) {
        status = device.WriteReqEx(group,
                                   offset,
                                   writeBuffer.size(),
                                   writeBuffer.data());
    } else {
        status = device.ReadWriteReqEx2(group,
                                        offset,
                                        readBuffer.size(),
                                        readBuffer.data(),
                                        writeBuffer.size(),
                                        writeBuffer.data(),
                                        &bytesRead);
    }

    if (ADSERR_NOERR != status) {
        LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex << status << '\n');
        return status;
    }
    std::cout.write((const char*)readBuffer.data(), readBuffer.size());
    return !std::cout.good();
}

int RunState(const AmsNetId netid, const uint16_t port, const std::string& gw, bhf::Commandline& args)
{
    auto device = AdsDevice { gw, netid, port ? port : uint16_t(10000) };
    const auto oldState = device.GetState();
    const auto value = args.Pop<const char*>();
    if (value) {
        const auto requestedState = std::stoi(value);
        if (requestedState >= ADSSTATE::ADSSTATE_MAXSTATES) {
            LOG_ERROR(
                "Requested state '" << std::dec << requestedState << "' exceeds max (" <<
                    uint16_t(ADSSTATE::ADSSTATE_MAXSTATES) <<
                    ")\n");
            return ADSERR_CLIENT_INVALIDPARM;
        }
        try {
            device.SetState(static_cast<ADSSTATE>(requestedState), oldState.device);
        } catch (const AdsException& ex) {
            // ignore AdsError 1861 after RUN/CONFIG mode change
            if (ex.errorCode != 1861) {
                throw;
            }
        }
    } else {
        std::cout << std::dec << (int)oldState.ads << '\n';
    }
    return 0;
}

template<typename T>
static T try_stoi(const char* str, const T defaultValue = 0)
{
    try {
        if (str && *str) {
            return static_cast<T>(std::stoi(++str));
        }
    } catch (...) {}
    return defaultValue;
}

int ParseCommand(int argc, const char* argv[])
{
    auto args = bhf::Commandline {usage, argc, argv};

    // drop argv[0] program name
    args.Pop<const char*>();
    const auto str = args.Pop<const char*>("Target is missing");
    const auto split = std::strcspn(str, ":");
    const auto netId = std::string {str, split};
    const auto port = try_stoi<uint16_t>(str + split);
    LOG_VERBOSE("NetId>" << netId << "< port>" << port << "<\n");

    bhf::ParameterList global = {
        {"--gw"},
    };
    args.Parse(global);

    const auto cmd = args.Pop<const char*>("Command is missing");
    if (!strcmp("addroute", cmd)) {
        return RunAddRoute(netId, args);
    } else if (!strcmp("netid", cmd)) {
        return RunNetId(netId);
    }

    const auto commands = CommandMap {
        {"raw", RunRaw},
        {"state", RunState},
    };
    const auto it = commands.find(cmd);
    if (it != commands.end()) {
        return it->second(make_AmsNetId(netId), port, global.Get<std::string>("--gw"), args);
    }
    LOG_ERROR("Unknown command >" << cmd << "<\n");
    return usage();
}

int main(int argc, const char* argv[])
{
    try {
        return ParseCommand(argc, argv);
    } catch (const AdsException& ex) {
        LOG_ERROR("AdsException message: " << ex.what() << '\n');
        return ex.errorCode;
    } catch (const std::exception& ex) {
        LOG_ERROR("Exception: " << ex.what() << '\n');
        return -2;
    } catch (...) {
        LOG_ERROR("Unknown exception\n");
        return -1;
    }
}