
#include "AdsVariable.h"
#include "AdsDevice.h"
#include "AdsNotificationOOI.h"
#include "AdsLib.h"

#include <array>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <thread>

#include <fructose/fructose.h>
using namespace fructose;

static const AmsNetId serverNetId {192, 168, 0, 231, 1, 1};

static size_t g_NumNotifications = 0;
static void NotifyCallback(const AmsAddr* pAddr, const AdsNotificationHeader* pNotification, uint32_t hUser)
{
#if 0
    static std::ostream& notificationOutput = std::cout;
#else
    static std::ostream notificationOutput(0);
#endif
    ++g_NumNotifications;
    auto pData = reinterpret_cast<const uint8_t*>(pNotification + 1);
    notificationOutput << std::setfill('0') <<
        "NetId 0x" << pAddr->netId <<
        "hUser 0x" << std::hex << std::setw(4) << hUser <<
        " sample time: " << std::dec << pNotification->nTimeStamp <<
        " sample size: " << std::dec << pNotification->cbSampleSize <<
        " value: 0x" << std::hex << (int)pData[0] << '\n';
}

void print(const AmsAddr& addr, std::ostream& out)
{
    out << "AmsAddr: " << std::dec <<
    (int)addr.netId.b[0] << '.' << (int)addr.netId.b[1] << '.' << (int)addr.netId.b[2] << '.' <<
    (int)addr.netId.b[3] << '.' << (int)addr.netId.b[4] << '.' << (int)addr.netId.b[5] << ':' <<
        addr.port << '\n';
}

long testPortOpen(std::ostream& out)
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

struct TestAds : test_base<TestAds> {
    static const int NUM_TEST_LOOPS = 10;
    std::ostream& out;

    TestAds(std::ostream& outstream)
        : out(outstream)
    {}

    void testAdsPortOpenEx(const std::string&)
    {
        static const size_t NUM_TEST_PORTS = 2;
        std::vector<AdsDevice> routes;

        for (size_t i = 0; i < NUM_TEST_PORTS; ++i) {
            routes.emplace_back("ads-server", serverNetId, AMSPORT_R0_PLC_TC3);
            fructose_loop_assert(i, 0 != routes.back().GetLocalPort());
        }
    }

    void testAdsReadReqEx2(const std::string&)
    {
        AdsDevice route {"ads-server", serverNetId, AMSPORT_R0_PLC_TC3};
        fructose_assert(0 != route.GetLocalPort());

        print(route.m_Addr, out);
        {
            AdsVariable<uint32_t> buffer {route, 0x4020, 0};
            for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
                fructose_loop_assert(i, 0 == buffer);
            }
        }

        // provide out of range port
        /* not possible with OOI */

        // provide nullptr to AmsAddr
        /* not possible with OOI */

        // provide unknown AmsAddr
        /* not possible with OOI */

        // provide nullptr to bytesRead
        /* not possible with OOI */

        // provide nullptr to buffer
        /* not possible with OOI */

        // provide 0 length buffer
        /* not possible with OOI */

        // provide invalid indexGroup
        try {
            AdsVariable<uint32_t> buffer {route, 0, 0};
            fructose_assert(0 == buffer);
            fructose_assert(false);
        } catch (const AdsException& ex) {
            fructose_assert(ADSERR_DEVICE_SRVNOTSUPP == ex.errorCode);
        }

        // provide invalid indexOffset
        try {
            AdsVariable<uint32_t> buffer {route, 0x4025, 0x10000};
            fructose_assert(0 == buffer);
            fructose_assert(false);
        } catch (const AdsException& ex) {
            fructose_assert(ADSERR_DEVICE_SRVNOTSUPP == ex.errorCode);
        }
    }

    void testAdsReadReqEx2LargeBuffer(const std::string&)
    {
        using LargeBuffer = std::array<uint8_t, 8192>;
        AdsDevice route {"ads-server", serverNetId, AMSPORT_R0_PLC_TC3};
        AdsVariable<LargeBuffer> buffer {route, "MAIN.moreBytes"};

        std::cout << buffer.operator LargeBuffer()[0] << '\n';
    }

    void testAdsReadDeviceInfoReqEx(const std::string&)
    {
        static const char NAME[] = "Plc30 App";
        {
            AdsDevice route {"ads-server", serverNetId, AMSPORT_R0_PLC_TC3};
            fructose_assert(0 != route.GetLocalPort());

            DeviceInfo info = route.GetDeviceInfo();
            for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
                fructose_loop_assert(i, 3 == info.version.version);
                fructose_loop_assert(i, 1 == info.version.revision);
                fructose_loop_assert(i, 1711 == info.version.build);
                fructose_loop_assert(i, 0 == strncmp(info.name, NAME, sizeof(NAME)));
            }
        }

        // provide out of range port
        /* not possible with OOI */

        // provide nullptr to AmsAddr
        /* not possible with OOI */

        // provide unknown AmsAddr
        /* not possible with OOI */

        // provide nullptr to devName/version
        /* not possible with OOI */
    }

    void testAdsReadStateReqEx(const std::string&)
    {
        {
            AdsDevice route {"ads-server", serverNetId, AMSPORT_R0_PLC_TC3};
            fructose_assert(0 != route.GetLocalPort());

            const auto state = route.GetState();
            fructose_assert(ADSSTATE_RUN == state.ads);
            fructose_assert(0 == state.device);
        }

        // provide bad server port
        try {
            AdsDevice badAmsAddrRoute {"ads-server", serverNetId, 1000};
            const auto state = badAmsAddrRoute.GetState();
            fructose_assert(0 == state.device);
            fructose_assert(false);
        } catch (const AdsException& ex) {
            fructose_assert(GLOBALERR_TARGET_PORT == ex.errorCode);
        }

        // provide out of range port
        /* not possible with OOI */

        // provide nullptr to AmsAddr
        /* not possible with OOI */

        // provide unknown AmsAddr
        /* not possible with OOI */

        // provide nullptr to adsState/devState
        /* not possible with OOI */
    }

    void testAdsReadWriteReqEx2(const std::string&)
    {
        static const char handleName[] = "MAIN.byByte";
        AdsDevice route {"ads-server", serverNetId, AMSPORT_R0_PLC_TC3};
        fructose_assert(0 != route.GetLocalPort());

        print(route.m_Addr, out);
        {
            uint32_t outBuffer = 0xDEADBEEF;
            AdsVariable<uint32_t> buffer {route, handleName};
            for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
                buffer = outBuffer;
                fructose_loop_assert(i, outBuffer == buffer);
                outBuffer = ~outBuffer;
            }
            buffer = 0x0; /* restore default value */
        }

        // provide out of range port
        /* not possible with OOI */

        // provide nullptr to AmsAddr
        /* not possible with OOI */

        // provide unknown AmsAddr
        /* not possible with OOI */

        // provide nullptr to bytesRead
        /* not possible with OOI */

        // provide nullptr to readBuffer
        /* not possible with OOI */

        // provide 0 length readBuffer
        /* not possible with OOI */

        // provide nullptr to writeBuffer
        /* not possible with OOI */

        // provide 0 length writeBuffer
        /* not possible with OOI */

        // provide invalid symbolName
        try {
            AdsVariable<uint32_t> buffer {route, "xxx"};
            fructose_assert(0 == buffer);
            fructose_assert(false);
        } catch (const AdsException& ex) {
            fructose_assert(ADSERR_DEVICE_SYMBOLNOTFOUND == ex.errorCode);
        }

        // provide invalid indexGroup
        try {
            AdsVariable<uint32_t> buffer {route, 0, 0};
            fructose_assert(0 == buffer);
            fructose_assert(false);
        } catch (const AdsException& ex) {
            fructose_assert(ADSERR_DEVICE_SRVNOTSUPP == ex.errorCode);
        }

        // provide invalid indexOffset
        try {
            AdsVariable<uint32_t> buffer {route, 0x4025, 0x10000};
            fructose_assert(0 == buffer);
            fructose_assert(false);
        } catch (const AdsException& ex) {
            fructose_assert(ADSERR_DEVICE_SRVNOTSUPP == ex.errorCode);
        }
    }

    void testAdsWriteReqEx(const std::string&)
    {
        AdsDevice route {"ads-server", serverNetId, AMSPORT_R0_PLC_TC3};
        fructose_assert(0 != route.GetLocalPort());

        print(route.m_Addr, out);
        {
            uint32_t outBuffer = 0xDEADBEEF;
            AdsVariable<uint32_t> buffer {route, 0x4020, 0};
            for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
                buffer = outBuffer;
                fructose_loop_assert(i, outBuffer == buffer);
                outBuffer = ~outBuffer;
            }
            buffer = 0x0; /* restore default value */
        }

        // provide out of range port
        /* not possible with OOI */

        // provide nullptr to AmsAddr
        /* not possible with OOI */

        // provide unknown AmsAddr
        /* not possible with OOI */

        // provide nullptr to writeBuffer
        /* not possible with OOI */

        // provide 0 length writeBuffer
        /* not possible with OOI */

        // provide invalid symbolName
        try {
            AdsVariable<uint32_t> buffer {route, "xxx"};
            fructose_assert(0 == buffer);
            fructose_assert(false);
        } catch (const AdsException& ex) {
            fructose_assert(ADSERR_DEVICE_SYMBOLNOTFOUND == ex.errorCode);
        }

        // provide invalid indexGroup
        try {
            AdsVariable<uint32_t> buffer {route, 0, 0};
            fructose_assert(0 == buffer);
            fructose_assert(false);
        } catch (const AdsException& ex) {
            fructose_assert(ADSERR_DEVICE_SRVNOTSUPP == ex.errorCode);
        }
    }

    void testAdsWriteControlReqEx(const std::string&)
    {
        AdsDevice route {"ads-server", serverNetId, AMSPORT_R0_PLC_TC3};
        fructose_assert(0 != route.GetLocalPort());

        for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
            route.SetState(ADSSTATE_STOP, ADSSTATE_INVALID);
            auto state = route.GetState();
            fructose_loop_assert(i, ADSSTATE_STOP == state.ads);
            fructose_loop_assert(i, ADSSTATE_INVALID == state.device);
            route.SetState(ADSSTATE_RUN, ADSSTATE_INVALID);
            state = route.GetState();
            fructose_loop_assert(i, ADSSTATE_RUN == state.ads);
            fructose_loop_assert(i, ADSSTATE_INVALID == state.device);
        }

        // provide out of range port
        /* not possible with OOI */

        // provide nullptr to AmsAddr
        /* not possible with OOI */

        // provide unknown AmsAddr
        /* not possible with OOI */

        // provide invalid adsState
        try {
            route.SetState(ADSSTATE_INVALID, ADSSTATE_INVALID);
            fructose_assert(false);
        } catch (const AdsException& ex) {
            fructose_assert(ADSERR_DEVICE_SRVNOTSUPP == ex.errorCode);
        }
        try {
            route.SetState(ADSSTATE_MAXSTATES, ADSSTATE_INVALID);
            fructose_assert(false);
        } catch (const AdsException& ex) {
            fructose_assert(ADSERR_DEVICE_SRVNOTSUPP == ex.errorCode);
        }

        // provide invalid devState
        // TODO find correct parameters for this test

        // provide trash buffer
        /* not possible with OOI */
    }

    void testAdsNotification(const std::string&)
    {
        static const uint32_t NOTIFY_CYCLE_100NS = 1000000;
        static const size_t MAX_NOTIFICATIONS_PER_PORT = 1024;
        static const size_t LEAKED_NOTIFICATIONS = MAX_NOTIFICATIONS_PER_PORT / 2;
        AdsNotificationAttrib attrib = {1, ADSTRANS_SERVERCYCLE, 0, {NOTIFY_CYCLE_100NS}};

        AdsDevice route {"ads-server", serverNetId, AMSPORT_R0_PLC_TC3};
        fructose_assert(0 != route.GetLocalPort());

        // provide out of range port
        /* not possible with OOI */

        // provide nullptr to AmsAddr
        /* not possible with OOI */

        // provide unknown AmsAddr
        /* not possible with OOI */

        // provide invalid indexGroup
        try {
            AdsNotification buffer {route, 0, 0, attrib, &NotifyCallback, 1};
            fructose_assert(false);
        } catch (const AdsException& ex) {
            fructose_assert(ADSERR_DEVICE_SRVNOTSUPP == ex.errorCode);
        }

        // provide invalid indexOffset
        try {
            AdsNotification buffer {route, 0x4025, 0x10000, attrib, &NotifyCallback, 2};
            fructose_assert(false);
        } catch (const AdsException& ex) {
            fructose_assert(ADSERR_DEVICE_SRVNOTSUPP == ex.errorCode);
        }

        // provide nullptr to attrib/callback/hNotification
        try {
            AdsNotification buffer {route, 0x4025, 0x10000, attrib, (PAdsNotificationFuncEx)nullptr, 3};
            fructose_assert(false);
        } catch (const AdsException& ex) {
            fructose_assert(ADSERR_CLIENT_INVALIDPARM == ex.errorCode);
        }

        // delete nonexisting notification
        /* not possible with OOI */

        // normal test
        {
            g_NumNotifications = 0;
            std::vector<AdsNotification> notifications;
            for (size_t i = 0; i < MAX_NOTIFICATIONS_PER_PORT; ++i) {
                notifications.emplace_back(route, 0x4020, 4, attrib, &NotifyCallback, i);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            for (size_t i = 0; i < MAX_NOTIFICATIONS_PER_PORT - LEAKED_NOTIFICATIONS; ++i) {
                notifications.pop_back();
            }
            fructose_assert(g_NumNotifications > 0);
        }
        {
            static const char handleName[] = "MAIN.byByte";
            g_NumNotifications = 0;
            std::vector<AdsNotification> notifications;
            for (size_t i = 0; i < MAX_NOTIFICATIONS_PER_PORT; ++i) {
                notifications.emplace_back(route, handleName, attrib, &NotifyCallback, i);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            for (size_t i = 0; i < MAX_NOTIFICATIONS_PER_PORT - LEAKED_NOTIFICATIONS; ++i) {
                notifications.pop_back();
            }
            fructose_assert(g_NumNotifications > 0);
        }
    }

    void testAdsTimeout(const std::string&)
    {
        AdsDevice route {"ads-server", serverNetId, AMSPORT_R0_PLC_TC3};
        fructose_assert(0 != route.GetLocalPort());

        fructose_assert(5000 == route.GetTimeout());
        route.SetTimeout(1000);
        fructose_assert(1000 == route.GetTimeout());

        // provide out of range port
        /* not possible with OOI */

        // provide nullptr to timeout
        /* not possible with OOI */
    }
};

struct TestAdsPerformance : test_base<TestAdsPerformance> {
    std::ostream& out;
    bool runEndurance;

    TestAdsPerformance(std::ostream& outstream)
        : out(outstream),
        runEndurance(false)
    {}

    void testLargeFrames(const std::string&)
    {
        // TODO testLargeFrames
        fructose_assert(false);
    }

    void testManyNotifications(const std::string& testname)
    {
        std::thread threads[8];
        g_NumNotifications = 0;
        const auto start = std::chrono::high_resolution_clock::now();
        for (auto& t : threads) {
            t = std::thread(&TestAdsPerformance::Notifications, this, 1024);
        }
        for (auto& t : threads) {
            t.join();
        }
        const auto end = std::chrono::high_resolution_clock::now();
        const auto tmms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        out << testname << ' ' << g_NumNotifications / tmms << " notifications/ms (" << g_NumNotifications << '/' <<
            tmms << ")\n";
    }

    void testParallelReadAndWrite(const std::string& testname)
    {
        std::thread threads[96];
        const auto start = std::chrono::high_resolution_clock::now();
        for (auto& t : threads) {
            t = std::thread(&TestAdsPerformance::Read, this, 1024);
        }
        for (auto& t : threads) {
            t.join();
        }
        const auto end = std::chrono::high_resolution_clock::now();
        const auto tmms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        out << testname << " took " << tmms << "ms\n";
    }

    void testEndurance(const std::string& testname)
    {
        static const size_t numNotifications = 1024;
        const long port = AdsPortOpenEx();
        fructose_assert(0 != port);

        const AdsDevice route("ads-server", serverNetId, AMSPORT_R0_PLC_TC3);
        AdsNotificationAttrib attrib = { 1, ADSTRANS_SERVERCYCLE, 0, {1000000} };
        std::vector<AdsNotification> notifications;

        runEndurance = true;
        g_NumNotifications = 0;
        std::thread threads[96];
        for (auto& t : threads) {
            t = std::thread(&TestAdsPerformance::Read, this, 1024);
        }

        const auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < numNotifications; ++i) {
            notifications.emplace_back(route, 0x4020, 4, attrib, &NotifyCallback, i);
        }

        std::cout << "Hit ENTER to stop endurance test\n";
        std::cin.ignore();
        runEndurance = false;

        for (auto& t : threads) {
            t.join();
        }
        const auto end = std::chrono::high_resolution_clock::now();
        const auto tmms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        out << testname << ' ' << 1000ULL * g_NumNotifications / tmms << " notifications/s (" << g_NumNotifications <<
            '/' << tmms << ")\n";
    }

private:
    void Notifications(size_t numNotifications)
    {
        const AdsDevice route("ads-server", serverNetId, AMSPORT_R0_PLC_TC3);
        AdsNotificationAttrib attrib = {1, ADSTRANS_SERVERCYCLE, 0, {1000000}};
        std::vector<AdsNotification> notifications;

        for (size_t i = 0; i < numNotifications; ++i) {
            notifications.emplace_back(route, 0x4020, 4, attrib, &NotifyCallback, i);
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    void Read(const size_t numLoops)
    {
        const AdsDevice route("ads-server", serverNetId, AMSPORT_R0_PLC_TC3);
        fructose_assert(0 != route.GetLocalPort());

        AdsVariable<uint32_t> buffer {route, 0x4020, 0};
        do {
            for (size_t i = 0; i < numLoops; ++i) {
                fructose_loop_assert(i, 0 == buffer);
            }
        } while (runEndurance);
    }
};

const AdsDevice staticRoute("ads-server", serverNetId, AMSPORT_R0_PLC_TC3);
AdsVariable<uint32_t> staticBuffer {staticRoute, "MAIN.byByte"};
static const uint32_t staticRead = staticBuffer;

int main()
{
    int failedTests = 0;
#if 0
    std::ostream nowhere(0);
    std::ostream& errorstream = nowhere;
#else
    std::ostream& errorstream = std::cout;
#endif
    errorstream << "Testing global static AdsVariable: " << staticBuffer << '\n';
    TestAds adsTest(errorstream);
    adsTest.add_test("testAdsPortOpenEx", &TestAds::testAdsPortOpenEx);
    adsTest.add_test("testAdsReadReqEx2", &TestAds::testAdsReadReqEx2);
    adsTest.add_test("testAdsReadReqEx2LargeBuffer", &TestAds::testAdsReadReqEx2LargeBuffer);
    adsTest.add_test("testAdsReadDeviceInfoReqEx", &TestAds::testAdsReadDeviceInfoReqEx);
    adsTest.add_test("testAdsReadStateReqEx", &TestAds::testAdsReadStateReqEx);
    adsTest.add_test("testAdsReadWriteReqEx2", &TestAds::testAdsReadWriteReqEx2);
    adsTest.add_test("testAdsWriteReqEx", &TestAds::testAdsWriteReqEx);
    adsTest.add_test("testAdsWriteControlReqEx", &TestAds::testAdsWriteControlReqEx);
    adsTest.add_test("testAdsNotification", &TestAds::testAdsNotification);
    adsTest.add_test("testAdsTimeout", &TestAds::testAdsTimeout);
    failedTests += adsTest.run();

    TestAdsPerformance performance(errorstream);
    performance.add_test("testManyNotifications", &TestAdsPerformance::testManyNotifications);
    performance.add_test("testParallelReadAndWrite", &TestAdsPerformance::testParallelReadAndWrite);
    performance.add_test("testEndurance", &TestAdsPerformance::testEndurance);
    failedTests += performance.run();

    std::cout << "Hit ENTER to continue\n";
    std::cin.ignore();
    return failedTests;
}
