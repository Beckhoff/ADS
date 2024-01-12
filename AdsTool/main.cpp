// SPDX-License-Identifier: MIT
/**
    Copyright (C) 2021 - 2023 Beckhoff Automation GmbH & Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
 */

#include "AdsDevice.h"
#include "AdsFile.h"
#include "AdsLib.h"
#include "ECatAccess.h"
#include "LicenseAccess.h"
#include "Log.h"
#include "RegistryAccess.h"
#include "RouterAccess.h"
#include "RTimeAccess.h"
#include "SymbolAccess.h"
#include "bhf/ParameterList.h"
#include "bhf/StringToInteger.h"
#include "bhf/WindowsQuirks.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <set>
#include <thread>

static int version()
{
    std::cout << "0.0.25-1\n";
    return 0;
}

[[ noreturn ]] static void usage(const std::string& errorMessage = {})
{
    /*
     * "--help" is the only case we are called with an empty errorMessage. That
     * seems the only case we should really print to stdout instead of stderr.
     */
    (errorMessage.empty() ? std::cout : std::cerr) << errorMessage <<
        R"(
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
	--retry=<retries> Number of attemps to retry the entire command.
	--version Show version on stdout

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

	ecat list-masters
		Print a list of all active EtherCAT Masters and their respective AmsNetIds to stdout

	file read <path>
		Dump content of the file from <path> to stdout
	examples:
		Make a local backup of explorer.exe:
		$ adstool 5.24.37.144.1.1 file read 'C:\Windows\explorer.exe' > ./explorer.exe

		Show content of a text file:
		$ adstool 5.24.37.144.1.1 file read 'C:\Temp\hello world.txt'
		Hello World!

	file delete <path>
		Delete a file from <path>.
	examples:
		Delete a file over ADS and check if it still exists
		$ adstool 5.24.37.144.1.1 file delete 'C:\Temp\hello world.txt'
		$ adstool 5.24.37.144.1.1 file read 'C:\Temp\hello world.txt'
		$ echo \$?
		1804

	file write [--append] <path>
		Read data from stdin write to the file at <path>.
	examples:
		Write text directly into a file:
		$ printf 'Hello World!' | adstool 5.24.37.144.1.1 file write 'C:\Temp\hello world.txt'

		Copy local file to remote:
		$ adstool 5.24.37.144.1.1 file write 'C:\Windows\explorer.exe' < ./explorer.exe

	file find [--maxdepth=<depth>] <path>
		Find files the given path.
	examples:
		Show all files and directories in "C:/TwinCAT"
		$ adstool 5.24.37.144.1.1 file find 'C:/TwinCAT'

		Show only direct children of "C:/TwinCAT"
		$ adstool 5.24.37.144.1.1 file find --maxdepth=1 'C:/TwinCAT'

		Test if file or directory exists
		$ adstool 5.24.37.144.1.1 file find --maxdepth=0 'C:/TwinCAT'

	license < platformid | systemid | volumeno >
		Read license information from device.
	examples:
		Read platformid from device
		$ adstool 5.24.37.144.1.1 license platformid
		50

		Read systemid from device
		$ adstool 5.24.37.144.1.1 license systemid
		95EEFDE0-0392-1452-275F-1BF9ACCB924E
		50

		Read volume licence number from device
		$ adstool 5.24.37.144.1.1 license volumeno
		123456

	netid
		Read the AmsNetId from a remote TwinCAT router
		$ adstool 192.168.0.231 netid

	pciscan <pci_id>
		Show PCI devices with <pci_id>
	examples:
		List PCI CCAT devices:
		$ adstool 5.24.37.144.1.1 pciscan 0x15EC5000
		PCI devices found: 2
		3:0 @ 0x4028629004
		7:0 @ 0x4026531852

	plc read-symbol <name>
		Read the value of the symbol described by <name> and print it to stdout.
	examples:
		$ adstool 5.24.37.144.1.1 plc read-symbol "MAIN.nNum1"

	plc write-symbol <name> <value>
		Write the <value> to the symbol described by <name>
	examples:
		$ adstool 5.24.37.144.1.1 plc write-symbol "MAIN.nNum1" 10
		$ adstool 5.24.37.144.1.1 plc write-symbol "MAIN.nNum1" 0xA

	plc show-symbols
		Print information about all PLC symbols to stdout in JSON format.
	examples:
		Write PLC symbol information into an out.json file
		$ adstool 5.24.37.144.1.1 plc show-symbols > out.json

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

	registry export <key>
		Export registry key to stdout
	examples:
		Write registry key to stdout
		$ adstool 5.24.37.144.1.1 registry export 'HKEY_LOCAL_MACHINE\Software\Beckhoff\TwinCAT3'

	registry import
		Import registry from stdin
	examples:
		Import registry settings from a file, which was exported with RegEdit.exe on Windows
		$ dos2unix < file.reg | adstool 5.24.37.144.1.1 registry import

	rtime read-latency
		Read rtime latency information.
	examples:
		Read maximum rtime latency
		$ adstool 5.24.37.144.1.1 rtime read-latency
		6

	rtime reset-latency
		Show current maximum rtime latency and reset.
	examples:
		Read old maximum rtime latency and reset:
		$ adstool 5.24.37.144.1.1 rtime reset-latency
		6
		Show new maximum rtime latency:
		$ adstool 5.24.37.144.1.1 rtime read-latency
		1

	rtime set-shared-cores <num_shared_cores>
		Configure the number of shared cores. All remaining cores will be isolated.
		To configure all cores as shared use 0xffffffff for num_shared_cores.
		If the requested core configuration is already active on the device the tool
		will return ADSERR_DEVICE_EXISTS -> 1807 mod 256 -> 15. And the text:
		"Requested shared core configuration already active, no change applied."
	examples:
		Isolate all but one core
		$ adstool 5.24.37.144.1.1 rtime set-shared-cores 1

		Set all cores as shared core
		$ adstool 5.24.37.144.1.1 rtime set-shared-cores 0xffffffff

	startprocess [--directory=<directory>] [--hidden] <application> [<commandline>]
		Starts a new process <application> on the target device, optionally using the specified <commandline>
		and a different starting <directory>. For Windows targets the --hidden flag can be used to hide the
		application window.
	examples:
		List all files with details from the /var/log folder and write the result to /tmp/output.txt
		$ adstool 5.24.37.144.1.1 startprocess --directory=/var/log /usr/bin/ls "-l --all > /tmp/output.txt"

	state [<value>]
		Read or write the ADS state of the device at AmsPort (default 10000).
		ADS states are documented here:
		https://infosys.beckhoff.com/english.php?content=../content/1033/tc3_adsdll2/117556747.html
	examples:
		Check if TwinCAT is in RUN:
		$ adstool 5.24.37.144.1.1 state
		5

		Set TwinCAT to CONFIG mode:
		$ adstool 5.24.37.144.1.1 state 16

		Wait about one minute for TwinCAT to report either RUN or CONFIG mode:
		$ adstool 5.24.37.144.1.1 --retry=60 state --compare 5 15

	var [--type=<DATATYPE>] <variable name> [<value>]
		Reads/Write from/to a given PLC variable.
		If value is not set, a read operation will be executed. Otherwise 'value' will
		be written to the variable.

		On read, the content of a given PLC variable is written to stdout. Format of the
		output depends on DATATYPE.

		On write, <value> is written to the given PLC variable in an appropriate manner for
		that datatype. For strings, <value> will be written as-is. For integers
		value will be interpreted as decimal unless it starts with "0x". In that
		case it will be interpreted as hex.
	DATATYPE:
		BOOL -> default output as decimal
		BYTE -> default output as decimal
		WORD -> default output as decimal
		DWORD -> default output as decimal
		LWORD -> default output as decimal
		STRING -> default output as raw bytes
		...
	examples:
		Read number as decimal:
		$ adstool 5.24.37.144.1.1 var --type=DWORD "MAIN.nNum1"
		10

		Read string:
		$ adstool 5.24.37.144.1.1 var --type=STRING "MAIN.sString1"
		Hello World!

		Write a number:
		$ adstool 5.24.37.144.1.1 var --type=DWORD "MAIN.nNum1" "100"

		Write a hexvalue:
		$ adstool 5.24.37.144.1.1 var --type=DWORD "MAIN.nNum1" "0x64"

		Write string:
		$ adstool 5.24.37.144.1.1 var --type=STRING "MAIN.sString1" "Hello World!"
		$ adstool 5.24.37.144.1.1 var --type=STRING "MAIN.sString1"
		Hello World!

		Use quotes to write special characters:
		$ adstool 5.24.37.144.1.1 var "MAIN.sString1" "STRING" "\"Hello World\""
		$ adstool 5.24.37.144.1.1 var "MAIN.sString1" "STRING"
		"Hello World!"

)";
    exit(!errorMessage.empty());
}

typedef int (* CommandFunc)(const AmsNetId, const uint16_t, const std::string&, bhf::Commandline&);
using CommandMap = std::map<const std::string, CommandFunc>;

int RunAddRoute(const std::string& remote, bhf::Commandline& args)
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

int RunECat(const AmsNetId netid, const uint16_t port, const std::string& gw, bhf::Commandline& args)
{
    const auto command = args.Pop<std::string>("ecat command is missing (list)");

    auto device = bhf::ads::ECatAccess(gw, netid, port);
    if (!command.compare("list-masters")) {
        return device.ListECatMasters(std::cout);
    }

    LOG_ERROR(__FUNCTION__ << "(): Unknown ecat command '" << command << "'\n");
    return -1;
}

int RunFile(const AmsNetId netid, const uint16_t port, const std::string& gw, bhf::Commandline& args)
{
    const auto command = args.Pop<std::string>("file command is missing");
    auto device = AdsDevice { gw, netid, port ? port : uint16_t(10000) };

    if (!command.compare("read")) {
        const auto path = args.Pop<std::string>("path is missing");
        const AdsFile adsFile { device, path,
                                bhf::ads::FOPEN::READ | bhf::ads::FOPEN::BINARY |
                                bhf::ads::FOPEN::ENSURE_DIR};
        uint32_t bytesRead;
        do {
            std::vector<char> buf(1024 * 1024); // 1MB
            adsFile.Read(buf.size(), buf.data(), bytesRead);
            bhf::ForceBinaryOutputOnWindows();
            std::cout.write(buf.data(), bytesRead);
        } while (bytesRead > 0);
    } else if (!command.compare("write")) {
        bhf::ParameterList params = {
            {"--append", true},
        };
        args.Parse(params);
        const auto append = params.Get<bool>("--append");
        const auto flags = (append ? bhf::ads::FOPEN::APPEND : bhf::ads::FOPEN::WRITE) |
                           bhf::ads::FOPEN::BINARY |
                           bhf::ads::FOPEN::PLUS |
                           bhf::ads::FOPEN::ENSURE_DIR
        ;

        const auto path = args.Pop<std::string>("path is missing");
        const AdsFile adsFile { device, path, flags};
        std::vector<char> buf(1024 * 1024); // 1MB
        auto length = read(0, buf.data(), buf.size());
        while (length > 0) {
            adsFile.Write(length, buf.data());
            length = read(0, buf.data(), buf.size());
        }
    } else if (!command.compare("delete")) {
        const auto path = args.Pop<std::string>("path is missing");
        AdsFile::Delete(device, path, bhf::ads::FOPEN::READ | bhf::ads::FOPEN::ENABLE_DIR);
    } else if (!command.compare("find")) {
        bhf::ParameterList params = {
            {"--maxdepth"},
        };
        args.Parse(params);
        const auto maxdepth = params.Get<size_t>("--maxdepth", std::numeric_limits<size_t>::max());

        const auto path = args.Pop<std::string>("path is missing");
        return AdsFile::Find(device, path, maxdepth, std::cout);
    } else {
        LOG_ERROR(__FUNCTION__ << "(): Unknown file command '" << command << "'\n");
        return -1;
    }
    return 0;
}

int RunRegistry(const AmsNetId netid, const uint16_t port, const std::string& gw, bhf::Commandline& args)
{
    const auto command = args.Pop<std::string>("registry command is missing");

    if (!command.compare("verify")) {
        return bhf::ads::RegistryAccess::Verify(std::cin, std::cout);
    }

    const auto reg = bhf::ads::RegistryAccess { gw, netid, port };
    if (!command.compare("export")) {
        const auto key = args.Pop<std::string>("registry key is missing");
        return reg.Export(key, std::cout);
    }
    if (!command.compare("import")) {
        return reg.Import(std::cin);
    }
    LOG_ERROR(__FUNCTION__ << "(): Unknown registry command '" << command << "'\n");
    return -1;
}

int RunLicense(const AmsNetId netid, const uint16_t port, const std::string& gw, bhf::Commandline& args)
{
    auto device = bhf::ads::LicenseAccess{ gw, netid, port };
    const auto command = args.Pop<std::string>();

    if (!command.compare("platformid")) {
        return device.ShowPlatformId(std::cout);
    } else if (!command.compare("systemid")) {
        return device.ShowSystemId(std::cout);
    } else if (!command.compare("volumeno")) {
        return device.ShowVolumeNo(std::cout);
    } else {
        LOG_ERROR(__FUNCTION__ << "(): Unknown license command '" << command << "'\n");
        return -1;
    }
}

int RunNetId(const std::string& remote)
{
    AmsNetId netId;
    bhf::ads::GetRemoteAddress(remote, netId);
    std::cout << netId << '\n';
    return 0;
}

int RunPCIScan(const AmsNetId netid, const uint16_t port, const std::string& gw, bhf::Commandline& args)
{
    const auto device = bhf::ads::RouterAccess{ gw, netid, port };
    auto pciId = args.Pop<uint64_t>("pciscan pci_id is missing");

    /* allow subVendorId/SystemId to be omitted from cmd */
    if (std::numeric_limits<uint32_t>::max() >= pciId) {
        pciId <<= 32;
    }
    return device.PciScan(pciId, std::cout);
}

int RunPLC(const AmsNetId netid, const uint16_t port, const std::string& gw, bhf::Commandline& args)
{
    auto device = bhf::ads::SymbolAccess{ gw, netid, port };
    const auto command = args.Pop<std::string>("plc command is missing");

    if (!command.compare("read-symbol")) {
        const auto name = args.Pop<std::string>("Variable name is missing");
        return device.Read(name, std::cout);
    } else if (!command.compare("write-symbol")) {
        const auto name = args.Pop<std::string>("Variable name is missing");
        const auto value = args.Pop<std::string>("Value is missing");
        return device.Write(name, value);
    } else if (!command.compare("show-symbols")) {
        return device.ShowSymbols(std::cout);
    }
    LOG_ERROR(__FUNCTION__ << "(): Unknown PLC command '" << command << "'\n");
    return -1;
}

int RunRTime(const AmsNetId netid, const uint16_t port, const std::string& gw, bhf::Commandline& args)
{
    const auto command = args.Pop<std::string>("rtime command is missing");
    auto device = bhf::ads::RTimeAccess{ gw, netid, port };

    if (!command.compare("read-latency")) {
        return device.ShowLatency(RTIME_READ_LATENCY);
    } else if (!command.compare("reset-latency")) {
        return device.ShowLatency(RTIME_RESET_LATENCY);
    } else if (!command.compare("set-shared-cores")) {
        const auto sharedCores = args.Pop<uint32_t>("Number of shared cores is missing");
        return device.SetSharedCores(sharedCores);
    } else {
        LOG_ERROR(__FUNCTION__ << "(): Unknown rtime command'" << command << "'\n");
        return -1;
    }
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

    LOG_VERBOSE("read: >" << readLen << "< group: >" << std::hex << group << "<offset:>" << offset << "<");

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
    bhf::ForceBinaryOutputOnWindows();
    std::cout.write((const char*)readBuffer.data(), readBuffer.size());
    return !std::cout.good();
}

int RunStartProcess(const AmsNetId netid, const uint16_t port, const std::string& gw, bhf::Commandline& args)
{
    auto device = AdsDevice{ gw, netid, port ? port : uint16_t(10000) };

    bhf::ParameterList params = {
        {"--directory"},
        {"--hidden", true},
    };
    args.Parse(params);

    const auto application = args.Pop<std::string>("application is missing");
    if (std::numeric_limits<uint32_t>::max() < application.length()) {
        LOG_ERROR("The length of <application> exceeds its 32bit value limit");
        return 1;
    }

    const auto directory = params.Get<std::string>("--directory");
    if (std::numeric_limits<uint32_t>::max() < directory.length()) {
        LOG_ERROR("The length of <directory> exceeds its 32bit value limit");
        return 1;
    }

    const auto commandline = args.Pop<std::string>();
    if (std::numeric_limits<uint32_t>::max() < commandline.length()) {
        LOG_ERROR("The length of <commandline> exceeds its 32bit value limit");
        return 1;
    }

    struct AdsStartProcessHeader {
        uint32_t leApplicationLength;
        uint32_t leDirectoryLength;
        uint32_t leCommandlineLength;
        const uint8_t* cdata() const
        {
            return reinterpret_cast<const uint8_t*>(&leApplicationLength);
        }
    } header = {
        bhf::ads::htole<uint32_t>(application.length()),
        bhf::ads::htole<uint32_t>(directory.length()),
        bhf::ads::htole<uint32_t>(commandline.length()),
    };

    std::vector<uint8_t> data;
    std::copy_n(header.cdata(), sizeof(header), std::back_inserter(data));
    // empty strings (zero terminators) always need to be present
    std::move(application.begin(), application.end(), std::back_inserter(data));
    data.push_back(0);
    std::move(directory.begin(), directory.end(), std::back_inserter(data));
    data.push_back(0);
    std::move(commandline.begin(), commandline.end(), std::back_inserter(data));
    data.push_back(0);

    uint32_t opts = params.Get<bool>("--hidden") << sizeof(uint16_t) * 8;

    const auto status = device.WriteReqEx(SYSTEMSERVICE_STARTPROCESS, opts, data.size(), data.data());
    if (ADSERR_NOERR != status) {
        LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex << status << '\n');
    }

    return status;
}

int RunState(const AmsNetId netid, const uint16_t port, const std::string& gw, bhf::Commandline& args)
{
    bhf::ParameterList params = {
        {"--compare", true},
    };
    args.Parse(params);

    std::set<int> stateList;
    while (const auto value = args.Pop<const char*>()) {
        const auto requestedState = stateList.insert(std::stoi(value)).first;
        if (*requestedState >= ADSSTATE::ADSSTATE_MAXSTATES) {
            usage("Requested state '" + std::to_string(*requestedState) + "' exceeds max (" +
                  std::to_string(ADSSTATE::ADSSTATE_MAXSTATES) + ")\n");
        }
    }
    auto device = AdsDevice { gw, netid, port ? port : uint16_t(10000) };
    const auto oldState = device.GetState();
    if (params.Get<bool>("--compare")) {
        if (stateList.empty()) {
            usage("--compare requires at least one state");
        }
        // TODO: switch to std::set::contains() with c++20
        return stateList.end() == stateList.find(oldState.ads);
    }

    if (!stateList.empty()) {
        try {
            device.SetState(static_cast<ADSSTATE>(*stateList.begin()), oldState.device);
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
int PrintAs(const std::vector<uint8_t>& readBuffer)
{
    const auto v = *(reinterpret_cast<const T*>(readBuffer.data()));
    std::cout << std::dec << bhf::ads::letoh(v) << '\n';
    return !std::cout.good();
}

template<>
int PrintAs<uint8_t>(const std::vector<uint8_t>& readBuffer)
{
    std::cout << std::dec << (int)readBuffer[0] << '\n';
    return !std::cout.good();
}

template<typename T>
int Write(const AdsDevice& device, const AdsHandle& handle, const char* const value)
{
    const auto writeBuffer = bhf::ads::htole(bhf::StringTo<T>(value));
    const auto status = device.WriteReqEx(ADSIGRP_SYM_VALBYHND,
                                          *handle,
                                          sizeof(writeBuffer),
                                          &writeBuffer);
    return status;
}

int RunVar(const AmsNetId netid, const uint16_t port, const std::string& gw, bhf::Commandline& args)
{
    bhf::ParameterList params = {
        {"--type"},
    };
    args.Parse(params);

    const auto name = args.Pop<std::string>("Variable name is missing");
    const auto value = args.Pop<const char*>();
    static const std::map<const std::string, size_t> typeMap = {
        {"BOOL", 1},
        {"BYTE", 1},
        {"WORD", 2},
        {"DWORD", 4},
        {"LWORD", 8},
        {"STRING", 255},
    };
    const auto type = params.Get<std::string>("--type");
    const auto it = typeMap.find(type);
    if (typeMap.end() == it) {
        LOG_ERROR(__FUNCTION__ << "(): Unknown TwinCAT type '" << type << "'\n");
        return -1;
    }
    const auto size = it->second;

    auto device = AdsDevice { gw, netid, port ? port : uint16_t(AMSPORT_R0_PLC_TC3) };
    const auto handle = device.GetHandle(name);

    if (!value) {
        std::vector<uint8_t> readBuffer(size);
        uint32_t bytesRead = 0;
        const auto status = device.ReadReqEx2(ADSIGRP_SYM_VALBYHND,
                                              *handle,
                                              readBuffer.size(),
                                              readBuffer.data(),
                                              &bytesRead);
        if (ADSERR_NOERR != status) {
            LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex << status << '\n');
            return status;
        }

        switch (bytesRead) {
        case sizeof(uint8_t):
            return PrintAs<uint8_t>(readBuffer);

        case sizeof(uint16_t):
            return PrintAs<uint16_t>(readBuffer);

        case sizeof(uint32_t):
            return PrintAs<uint32_t>(readBuffer);

        case sizeof(uint64_t):
            return PrintAs<uint64_t>(readBuffer);

        default:
            bhf::ForceBinaryOutputOnWindows();
            std::cout.write((const char*)readBuffer.data(), bytesRead);
            return !std::cout.good();
        }
    }

    LOG_VERBOSE("name>" << name << "< value>" << value << "<\n");
    LOG_VERBOSE("size>" << size << "< value>" << value << "<\n");

    switch (size) {
    case sizeof(uint8_t):
        return Write<uint8_t>(device, handle, value);

    case sizeof(uint16_t):
        return Write<uint16_t>(device, handle, value);

    case sizeof(uint32_t):
        return Write<uint32_t>(device, handle, value);

    case sizeof(uint64_t):
        return Write<uint64_t>(device, handle, value);

    default:
        {
            auto writeBuffer = std::vector<char>(size);
            strncpy(writeBuffer.data(), value, writeBuffer.size());
            return device.WriteReqEx(ADSIGRP_SYM_VALBYHND,
                                     *handle,
                                     writeBuffer.size(),
                                     writeBuffer.data());
        }
    }
}

template<typename T>
int TryRun(T f)
{
    try {
        return f();
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

template<typename T>
int Run(T f, size_t retries)
{
    auto result = TryRun(f);

    // success or no retry allowed
    if (!result || !retries) {
        return result;
    }

    while (retries-- > 0) {
        LOG_WARN("Command failed, retrying...\n");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        result = TryRun(f);
        if (!result) {
            return 0;
        }
    }
    LOG_ERROR("Too many retries, giving up!\n");
    return result;
}

int ParseCommand(int argc, const char* argv[])
{
    auto args = bhf::Commandline {usage, argc, argv};

    // drop argv[0] program name
    args.Pop<const char*>();
    const auto str = args.Pop<const char*>("Target is missing");
    if (!strcmp("--help", str)) {
        usage();
    } else if (!strcmp("--version", str)) {
        return version();
    }
    const auto split = std::strcspn(str, ":");
    const auto netId = std::string {str, split};
    const auto port = bhf::try_stoi<uint16_t>(str + split);
    LOG_VERBOSE("NetId>" << netId << "< port>" << port << "<\n");

    bhf::ParameterList global = {
        {"--gw"},
        {"--localams"},
        {"--log-level"},
        {"--retry"},
    };
    args.Parse(global);

    const auto retries = global.Get<size_t>("--retry", 0);
    const auto localNetId = global.Get<std::string>("--localams");
    if (!localNetId.empty()) {
        bhf::ads::SetLocalAddress(make_AmsNetId(localNetId));
    }

    const auto logLevel = global.Get<size_t>("--log-level", 1);
    // highest loglevel is error==3, we allow 4 to disable all messages
    Logger::logLevel = std::min(logLevel, (size_t)4);

    const auto cmd = args.Pop<const char*>("Command is missing");
    if (!strcmp("addroute", cmd)) {
        return Run(std::bind(RunAddRoute, netId, args), retries);
    } else if (!strcmp("netid", cmd)) {
        return Run(std::bind(RunNetId, netId), retries);
    }

    const auto commands = CommandMap {
        {"ecat", RunECat},
        {"file", RunFile},
        {"registry", RunRegistry},
        {"license", RunLicense},
        {"pciscan", RunPCIScan},
        {"plc", RunPLC},
        {"raw", RunRaw},
        {"rtime", RunRTime},
        {"startprocess", RunStartProcess},
        {"state", RunState},
        {"var", RunVar},
    };
    const auto it = commands.find(cmd);
    if (it != commands.end()) {
        return Run(std::bind(it->second, make_AmsNetId(netId), port, global.Get<std::string>("--gw"), args), retries);
    }
    usage(std::string {"Unknown command >"} + cmd + "<\n");
}

int main(int argc, const char* argv[])
{
    return TryRun(std::bind(ParseCommand, argc, argv));
}
