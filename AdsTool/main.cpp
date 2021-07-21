// SPDX-License-Identifier: MIT
/**
    Copyright (C) 2021 Beckhoff Automation GmbH & Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
 */

#include "AdsException.h"
#include "AdsLib.h"
#include "Log.h"
#include "ParameterList.h"
#include <cstring>
#include <iostream>
#include <vector>

int usage(const char* const errorMessage = nullptr)
{
    if (errorMessage) {
        LOG_ERROR(errorMessage);
    }
    std::cout <<
        R"(
USAGE:
	<target> <command> [CMD_OPTIONS...] [<command_parameter>...]

	target: hostname or IP address of your target

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
)";
    exit(1);
}

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

    const auto cmd = args.Pop<const char*>("Command is missing");
    if (!strcmp("addroute", cmd)) {
        return RunAddRoute(netId, args);
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
