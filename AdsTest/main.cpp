// SPDX-License-Identifier: MIT
/**
    Copyright (C) 2023 Beckhoff Automation GmbH & Co. KG
    Author: Jan Ole Hüser <j.hueser@beckhoff.com>
 */

#include "AdsLib.h"
#include "Log.h"
#include "RegistryAccessTest.h"
#include "bhf/ParameterList.h"
#include "bhf/StringToInteger.h"
#include "bhf/WindowsQuirks.h"
#include <iostream>
#include <cstring>

[[noreturn]] static void usage(const std::string &errorMessage = {})
{
	(errorMessage.empty() ? std::cout : std::cerr) << errorMessage <<
		R"~~~(
USAGE:
	[<target[:port]>] [OPTIONS...] <command> [CMD_OPTIONS...] [<command_parameter>...]

	target: AmsNetId, hostname or IP address of your target
	port: AmsPort if omitted the default is command specific

OPTIONS:
	--gw=<hostname> or IP address of your AmsNetId target (mandatory in standalone mode)
	--help Show this message on stdout
	--localams=<netid> Specify your own AmsNetId (by default derived from local IP + ".1.1")
	--log-level=<verbosity> Messages will be shown if their own level is equal or less to verbosity.
		0 verbose | Show all messages, even if they are only useful to developers
		1 info    | (DEFAULT) Show everything, but the verbose stuff
		2 warn    | Don't show informational messages, just warnings and errors
		3 error   | Don't care about warnigs, show errors only
		4 silent  | Stay silent, don't log anything

COMMANDS:
	registry testdeletekey <key>
		Test the deletion of a registry key.
	examples:
		$ AdsTest 5.24.37.144.1.1 registry testdeletekey

)~~~";
	exit(errorMessage.empty() ? 0 : 1);
}

typedef int (*CommandFunc)(const AmsNetId, const uint16_t, const std::string &,
			   bhf::Commandline &);
using CommandMap = std::map<const std::string, CommandFunc>;

static int RunRegistry(const AmsNetId netid, const uint16_t port,
		       const std::string &gw, bhf::Commandline &args)
{
	const auto command =
		args.Pop<std::string>("registry command is missing");

	if (!command.compare("testdeletekey")) {
		return bhf::adstest::testDeleteRegistryKey(gw, netid, port);
	}

	LOG_ERROR(__FUNCTION__ << "(): Unknown registry command '" << command
			       << "'\n");
	return -1;
}

int ParseCommand(int argc, const char *argv[])
{
	auto args = bhf::Commandline{ usage, argc, argv };

	// drop argv[0] program name
	args.Pop<const char *>();

	const auto str = args.Pop<const char *>("Target is missing");
	if (!strcmp("--help", str)) {
		usage();
	}

	const auto split = std::strcspn(str, ":");
	const auto netId = std::string{ str, split };
	const auto port = bhf::try_stoi<uint16_t>(str + split);
	LOG_VERBOSE("NetId>" << netId << "< port>" << port << "<\n");

	bhf::ParameterList global = {
		{ "--gw" },
		{ "--localams" },
		{ "--log-level" },
	};
	args.Parse(global);

	const auto localNetId = global.Get<std::string>("--localams");
	if (!localNetId.empty()) {
		bhf::ads::SetLocalAddress(make_AmsNetId(localNetId));
	}

	const auto logLevel = global.Get<size_t>("--log-level", 1);
	// highest loglevel is error==3, we allow 4 to disable all messages
	Logger::logLevel = std::min(logLevel, (size_t)4);

	const auto cmd = args.Pop<const char *>("Command is missing");

	const auto commands = CommandMap{
		{ "registry", RunRegistry },
	};
	const auto it = commands.find(cmd);
	if (it != commands.end()) {
		return it->second(make_AmsNetId(netId), port,
				  global.Get<std::string>("--gw"), args);
	}
	usage(std::string{ "Unknown command >" } + cmd + "<\n");

	return 0;
}

int main(int argc, const char *argv[])
{
	bhf::ForceBinaryOutputOnWindows();
	return ParseCommand(argc, argv);
}
