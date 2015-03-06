
#include <Windows.h>
#include <TcAdsDef.h>
#include <TcAdsAPI.h>
#include <cstdint>

#include <iostream>

#pragma warning(push, 0)
#include <fructose/fructose.h>
#pragma warning(pop)
using namespace fructose;

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

struct TestAds : test_base < TestAds >
{
	std::ostream &out;

	TestAds(std::ostream& outstream)
		: out(outstream)
	{}

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
		unsigned long bytesRead;
		char buffer[4];
		fructose_assert(0 == AdsSyncReadReqEx2(port, &server, group, offset, sizeof(buffer), buffer, &bytesRead));
		fructose_assert(0 == AdsPortCloseEx(port));
	}

	void testAdsReadDeviceInfoReqEx(const std::string&)
	{
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();
		fructose_assert(0 != port);

		AdsVersion version;
		char devName[50];
		fructose_assert(0 == AdsSyncReadDeviceInfoReqEx(port, &server, devName, &version));
		fructose_assert(0 == AdsPortCloseEx(port));

		out << "AdsSyncReadDeviceInfoReqEx() name: " << devName
			<< " Version: " << (int)version.version << '.' << (int)version.revision << '.' << (int)version.build << '\n';
	}

	void testAdsReadStateReqEx(const std::string&)
	{
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();
		fructose_assert(0 != port);


		USHORT adsState;
		USHORT deviceState;
		fructose_assert(0 == AdsSyncReadStateReqEx(port, &server, &adsState, &deviceState));
		fructose_assert(0 == AdsPortCloseEx(port));

		out << "AdsSyncReadStateReqEx() adsState: " << (int)adsState
			<< " device: " << (int)deviceState << '\n';
	}
};

int main(int argc, char* argv[])
{
#if 0
	std::ostream nowhere(0);
	TestAds adsTest(nowhere);
#else
	TestAds adsTest(std::cout);
#endif
	adsTest.add_test("testAdsPortOpenEx", &TestAds::testAdsPortOpenEx);
	adsTest.add_test("testAdsReadReqEx2", &TestAds::testAdsReadReqEx2);
	adsTest.add_test("testAdsReadDeviceInfoReqEx", &TestAds::testAdsReadDeviceInfoReqEx);
	adsTest.add_test("testAdsReadStateReqEx", &TestAds::testAdsReadStateReqEx);
	adsTest.run(argc, argv);

	std::cout << "Hit ENTER to continue\n";
	std::cin.ignore();
}