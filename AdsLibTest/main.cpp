
#include <AdsLib.h>

#include "AmsRouter.h"

#include <iostream>

#pragma warning(push, 0)
#include <fructose/fructose.h>
#pragma warning(pop)
using namespace fructose;

#pragma warning(disable: 4800)

void print(const AmsAddr &addr, std::ostream &out)
{
	out << "AmsAddr: "
		<< (int)addr.netId.b[0] << '.' << (int)addr.netId.b[1] << '.' << (int)addr.netId.b[2] << '.'
		<< (int)addr.netId.b[3] << '.' << (int)addr.netId.b[4] << '.' << (int)addr.netId.b[5] << ':'
		<< addr.port << '\n';
}

long testPortOpen(std::ostream &out)
{
	long port = AdsPortOpenEx();
	if (!port) {
		return 0;
	}

	AmsAddr addr;
	if (AdsGetLocalAddressEx(port, &addr)) {
		AdsPortCloseEx(port);
		return 0;
	}
	out << "Port: " << port << ' ';
	print(addr, out);
	return port;
}

struct TestAmsRouter : test_base < TestAmsRouter >
{
	std::ostream &out;

	TestAmsRouter(std::ostream& outstream)
		: out(outstream)
	{}

	void testAmsRouterAddRoute(const std::string&)
	{
		static const AmsNetId netId_1{ 192, 168, 0, 231, 1, 1 };
		static const AmsNetId netId_2{ 127, 0, 0, 1, 2, 1 };
		static const IpV4 ip_local("127.0.0.1");
		static const IpV4 ip_remote("192.168.0.232");
		AmsRouter testee;

		// test new Ams with new Ip
		fructose_assert(testee.AddRoute(netId_1, ip_local));
		fructose_assert(testee.GetConnection(netId_1));
		fructose_assert(ip_local == testee.GetConnection(netId_1)->destIp);

		// existent Ams with new Ip -> close old connection
		fructose_assert(testee.AddRoute(netId_1, ip_remote));
		fructose_assert(testee.GetConnection(netId_1));
		fructose_assert(ip_remote == testee.GetConnection(netId_1)->destIp);

		// new Ams with existent Ip
		fructose_assert(testee.AddRoute(netId_2, ip_remote));
		fructose_assert(testee.GetConnection(netId_2));
		fructose_assert(ip_remote == testee.GetConnection(netId_2)->destIp);

		// already exists
		fructose_assert(testee.AddRoute(netId_1, ip_remote));
		fructose_assert(testee.GetConnection(netId_1));
		fructose_assert(ip_remote == testee.GetConnection(netId_1)->destIp);
	}

	void testAmsRouterDelRoute(const std::string&)
	{
		static const AmsNetId netId_1{ 192, 168, 0, 231, 1, 1 };
		static const AmsNetId netId_2{ 127, 0, 0, 1, 2, 1 };
		static const IpV4 ip_local("127.0.0.1");
		static const IpV4 ip_remote("192.168.0.232");
		AmsRouter testee;

		// add + remove -> null
		fructose_assert(testee.AddRoute(netId_1, ip_remote));
		fructose_assert(testee.GetConnection(netId_1));
		fructose_assert(ip_remote == testee.GetConnection(netId_1)->destIp);
		testee.DelRoute(netId_1);
		fructose_assert(!testee.GetConnection(netId_1));

		// add_1, add_2, remove_1 -> null_1 valid_2
		fructose_assert(testee.AddRoute(netId_1, ip_remote));
		fructose_assert(testee.GetConnection(netId_1));
		fructose_assert(ip_remote == testee.GetConnection(netId_1)->destIp);
		fructose_assert(testee.AddRoute(netId_2, ip_local));
		fructose_assert(testee.GetConnection(netId_2));
		fructose_assert(ip_local == testee.GetConnection(netId_2)->destIp);
		testee.DelRoute(netId_1);
		fructose_assert(!testee.GetConnection(netId_1));
		fructose_assert(testee.GetConnection(netId_2));
	}
};
#include <chrono>
struct TestAds : test_base < TestAds >
{
	std::ostream &out;

	TestAds(std::ostream& outstream)
		: out(outstream)
	{
		AdsAddRoute(AmsNetId{ 192, 168, 0, 231, 1, 1 }, IpV4{ "192.168.0.232" });
	}

	void testAdsPortOpenEx(const std::string&)
	{
		static const size_t NUM_TEST_PORTS = 2;
		long port[NUM_TEST_PORTS];


		for (int i = 0; i < NUM_TEST_PORTS; ++i) {
			port[i] = testPortOpen(out);
			fructose_loop_assert(i, 0 != port[i]);
		}

		for (int i = 0; i < NUM_TEST_PORTS; ++i) {
			if (port[i]) {
				fructose_loop_assert(i, !AdsPortCloseEx(port[i]));
			}
		}

	}

	void testAdsReadReqEx2(const std::string&)
	{
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();
		fructose_assert(0 != port);

		print(server, out);

		uint32_t group = 0x4020;
		uint32_t offset = 4;
		uint32_t bytesRead;
		uint32_t buffer;

		for (int i = 0; i < 10; ++i) {
			fructose_assert(0 == AdsSyncReadReqEx2(port, &server, group, offset, sizeof(buffer), &buffer, &bytesRead));
			fructose_assert(0 == buffer);
		}
		fructose_assert(0 == AdsPortCloseEx(port));
	}

	void testAdsReadDeviceInfoReqEx(const std::string&)
	{
		static const char NAME[] = "Plc30 App";
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();
		fructose_assert(0 != port);

		for (int i = 0; i < 2; ++i) {
			AdsVersion version{ 0, 0, 0 };
			char devName[16 + 1]{};
			fructose_assert(0 == AdsSyncReadDeviceInfoReqEx(port, &server, devName, &version));
			fructose_assert(3 == version.version);
			fructose_assert(1 == version.revision);
			fructose_assert(1101 == version.build);
			fructose_assert(0 == strncmp(devName, NAME, sizeof(NAME)));

			out << "AdsSyncReadDeviceInfoReqEx() name: " << devName
				<< " Version: " << (int)version.version << '.' << (int)version.revision << '.' << (int)version.build << '\n';
		}
		fructose_assert(0 == AdsPortCloseEx(port));
	}

	void testAdsReadStateReqEx(const std::string&)
	{
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();
		fructose_assert(0 != port);


		uint16_t adsState;
		uint16_t devState;
		fructose_assert(0 == AdsSyncReadStateReqEx(port, &server, &adsState, &devState));
		fructose_assert(ADSSTATE_RUN == adsState);
		fructose_assert(0 == devState);
		fructose_assert(0 == AdsPortCloseEx(port));

		out << "AdsSyncReadStateReqEx() adsState: " << (int)adsState
			<< " device: " << (int)devState << '\n';
	}

	void testAdsWriteReqEx(const std::string&)
	{
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();
		fructose_assert(0 != port);

		print(server, out);

		uint32_t group = 0x4020;
		uint32_t offset = 4;
		uint32_t bytesRead;
		uint32_t buffer;
		uint32_t outBuffer = 0xDEADBEEF;

		for (int i = 0; i < 10; ++i) {
			fructose_assert(0 == AdsSyncWriteReqEx(port, &server, group, offset, sizeof(outBuffer), &outBuffer));
			fructose_assert(0 == AdsSyncReadReqEx2(port, &server, group, offset, sizeof(buffer), &buffer, &bytesRead));
			fructose_assert(outBuffer == buffer);
			outBuffer = ~outBuffer;
		}
		const uint32_t defaultValue = 0;
		fructose_assert(0 == AdsSyncWriteReqEx(port, &server, group, offset, sizeof(defaultValue), &defaultValue));
		fructose_assert(0 == AdsPortCloseEx(port));
	}

	void testAdsWriteControlReqEx(const std::string&)
	{
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();
		fructose_assert(0 != port);

		uint32_t outBuffer = 0xDEADBEEF;
		unsigned long bytesRead;
		uint32_t buffer;
		uint16_t adsState;
		uint16_t devState;

		for (int i = 0; i < 10; ++i) {
			fructose_assert(0 == AdsSyncWriteControlReqEx(port, &server, ADSSTATE_STOP, 0, 0, nullptr));
			fructose_assert(0 == AdsSyncReadStateReqEx(port, &server, &adsState, &devState));
			fructose_assert(ADSSTATE_STOP == adsState);
			fructose_assert(0 == devState);
			fructose_assert(0 == AdsSyncWriteControlReqEx(port, &server, ADSSTATE_RUN, 0, 0, nullptr));
			fructose_assert(0 == AdsSyncReadStateReqEx(port, &server, &adsState, &devState));
			fructose_assert(ADSSTATE_RUN == adsState);
			fructose_assert(0 == devState);
		}
		fructose_assert(0 == AdsPortCloseEx(port));
	}

	void testAdsTimeout(const std::string&)
	{
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();
		uint32_t timeout;

		fructose_assert(0 != port);
		fructose_assert(ADSERR_CLIENT_PORTNOTOPEN == AdsSyncGetTimeoutEx(55555, &timeout));
		fructose_assert(0 == AdsSyncGetTimeoutEx(port, &timeout));
		fructose_assert(5000 == timeout);
		fructose_assert(0 == AdsSyncSetTimeoutEx(port, 1000));
		fructose_assert(0 == AdsSyncGetTimeoutEx(port, &timeout));
		fructose_assert(1000 == timeout);
		fructose_assert(0 == AdsSyncSetTimeoutEx(port, 5000));
		fructose_assert(0 == AdsPortCloseEx(port));
	}
};

int main()
{
	std::ostream nowhere(0);
#if 0
	std::ostream& errorstream = nowhere;
#else
	std::ostream& errorstream = std::cout;
#endif
	TestAmsRouter routerTest(errorstream);
	routerTest.add_test("testAmsRouterAddRoute", &TestAmsRouter::testAmsRouterAddRoute);
	routerTest.add_test("testAmsRouterDelRoute", &TestAmsRouter::testAmsRouterDelRoute);
	routerTest.run();

	TestAds adsTest(errorstream);
	adsTest.add_test("testAdsPortOpenEx", &TestAds::testAdsPortOpenEx);
	adsTest.add_test("testAdsReadReqEx2", &TestAds::testAdsReadReqEx2);
	adsTest.add_test("testAdsReadDeviceInfoReqEx", &TestAds::testAdsReadDeviceInfoReqEx);
	adsTest.add_test("testAdsReadStateReqEx", &TestAds::testAdsReadStateReqEx);
	adsTest.add_test("testAdsWriteReqEx", &TestAds::testAdsWriteReqEx);
	adsTest.add_test("testAdsWriteControlReqEx", &TestAds::testAdsWriteControlReqEx);
	// AddNotification
	// DelNotification
	adsTest.add_test("testAdsTimeout", &TestAds::testAdsTimeout);
	// ReadWrite
	adsTest.run();

	std::cout << "Hit ENTER to continue\n";
	std::cin.ignore();
}
#pragma warning(default: 4800)
