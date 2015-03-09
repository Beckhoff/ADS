
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
	adsTest.run();

	std::cout << "Hit ENTER to continue\n";
	std::cin.ignore();
}
#pragma warning(default: 4800)