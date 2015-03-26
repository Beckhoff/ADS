
#include <Windows.h>
#include <TcAdsDef.h>
#include <TcAdsAPI.h>
#include <cstdint>
#include <chrono>
#include <thread>

#include <iostream>
#include <iomanip>

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
	static const int NUM_TEST_LOOPS = 10;
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

		unsigned long bytesRead;
		uint32_t buffer;

		for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
			fructose_loop_assert(i, 0 == AdsSyncReadReqEx2(port, &server, 0x4020, 0, sizeof(buffer), &buffer, &bytesRead));
			fructose_loop_assert(i, sizeof(buffer) == bytesRead);
			fructose_loop_assert(i, 0 == buffer);
		}
		fructose_assert(0 == AdsPortCloseEx(port));
	}

	void testAdsReadDeviceInfoReqEx(const std::string&)
	{
		static const char NAME[] = "Plc30 App";
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();
		fructose_assert(0 != port);

		for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
			AdsVersion version{ 0, 0, 0 };
			char devName[16 + 1]{};
			fructose_loop_assert(i, 0 == AdsSyncReadDeviceInfoReqEx(port, &server, devName, &version));
			fructose_loop_assert(i, 3 == version.version);
			fructose_loop_assert(i, 1 == version.revision);
			fructose_loop_assert(i, 1101 == version.build);
			fructose_loop_assert(i, 0 == strncmp(devName, NAME, sizeof(NAME)));
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

	void testAdsReadWriteReqEx2(const std::string&)
	{
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();
		fructose_assert(0 != port);


		uint32_t hHandle;
		unsigned long bytesRead;
		fructose_assert(0 == AdsSyncReadWriteReqEx2(port, &server, 0xF003, 0, sizeof(hHandle), &hHandle, 11, "MAIN.byByte", &bytesRead));
		fructose_assert(sizeof(hHandle) == bytesRead);

		uint32_t outBuffer = 0xDEADBEEF;
		uint32_t buffer;
		for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
			fructose_loop_assert(i, 0 == AdsSyncWriteReqEx(port, &server, 0xF005, hHandle, sizeof(outBuffer), &outBuffer));
			fructose_loop_assert(i, 0 == AdsSyncReadReqEx2(port, &server, 0xF005, hHandle, sizeof(buffer), &buffer, &bytesRead));
			fructose_loop_assert(i, sizeof(buffer) == bytesRead);
			fructose_loop_assert(i, outBuffer == buffer);
			outBuffer = ~outBuffer;
		}
		fructose_assert(0 == AdsPortCloseEx(port));
	}

	void testAdsWriteReqEx(const std::string&)
	{
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();
		fructose_assert(0 != port);

		print(server, out);

		unsigned long bytesRead;
		uint32_t buffer;
		uint32_t outBuffer = 0xDEADBEEF;

		for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
			fructose_loop_assert(i, 0 == AdsSyncWriteReqEx(port, &server, 0x4020, 0, sizeof(outBuffer), &outBuffer));
			fructose_loop_assert(i, 0 == AdsSyncReadReqEx2(port, &server, 0x4020, 0, sizeof(buffer), &buffer, &bytesRead));
			fructose_loop_assert(i, sizeof(buffer) == bytesRead);
			fructose_loop_assert(i, outBuffer == buffer);
			outBuffer = ~outBuffer;
		}
		uint32_t defaultValue = 0;
		fructose_assert(0 == AdsSyncWriteReqEx(port, &server, 0x4020, 0, sizeof(defaultValue), &defaultValue));
		fructose_assert(0 == AdsPortCloseEx(port));
	}

	void testAdsWriteControlReqEx(const std::string&)
	{
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();
		fructose_assert(0 != port);

		uint16_t adsState;
		uint16_t devState;

		for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
			fructose_assert(0 == AdsSyncWriteControlReqEx(port, &server, ADSSTATE_STOP, 0, 0, nullptr));
			fructose_loop_assert(i, 0 == AdsSyncReadStateReqEx(port, &server, &adsState, &devState));
			fructose_loop_assert(i, ADSSTATE_STOP == adsState);
			fructose_loop_assert(i, 0 == devState);
			fructose_loop_assert(i, 0 == AdsSyncWriteControlReqEx(port, &server, ADSSTATE_RUN, 0, 0, nullptr));
			fructose_loop_assert(i, 0 == AdsSyncReadStateReqEx(port, &server, &adsState, &devState));
			fructose_loop_assert(i, ADSSTATE_RUN == adsState);
			fructose_loop_assert(i, 0 == devState);
		}
		fructose_assert(0 == AdsPortCloseEx(port));
	}

	static void __stdcall NotifyCallback(AmsAddr* pAddr, AdsNotificationHeader* pNotification, unsigned long hUser)
	{
#if 0
		std::cout << std::setfill('0')
			<< "hUser 0x" << std::hex << std::setw(4) << hUser
			<< " sample time: " << std::dec << pNotification->nTimeStamp
			<< " sample size: " << std::dec << pNotification->cbSampleSize
			<< " value: 0x" << std::hex << (int)pNotification->data[0] << '\n';
#endif
	}

	void testAdsNotification(const std::string&)
	{
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();

		fructose_assert(0 != port);

		static const size_t MAX_NOTIFICATIONS_PER_PORT = 1024;
		static const size_t LEAKED_NOTIFICATIONS = MAX_NOTIFICATIONS_PER_PORT / 2;
		unsigned long notification[MAX_NOTIFICATIONS_PER_PORT];
		AdsNotificationAttrib attrib = { 1, ADSTRANS_SERVERCYCLE, 0, 1000000 };
		unsigned long hUser;

		for (hUser = 0; hUser < MAX_NOTIFICATIONS_PER_PORT; ++hUser) {
			fructose_loop_assert(hUser, 0 == AdsSyncAddDeviceNotificationReqEx(port, &server, 0x4020, 4, &attrib, &NotifyCallback, hUser, &notification[hUser]));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		for (hUser = 0; hUser < MAX_NOTIFICATIONS_PER_PORT - LEAKED_NOTIFICATIONS; ++hUser) {
			fructose_loop_assert(hUser, 0 == AdsSyncDelDeviceNotificationReqEx(port, &server, notification[hUser]));
		}
		fructose_assert(0 == AdsPortCloseEx(port));
	}

	void testAdsTimeout(const std::string&)
	{
		AmsAddr server{ { 192, 168, 0, 231, 1, 1 }, AMSPORT_R0_PLC_TC3 };
		const long port = AdsPortOpenEx();
		long timeout;

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
	TestAds adsTest(errorstream);
	adsTest.add_test("testAdsPortOpenEx", &TestAds::testAdsPortOpenEx);
	adsTest.add_test("testAdsReadReqEx2", &TestAds::testAdsReadReqEx2);
	adsTest.add_test("testAdsReadDeviceInfoReqEx", &TestAds::testAdsReadDeviceInfoReqEx);
	adsTest.add_test("testAdsReadStateReqEx", &TestAds::testAdsReadStateReqEx);
	adsTest.add_test("testAdsReadWriteReqEx2", &TestAds::testAdsReadWriteReqEx2);
	adsTest.add_test("testAdsWriteReqEx", &TestAds::testAdsWriteReqEx);
	adsTest.add_test("testAdsWriteControlReqEx", &TestAds::testAdsWriteControlReqEx);
	adsTest.add_test("testAdsNotification", &TestAds::testAdsNotification);
	adsTest.add_test("testAdsTimeout", &TestAds::testAdsTimeout);
	adsTest.run();

	std::cout << "Hit ENTER to continue\n";
	std::cin.ignore();
}