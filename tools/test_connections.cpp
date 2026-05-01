// SPDX-License-Identifier: MIT
/**
 * Connection and data-integrity test for plain, SSC, SCA and PSK ADS.
 * Usage:
 *   test_connections <target-netid> --gw=<ip[:port]> --localams=<ams>
 *                   [--mode=plain|ssc|sca|psk]
 *                   [--cert=<path>] [--key=<path>] [--ca=<path>]
 *                   [--username=<u>] [--password=<p>]
 *                   [--psk-identity=<id>]
 */
#include <AdsLib.h>

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>

// ── TwinCAT primitive sizes ─────────────────────────────────────────────────
using TC_BOOL = uint8_t; // BOOL  (1 byte)
using TC_INT = int16_t; // INT   (2 bytes)
using TC_DINT = int32_t; // DINT  (4 bytes)
using TC_LREAL = double; // LREAL (8 bytes)

static constexpr uint32_t STR80_LEN = 81; // STRING(80) + NUL
static constexpr uint32_t STR5_LEN = 6; // STRING(5)  + NUL

// ── Argument helper ─────────────────────────────────────────────────────────
static std::string getArg(int argc, char **argv, const char *name,
			  const char *defVal = "")
{
	const std::string prefix = std::string("--") + name + "=";
	for (int i = 2; i < argc; ++i) {
		const std::string a(argv[i]);
		if (a.substr(0, prefix.size()) == prefix) {
			return a.substr(prefix.size());
		}
	}
	return defVal;
}

// ── ADS state name ──────────────────────────────────────────────────────────
static const char *adsStateName(uint16_t state)
{
	switch (state) {
	case ADSSTATE_INVALID:
		return "INVALID";
	case ADSSTATE_IDLE:
		return "IDLE";
	case ADSSTATE_RESET:
		return "RESET";
	case ADSSTATE_INIT:
		return "INIT";
	case ADSSTATE_START:
		return "START";
	case ADSSTATE_RUN:
		return "RUN";
	case ADSSTATE_STOP:
		return "STOP";
	case ADSSTATE_SAVECFG:
		return "SAVECFG";
	case ADSSTATE_LOADCFG:
		return "LOADCFG";
	case ADSSTATE_POWERFAILURE:
		return "POWERFAILURE";
	case ADSSTATE_POWERGOOD:
		return "POWERGOOD";
	case ADSSTATE_ERROR:
		return "ERROR";
	case ADSSTATE_SHUTDOWN:
		return "SHUTDOWN";
	case ADSSTATE_SUSPEND:
		return "SUSPEND";
	case ADSSTATE_RESUME:
		return "RESUME";
	case ADSSTATE_CONFIG:
		return "CONFIG";
	case ADSSTATE_RECONFIG:
		return "RECONFIG";
	default:
		return "UNKNOWN";
	}
}

// ── RAII symbol handle ───────────────────────────────────────────────────────
struct SymbolHandle {
	long port;
	const AmsAddr *server;
	uint32_t handle{ 0 };
	long status{ 0 };

	SymbolHandle(long p, const AmsAddr *s, const std::string &name)
		: port(p)
		, server(s)
	{
		uint32_t bytesRead = 0;
		status = AdsSyncReadWriteReqEx2(
			port, server, ADSIGRP_SYM_HNDBYNAME, 0, sizeof(handle),
			&handle, static_cast<uint32_t>(name.size()),
			name.c_str(), &bytesRead);
	}

	~SymbolHandle()
	{
		if (!status) {
			AdsSyncWriteReqEx(port, server, ADSIGRP_SYM_RELEASEHND,
					  0, sizeof(handle), &handle);
		}
	}

	bool ok() const
	{
		return status == 0;
	}

	SymbolHandle(const SymbolHandle &) = delete;
	SymbolHandle &operator=(const SymbolHandle &) = delete;
};

// ── Typed read by symbol name ────────────────────────────────────────────────
template <typename T>
static long readVar(long port, const AmsAddr *server, const std::string &name,
		    T &out)
{
	SymbolHandle sym(port, server, name);
	if (!sym.ok()) {
		std::cerr << "  GetHandle('" << name << "') failed: 0x"
			  << std::hex << sym.status << std::dec << "\n";
		return sym.status;
	}
	uint32_t bytesRead = 0;
	const long st = AdsSyncReadReqEx2(port, server, ADSIGRP_SYM_VALBYHND,
					  sym.handle, sizeof(T), &out,
					  &bytesRead);
	if (st) {
		std::cerr << "  Read('" << name << "') failed: 0x" << std::hex
			  << st << std::dec << "\n";
	}
	return st;
}

// ── String read by symbol name ───────────────────────────────────────────────
static long readString(long port, const AmsAddr *server,
		       const std::string &name, char *buf, uint32_t bufLen)
{
	SymbolHandle sym(port, server, name);
	if (!sym.ok()) {
		std::cerr << "  GetHandle('" << name << "') failed: 0x"
			  << std::hex << sym.status << std::dec << "\n";
		return sym.status;
	}
	memset(buf, 0, bufLen);
	uint32_t bytesRead = 0;
	const long st = AdsSyncReadReqEx2(port, server, ADSIGRP_SYM_VALBYHND,
					  sym.handle, bufLen, buf, &bytesRead);
	if (st) {
		std::cerr << "  Read('" << name << "') failed: 0x" << std::hex
			  << st << std::dec << "\n";
	}
	buf[bufLen - 1] = '\0';
	return st;
}

// ── Typed write by symbol name ────────────────────────────────────────────────
template <typename T>
static long writeVar(long port, const AmsAddr *server, const std::string &name,
		     const T &value)
{
	SymbolHandle sym(port, server, name);
	if (!sym.ok()) {
		std::cerr << "  GetHandle('" << name << "') failed: 0x"
			  << std::hex << sym.status << std::dec << "\n";
		return sym.status;
	}
	const long st = AdsSyncWriteReqEx(port, server, ADSIGRP_SYM_VALBYHND,
					  sym.handle, sizeof(T), &value);
	if (st) {
		std::cerr << "  Write('" << name << "') failed: 0x" << std::hex
			  << st << std::dec << "\n";
	}
	return st;
}

// ── Test: read the six Main variables ────────────────────────────────────────
static bool testVariableReads(long port, const AmsAddr *server)
{
	std::cout << "\n=== Variable Read Test ===\n";
	bool ok = true;

	// Main.Bool1  [BOOL]
	TC_BOOL b1 = 0;
	if (readVar(port, server, "Main.Bool1", b1) == 0) {
		std::cout << "  Main.Bool1  [BOOL]       = "
			  << (b1 ? "TRUE" : "FALSE") << "\n";
	} else {
		ok = false;
	}

	// Main.count  [INT]
	TC_INT count = 0;
	if (readVar(port, server, "Main.count", count) == 0) {
		std::cout << "  Main.count  [INT]        = " << count << "\n";
	} else {
		ok = false;
	}

	// Main.dint   [DINT]
	TC_DINT dint = 0;
	if (readVar(port, server, "Main.dint1", dint) == 0) {
		std::cout << "  Main.dint   [DINT]       = " << dint << "\n";
	} else {
		ok = false;
	}

	// Main.lreal  [LREAL]
	TC_LREAL lreal = 0.0;
	if (readVar(port, server, "Main.lreal1", lreal) == 0) {
		std::cout << "  Main.lreal  [LREAL]      = " << std::fixed
			  << std::setprecision(6) << lreal << "\n";
	} else {
		ok = false;
	}

	// Main.str1   [STRING(80)]
	char str1[STR80_LEN] = {};
	if (readString(port, server, "Main.str1", str1, STR80_LEN) == 0) {
		std::cout << "  Main.str1   [STRING(80)] = \"" << str1
			  << "\"\n";
	} else {
		ok = false;
	}

	// Main.str2   [STRING(5)]
	char str2[STR5_LEN] = {};
	if (readString(port, server, "Main.str2", str2, STR5_LEN) == 0) {
		std::cout << "  Main.str2   [STRING(5)]  = \"" << str2
			  << "\"\n";
	} else {
		ok = false;
	}

	return ok;
}

// ── Test: write a sentinel to Main.dint, read back, verify ───────────────────
static bool testWriteVerify(long port, const AmsAddr *server)
{
	std::cout << "\n=== Write / Verify Round-Trip Test (Main.dint1) ===\n";

	const TC_DINT sentinel = 0x4242;

	// Save original value
	TC_DINT original = 0;
	if (readVar(port, server, "Main.dint1", original) != 0) {
		return false;
	}
	std::cout << "  Original value   = " << original << "\n";

	// Write sentinel
	std::cout << "  Writing          = " << sentinel << " ... ";
	if (writeVar(port, server, "Main.dint1", sentinel) != 0) {
		std::cout << "FAIL\n";
		return false;
	}
	std::cout << "OK\n";

	// Read back
	TC_DINT readback = 0;
	std::cout << "  Reading back     = ";
	if (readVar(port, server, "Main.dint1", readback) != 0) {
		return false;
	}
	std::cout << readback << " ... ";

	if (readback != sentinel) {
		std::cout << "MISMATCH (expected " << sentinel << ")\n";
		// Restore original before failing
		writeVar(port, server, "Main.dint1", original);
		return false;
	}
	std::cout << "MATCH\n";

	// Restore original value
	std::cout << "  Restoring        = " << original << " ... ";
	if (writeVar(port, server, "Main.dint1", original) != 0) {
		return false;
	}
	std::cout << "OK\n";

	return true;
}

// ── Test: measure round-trip latency over N reads of Main.dint ───────────────
static bool testLatency(long port, const AmsAddr *server)
{
	std::cout << "\n=== Latency Test (20 reads of Main.dint1) ===\n";

	SymbolHandle sym(port, server, "Main.dint1");
	if (!sym.ok()) {
		std::cerr << "  GetHandle failed: 0x" << std::hex << sym.status
			  << "\n";
		return false;
	}

	using Clock = std::chrono::steady_clock;
	using us = std::chrono::microseconds;

	long long minUs = std::numeric_limits<long long>::max();
	long long maxUs = 0;
	long long sumUs = 0;
	const int N = 20;

	for (int i = 0; i < N; ++i) {
		TC_DINT val = 0;
		uint32_t bytes = 0;
		const auto t0 = Clock::now();
		const long st = AdsSyncReadReqEx2(port, server,
						  ADSIGRP_SYM_VALBYHND,
						  sym.handle, sizeof(val), &val,
						  &bytes);
		const auto dt =
			std::chrono::duration_cast<us>(Clock::now() - t0)
				.count();

		if (st) {
			std::cerr << "  Read failed on iteration " << i
				  << ": 0x" << std::hex << st << "\n";
			return false;
		}
		sumUs += dt;
		if (dt < minUs)
			minUs = dt;
		if (dt > maxUs)
			maxUs = dt;
	}

	const double avgMs = static_cast<double>(sumUs) / N / 1000.0;
	const double minMs = static_cast<double>(minUs) / 1000.0;
	const double maxMs = static_cast<double>(maxUs) / 1000.0;

	std::cout << std::fixed << std::setprecision(3);
	std::cout << "  Avg: " << avgMs << " ms"
		  << "   Min: " << minMs << " ms"
		  << "   Max: " << maxMs << " ms\n";
	return true;
}

// ── main ─────────────────────────────────────────────────────────────────────
int main(int argc, char **argv)
{
	if (argc < 2) {
		std::cerr
			<< "Usage: test_connections <target-netid> [OPTIONS]\n"
			<< "  --gw=<ip>             Gateway IP (required)\n"
			<< "  --localams=<ams>      Local AMS NetId\n"
			<< "  --mode=plain|ssc|sca|psk  (default: plain)\n"
			<< "  --cert=<path>         Client certificate (PEM, SSC/SCA)\n"
			<< "  --key=<path>          Client private key (PEM, SSC/SCA)\n"
			<< "  --ca=<path>           CA certificate for SCA (PEM)\n"
			<< "  --username=<u>        SSC first-time registration\n"
			<< "  --password=<p>        SSC password / PSK password\n"
			<< "  --psk-identity=<id>   PSK identity string\n";
		return 1;
	}

	const AmsNetId targetNetId{ argv[1] };
	const AmsAddr server{ targetNetId, AMSPORT_R0_PLC_TC3 };

	const auto gw = getArg(argc, argv, "gw");
	const auto localAms = getArg(argc, argv, "localams");
	const auto mode = getArg(argc, argv, "mode", "plain");
	const auto certPath = getArg(argc, argv, "cert");
	const auto keyPath = getArg(argc, argv, "key");
	const auto caPath = getArg(argc, argv, "ca");
	const auto username = getArg(argc, argv, "username");
	const auto password = getArg(argc, argv, "password");
	const auto pskIdentity = getArg(argc, argv, "psk-identity");

	if (gw.empty()) {
		std::cerr << "Error: --gw is required\n";
		return 1;
	}

	if (!localAms.empty()) {
		bhf::ads::SetLocalAddress(AmsNetId{ localAms });
	}

	long routeResult = -1;

	if (mode == "plain") {
		routeResult = bhf::ads::AddLocalRoute(targetNetId, gw.c_str());
	} else if (mode == "ssc" || mode == "sca") {
		if (certPath.empty() || keyPath.empty()) {
			std::cerr << "Error: --cert and --key required for "
				  << mode << " mode\n";
			return 1;
		}
		bhf::ads::SecureAdsConfig cfg;
		cfg.certPath = certPath;
		cfg.keyPath = keyPath;
		if (mode == "ssc") {
			cfg.mode = bhf::ads::SecureAdsConfig::Mode::SSC;
			cfg.username = username;
			cfg.password = password;
		} else {
			cfg.mode = bhf::ads::SecureAdsConfig::Mode::SCA;
			cfg.caPath = caPath;
		}
		routeResult =
			bhf::ads::AddSecureRoute(targetNetId, gw.c_str(), cfg);
	} else if (mode == "psk") {
		if (pskIdentity.empty() || password.empty()) {
			std::cerr
				<< "Error: --psk-identity and --password required for psk mode\n";
			return 1;
		}
		bhf::ads::SecureAdsConfig cfg;
		cfg.mode = bhf::ads::SecureAdsConfig::Mode::PSK;
		cfg.pskIdentity = pskIdentity;
		cfg.password = password;
		routeResult =
			bhf::ads::AddSecureRoute(targetNetId, gw.c_str(), cfg);
	} else {
		std::cerr << "Error: unknown mode '" << mode << "'\n";
		return 1;
	}

	if (routeResult) {
		std::cerr << "AddRoute failed: 0x" << std::hex << routeResult
			  << "\n";
		return 1;
	}

	const long port = AdsPortOpenEx();
	if (!port) {
		std::cerr << "AdsPortOpenEx failed\n";
		bhf::ads::DelLocalRoute(targetNetId);
		return 1;
	}

	int rc = 0;

	// ── Device info ───────────────────────────────────────────────────────
	std::cout << "\n=== Device Info ===\n";
	char devName[17] = {};
	AdsVersion version{ 0, 0, 0 };
	long status =
		AdsSyncReadDeviceInfoReqEx(port, &server, devName, &version);
	if (status) {
		std::cerr << "  ReadDeviceInfo failed: 0x" << std::hex << status
			  << "\n";
		rc = 1;
	} else {
		std::cout << "  Device:    " << devName << " v"
			  << static_cast<int>(version.version) << "."
			  << static_cast<int>(version.revision) << "."
			  << version.build << "\n";
	}

	// ── ADS state ─────────────────────────────────────────────────────────
	uint16_t adsState = 0, devState = 0;
	status = AdsSyncReadStateReqEx(port, &server, &adsState, &devState);
	if (status) {
		std::cerr << "  ReadState failed: 0x" << std::hex << status
			  << "\n";
		rc = 1;
	} else {
		std::cout << "  ADS State: " << adsStateName(adsState) << " ("
			  << std::dec << adsState << ")"
			  << "   Device State: " << devState << "\n";
	}

	// ── Variable reads ────────────────────────────────────────────────────
	if (!testVariableReads(port, &server)) {
		rc = 1;
	}

	// ── Write / verify round-trip ─────────────────────────────────────────
	if (!testWriteVerify(port, &server)) {
		rc = 1;
	}

	// ── Latency ───────────────────────────────────────────────────────────
	if (!testLatency(port, &server)) {
		rc = 1;
	}

	std::cout << "\n=== Result: " << (rc == 0 ? "PASS" : "FAIL")
		  << " ===\n";

	AdsPortCloseEx(port);
	bhf::ads::DelLocalRoute(targetNetId);
	return rc;
}