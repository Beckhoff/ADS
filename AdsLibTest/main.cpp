
#include <AdsLib.h>

#include "AmsRouter.h"

#include <iostream>
#include <iomanip>

#include <fructose/fructose.h>
using namespace fructose;

static const AmsNetId serverNetId {192, 168, 0, 231, 1, 1};
static const AmsAddr server {serverNetId, AMSPORT_R0_PLC_TC3};
static const AmsAddr serverBadPort {serverNetId, 1000};

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

struct TestAmsAddr : test_base<TestAmsAddr> {
    std::ostream& out;

    TestAmsAddr(std::ostream& outstream)
        : out(outstream)
    {}

    void testAmsAddrCompare(const std::string&)
    {
        static const AmsAddr testee {AmsNetId {192, 168, 0, 231, 1, 1}, 1000};
        static const AmsAddr lower_last {AmsNetId {192, 168, 0, 231, 1, 0}, 1000};
        static const AmsAddr lower_middle {AmsNetId {192, 168, 0, 1, 1, 1}, 1000};
        static const AmsAddr lower_port {AmsNetId {192, 168, 0, 231, 1, 1}, 999};

        fructose_assert(lower_last < testee);
        fructose_assert(lower_middle < testee);
        fructose_assert(lower_port < testee);
        fructose_assert(!(testee < lower_last));
        fructose_assert(!(testee < lower_middle));
        fructose_assert(!(testee < lower_port));
        fructose_assert(!(testee < testee));
    }
};

struct TestAmsRouter : test_base<TestAmsRouter> {
    std::ostream& out;

    TestAmsRouter(std::ostream& outstream)
        : out(outstream)
    {}

    void testAmsRouterAddRoute(const std::string&)
    {
        static const AmsNetId netId_1 { 192, 168, 0, 231, 1, 1 };
        static const AmsNetId netId_2 { 127, 0, 0, 1, 2, 1 };
        static const IpV4 ip_local("127.0.0.1");
        static const IpV4 ip_remote("ads-server");
        AmsRouter testee;

        // test new Ams with new Ip
        fructose_assert(0 == testee.AddRoute(netId_1, ip_local));
        fructose_assert(!!testee.GetConnection(netId_1));
        fructose_assert(ip_local == testee.GetConnection(netId_1)->destIp);

        // existent Ams with new Ip -> close old connection manually, now
        fructose_assert(ROUTERERR_PORTALREADYINUSE == testee.AddRoute(netId_1, ip_remote));
        fructose_assert(!!testee.GetConnection(netId_1));
        fructose_assert(ip_local == testee.GetConnection(netId_1)->destIp);
        testee.DelRoute(netId_1);
        fructose_assert(0 == testee.AddRoute(netId_1, ip_remote));
        fructose_assert(!!testee.GetConnection(netId_1));
        fructose_assert(ip_remote == testee.GetConnection(netId_1)->destIp);

        // new Ams with existent Ip
        fructose_assert(0 == testee.AddRoute(netId_2, ip_remote));
        fructose_assert(!!testee.GetConnection(netId_2));
        fructose_assert(ip_remote == testee.GetConnection(netId_2)->destIp);

        // already exists
        fructose_assert(0 == testee.AddRoute(netId_1, ip_remote));
        fructose_assert(!!testee.GetConnection(netId_1));
        fructose_assert(ip_remote == testee.GetConnection(netId_1)->destIp);
    }

    void testAmsRouterDelRoute(const std::string&)
    {
        static const AmsNetId netId_1 { 192, 168, 0, 231, 1, 1 };
        static const AmsNetId netId_2 { 127, 0, 0, 1, 2, 1 };
        static const IpV4 ip_local("127.0.0.1");
        static const IpV4 ip_remote("ads-server");
        AmsRouter testee;

        // add + remove -> null
        fructose_assert(0 == testee.AddRoute(netId_1, ip_remote));
        fructose_assert(!!testee.GetConnection(netId_1));
        fructose_assert(ip_remote == testee.GetConnection(netId_1)->destIp);
        testee.DelRoute(netId_1);
        fructose_assert(!testee.GetConnection(netId_1));

        // add_1, add_2, remove_1 -> null_1 valid_2
        fructose_assert(0 == testee.AddRoute(netId_1, ip_remote));
        fructose_assert(!!testee.GetConnection(netId_1));
        fructose_assert(ip_remote == testee.GetConnection(netId_1)->destIp);
        fructose_assert(0 == testee.AddRoute(netId_2, ip_local));
        fructose_assert(!!testee.GetConnection(netId_2));
        fructose_assert(ip_local == testee.GetConnection(netId_2)->destIp);
        testee.DelRoute(netId_1);
        fructose_assert(!testee.GetConnection(netId_1));
        fructose_assert(!!testee.GetConnection(netId_2));
    }

    void testAmsRouterSetLocalAddress(const std::string&)
    {
        const AmsNetId newNetId {1, 2, 3, 4, 5, 6};
        AmsRouter testee;
        AmsAddr changed;
        AmsAddr empty;

        const auto port = testee.OpenPort();
        fructose_assert(0 == testee.GetLocalAddress(port, &empty));
        fructose_assert(!empty.netId);

        testee.SetLocalAddress(newNetId);
        fructose_assert(0 == testee.GetLocalAddress(port, &changed));
        fructose_assert(0 == memcmp(&newNetId, &changed.netId, sizeof(newNetId)));
        fructose_assert(port == changed.port);
    }

    void testConcurrentRoutes(const std::string&)
    {
        std::thread threads[256];
        AmsRouter testee;
        uint8_t i = 0;
        for (auto& t : threads) {
            t = std::thread(&TestAmsRouter::Run, this, std::ref(testee), ++i);
        }

        for (auto& t : threads) {
            t.join();
        }
    }
private:
    void Run(AmsRouter& testee, uint8_t id)
    {
        static const IpV4 ip("ads-server");
        for (uint8_t i = 0; i < 255; ++i) {
            AmsNetId netId {192, 168, 0, i, 0, id};
            fructose_assert_eq(0, testee.AddRoute(netId, ip));
            fructose_assert(!!testee.GetConnection(netId));
            fructose_assert(ip == testee.GetConnection(netId)->destIp);
        }

        for (uint8_t i = 0; i < 255; ++i) {
            AmsNetId netId {192, 168, 0, i, 0, id};
            testee.DelRoute(netId);
            fructose_assert(!testee.GetConnection(netId));
        }
        std::cout << std::dec << (int)id << '\n';
    }
};

struct TestIpV4 : test_base<TestIpV4> {
    std::ostream& out;

    TestIpV4(std::ostream& outstream)
        : out(outstream)
    {}

    void testComparsion(const std::string&)
    {
        static const IpV4 testee {"192.168.0.1"};
        static const IpV4 localhost {"localhost"};
        static const IpV4 lower {"192.167.0.1"};
        static const IpV4 higher {"193.0.0.0"};

        fructose_assert_eq(0xC0A80001, testee.value);
        fructose_assert_eq(0x7F000001U, localhost.value);
        fructose_assert_eq(0xC0A70001, lower.value);
        fructose_assert_eq(0xC1000000, higher.value);
        fructose_assert_exception(IpV4 {"192.168.0."}, std::runtime_error);     // too short
        fructose_assert_exception(IpV4 {"0.0.0.257"}, std::runtime_error);      // too high
        fructose_assert_exception(IpV4 {"-1.0.0.254"}, std::runtime_error);     // too low
        fructose_assert_exception(IpV4 {"192.d.0.254"}, std::runtime_error);    // invalid
        fructose_assert(lower < testee);
        fructose_assert(testee < higher);
    }
};

struct TestRingBuffer : test_base<TestRingBuffer> {
    static const int NUM_TEST_LOOPS = 1024;
    std::ostream& out;

    TestRingBuffer(std::ostream& outstream)
        : out(outstream)
    {}

    void testBytesFree(const std::string&)
    {
        RingBuffer testee { 1 };
        const auto data = testee.write;
        fructose_assert(0 == testee.BytesAvailable());
        fructose_assert(1 == testee.BytesFree());
        fructose_assert(testee.write == testee.read);

        *testee.write = 0xA5;
        testee.Write(1);
        fructose_assert(1 == testee.BytesAvailable());
        fructose_assert(0 == testee.BytesFree());
        fructose_assert(0xA5 == *testee.read);
        fructose_assert(data + 1 == testee.write);

        testee.ReadFromLittleEndian<uint8_t>();
        fructose_assert(0 == testee.BytesAvailable());
        fructose_assert(1 == testee.BytesFree());
        fructose_assert(testee.write == testee.read);

        *testee.write = 0x5A;
        testee.Write(1);
        fructose_assert(1 == testee.BytesAvailable());
        fructose_assert(0 == testee.BytesFree());
        fructose_assert(0x5A == *testee.read);
    }

    void testWriteChunk(const std::string&)
    {
        RingBuffer testee { 1 };

        for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
            fructose_assert(1 == testee.WriteChunk());
            testee.Write(1);
            fructose_assert(0 == testee.WriteChunk());
            testee.ReadFromLittleEndian<uint8_t>();
        }
    }
};

struct TestAds : test_base<TestAds> {
    static const int NUM_TEST_LOOPS = 10;
    std::ostream& out;

    TestAds(std::ostream& outstream)
        : out(outstream)
    {
        AdsAddRoute(serverNetId, "ads-server");
    }
#ifdef WIN32
    ~TestAds()
    {
        // WORKAROUND: On Win7-64 AdsConnection::~AdsConnection() is triggered by the destruction
        //             of the static AdsRouter object and hangs in receive.join()
        AdsDelRoute(serverNetId);
    }
#endif

    void testAdsPortOpenEx(const std::string&)
    {
        static const size_t NUM_TEST_PORTS = Router::NUM_PORTS_MAX;
        long port[NUM_TEST_PORTS];

        for (size_t i = 0; i < NUM_TEST_PORTS; ++i) {
            port[i] = testPortOpen(out);
            fructose_loop_assert(i, 0 != port[i]);
        }
        // there should be no more ports available at ADS router
        fructose_assert(0 == testPortOpen(out));

        for (size_t i = 0; i < NUM_TEST_PORTS; ++i) {
            if (port[i]) {
                fructose_loop_assert(i, !AdsPortCloseEx(port[i]));
            }
        }

        // close an already closed port()
        fructose_assert(ADSERR_CLIENT_PORTNOTOPEN == AdsPortCloseEx(port[0]));
    }

    void testAdsReadReqEx2(const std::string&)
    {
        const long port = AdsPortOpenEx();
        fructose_assert(0 != port);

        print(server, out);

        uint32_t bytesRead;
        uint32_t buffer;
        for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
            fructose_loop_assert(i, 0 == AdsSyncReadReqEx2(port,
                                                           &server,
                                                           0x4020,
                                                           0,
                                                           sizeof(buffer),
                                                           &buffer,
                                                           &bytesRead));
            fructose_loop_assert(i, sizeof(buffer) == bytesRead);
            fructose_loop_assert(i, 0 == buffer);
        }

        // provide out of range port
        bytesRead = 0xDEADBEEF;
        fructose_assert(ADSERR_CLIENT_PORTNOTOPEN ==
                        AdsSyncReadReqEx2(0, &server, 0x4020, 0, sizeof(buffer), &buffer, &bytesRead));
        fructose_assert(0xDEADBEEF == bytesRead); // BUG? TcAdsDll doesn't reset bytesRead!

        // provide nullptr to AmsAddr
        bytesRead = 0xDEADBEEF;
        fructose_assert(ADSERR_CLIENT_NOAMSADDR ==
                        AdsSyncReadReqEx2(port, nullptr, 0x4020, 0, sizeof(buffer), &buffer, &bytesRead));
        fructose_assert(0xDEADBEEF == bytesRead); // BUG? TcAdsDll doesn't reset bytesRead!

        // provide unknown AmsAddr
        bytesRead = 0xDEADBEEF;
        AmsAddr unknown { { 1, 2, 3, 4, 5, 6 }, AMSPORT_R0_PLC_TC3 };
        fructose_assert(GLOBALERR_MISSING_ROUTE ==
                        AdsSyncReadReqEx2(port, &unknown, 0x4020, 0, sizeof(buffer), &buffer, &bytesRead));
        fructose_assert(0 == bytesRead);

        // provide nullptr to bytesRead
        buffer = 0xDEADBEEF;
        fructose_assert(0 == AdsSyncReadReqEx2(port, &server, 0x4020, 0, sizeof(buffer), &buffer, nullptr));
        fructose_assert(0 == buffer);

        // provide nullptr to buffer
        fructose_assert(ADSERR_CLIENT_INVALIDPARM ==
                        AdsSyncReadReqEx2(port, &server, 0x4020, 0, sizeof(buffer), nullptr, nullptr));
        fructose_assert(ADSERR_CLIENT_INVALIDPARM ==
                        AdsSyncReadReqEx2(port, &server, 0x4020, 0, sizeof(buffer), nullptr, &bytesRead));

        // provide 0 length buffer
        bytesRead = 0xDEADBEEF;
        fructose_assert(0 == AdsSyncReadReqEx2(port, &server, 0x4020, 0, 0, &buffer, &bytesRead));
        fructose_assert(0 == bytesRead);
        //TODO is this a bug? Shouldn't the request fail with ADSERR_DEVICE_INVALIDSIZE?

        // provide invalid indexGroup
        bytesRead = 0xDEADBEEF;
        fructose_assert(ADSERR_DEVICE_SRVNOTSUPP ==
                        AdsSyncReadReqEx2(port, &server, 0, 0, sizeof(buffer), &buffer, &bytesRead));
        fructose_assert(0 == bytesRead);

        // provide invalid indexOffset
        bytesRead = 0xDEADBEEF;
        fructose_assert(ADSERR_DEVICE_SRVNOTSUPP ==
                        AdsSyncReadReqEx2(port, &server, 0x4025, 0x10000, sizeof(buffer), &buffer, &bytesRead));
        fructose_assert(0 == bytesRead);
        fructose_assert(0 == AdsPortCloseEx(port));
    }

    void testAdsReadDeviceInfoReqEx(const std::string&)
    {
        static const char NAME[] = "Plc30 App";
        const long port = AdsPortOpenEx();
        fructose_assert(0 != port);

        for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
            AdsVersion version { 0, 0, 0 };
            char devName[16 + 1] {};
            fructose_loop_assert(i, 0 == AdsSyncReadDeviceInfoReqEx(port, &server, devName, &version));
            fructose_loop_assert(i, 3 == version.version);
            fructose_loop_assert(i, 1 == version.revision);
            fructose_loop_assert(i, 1202 == version.build);
            fructose_loop_assert(i, 0 == strncmp(devName, NAME, sizeof(NAME)));
        }

        // provide out of range port
        AdsVersion version { 0, 0, 0 };
        char devName[16 + 1] {};
        fructose_assert(ADSERR_CLIENT_PORTNOTOPEN == AdsSyncReadDeviceInfoReqEx(0, &server, devName, &version));

        // provide nullptr to AmsAddr
        fructose_assert(ADSERR_CLIENT_NOAMSADDR == AdsSyncReadDeviceInfoReqEx(port, nullptr, devName, &version));

        // provide unknown AmsAddr
        AmsAddr unknown { { 1, 2, 3, 4, 5, 6 }, AMSPORT_R0_PLC_TC3 };
        fructose_assert(GLOBALERR_MISSING_ROUTE == AdsSyncReadDeviceInfoReqEx(port, &unknown, devName, &version));

        // provide nullptr to devName/version
        fructose_assert(ADSERR_CLIENT_INVALIDPARM == AdsSyncReadDeviceInfoReqEx(port, &server, nullptr, &version));
        fructose_assert(ADSERR_CLIENT_INVALIDPARM == AdsSyncReadDeviceInfoReqEx(port, &server, devName, nullptr));
        fructose_assert(0 == AdsPortCloseEx(port));
    }

    void testAdsReadStateReqEx(const std::string&)
    {
        const long port = AdsPortOpenEx();
        fructose_assert(0 != port);

        uint16_t adsState;
        uint16_t devState;
        fructose_assert(0 == AdsSyncReadStateReqEx(port, &server, &adsState, &devState));
        fructose_assert(ADSSTATE_RUN == adsState);
        fructose_assert(0 == devState);

        // provide bad server port
        fructose_assert(GLOBALERR_TARGET_PORT == AdsSyncReadStateReqEx(port, &serverBadPort, &adsState, &devState));

        // provide out of range port
        fructose_assert(ADSERR_CLIENT_PORTNOTOPEN == AdsSyncReadStateReqEx(0, &server, &adsState, &devState));

        // provide nullptr to AmsAddr
        fructose_assert(ADSERR_CLIENT_NOAMSADDR == AdsSyncReadStateReqEx(port, nullptr, &adsState, &devState));

        // provide unknown AmsAddr
        AmsAddr unknown { { 1, 2, 3, 4, 5, 6 }, AMSPORT_R0_PLC_TC3 };
        fructose_assert(GLOBALERR_MISSING_ROUTE == AdsSyncReadStateReqEx(port, &unknown, &adsState, &devState));

        // provide nullptr to adsState/devState
        fructose_assert(ADSERR_CLIENT_INVALIDPARM == AdsSyncReadStateReqEx(port, &server, nullptr, &devState));
        fructose_assert(ADSERR_CLIENT_INVALIDPARM == AdsSyncReadStateReqEx(port, &server, &adsState, nullptr));
        fructose_assert(0 == AdsPortCloseEx(port));
    }

    void testAdsReadWriteReqEx2(const std::string&)
    {
        char handleName[] = "MAIN.byByte";
        const long port = AdsPortOpenEx();
        fructose_assert(0 != port);

        uint32_t hHandle;
        uint32_t bytesRead;
        fructose_assert(0 ==
                        AdsSyncReadWriteReqEx2(port, &server, 0xF003, 0, sizeof(hHandle), &hHandle, sizeof(handleName),
                                               handleName,
                                               &bytesRead));
        fructose_assert(sizeof(hHandle) == bytesRead);
        hHandle = qFromLittleEndian<uint32_t>((uint8_t*)&hHandle);

        uint32_t buffer;
        uint32_t outBuffer = 0xDEADBEEF;
        for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
            fructose_loop_assert(i, 0 == AdsSyncWriteReqEx(port,
                                                           &server,
                                                           0xF005,
                                                           hHandle,
                                                           sizeof(outBuffer),
                                                           &outBuffer));
            fructose_loop_assert(i,
                                 0 ==
                                 AdsSyncReadReqEx2(port, &server, 0xF005, hHandle, sizeof(buffer), &buffer,
                                                   &bytesRead));
            fructose_loop_assert(i, sizeof(buffer) == bytesRead);
            fructose_loop_assert(i, outBuffer == buffer);
            outBuffer = ~outBuffer;
        }
        hHandle = qToLittleEndian<uint32_t>(hHandle);
        fructose_assert(0 == AdsSyncWriteReqEx(port, &server, 0xF006, 0, sizeof(hHandle), &hHandle));

        // provide out of range port
        bytesRead = 0xDEADBEEF;
        fructose_assert(ADSERR_CLIENT_PORTNOTOPEN ==
                        AdsSyncReadWriteReqEx2(0, &server, 0xF003, 0, sizeof(buffer), &buffer, sizeof(handleName),
                                               handleName,
                                               &bytesRead));
        fructose_assert(0xDEADBEEF == bytesRead); // BUG? TcAdsDll doesn't reset bytesRead!

        // provide nullptr to AmsAddr
        bytesRead = 0xDEADBEEF;
        fructose_assert(ADSERR_CLIENT_NOAMSADDR ==
                        AdsSyncReadWriteReqEx2(port, nullptr, 0xF003, 0, sizeof(buffer), &buffer, sizeof(handleName),
                                               handleName,
                                               &bytesRead));
        fructose_assert(0xDEADBEEF == bytesRead); // BUG? TcAdsDll doesn't reset bytesRead!

        // provide unknown AmsAddr
        bytesRead = 0xDEADBEEF;
        AmsAddr unknown { { 1, 2, 3, 4, 5, 6 }, AMSPORT_R0_PLC_TC3 };
        fructose_assert(GLOBALERR_MISSING_ROUTE ==
                        AdsSyncReadWriteReqEx2(port, &unknown, 0xF003, 0, sizeof(buffer), &buffer, sizeof(handleName),
                                               handleName,
                                               &bytesRead));
        fructose_assert(0 == bytesRead);

        // provide nullptr to bytesRead
        buffer = 0xDEADBEEF;
        fructose_assert(0 ==
                        AdsSyncReadWriteReqEx2(port, &server, 0xF003, 0, sizeof(buffer), &buffer, sizeof(handleName),
                                               handleName,
                                               nullptr));
        fructose_assert(0xDEADBEEF != buffer);

        // provide nullptr to readBuffer
        fructose_assert(ADSERR_CLIENT_INVALIDPARM ==
                        AdsSyncReadWriteReqEx2(port, &server, 0xF003, 0, sizeof(buffer), nullptr, sizeof(handleName),
                                               handleName,
                                               nullptr));
        fructose_assert(ADSERR_CLIENT_INVALIDPARM ==
                        AdsSyncReadWriteReqEx2(port, &server, 0xF003, 0, sizeof(buffer), nullptr, sizeof(handleName),
                                               handleName,
                                               &bytesRead));

        // provide 0 length readBuffer
        bytesRead = 0xDEADBEEF;
        fructose_assert(ADSERR_DEVICE_INVALIDSIZE ==
                        AdsSyncReadWriteReqEx2(port, &server, 0xF003, 0, 0, &buffer, sizeof(handleName), handleName,
                                               &bytesRead));
        fructose_assert(0 == bytesRead);

        // provide nullptr to writeBuffer
        fructose_assert(ADSERR_CLIENT_INVALIDPARM ==
                        AdsSyncReadWriteReqEx2(port, &server, 0xF003, 0, sizeof(buffer), &buffer, sizeof(handleName),
                                               nullptr,
                                               nullptr));
        fructose_assert(ADSERR_CLIENT_INVALIDPARM ==
                        AdsSyncReadWriteReqEx2(port, &server, 0xF003, 0, sizeof(buffer), &buffer, sizeof(handleName),
                                               nullptr,
                                               &bytesRead));

        // provide 0 length writeBuffer
        bytesRead = 0xDEADBEEF;
        fructose_assert(ADSERR_DEVICE_SYMBOLNOTFOUND ==
                        AdsSyncReadWriteReqEx2(port, &server, 0xF003, 0, sizeof(buffer), &buffer, 0, handleName,
                                               &bytesRead));
        fructose_assert(0 == bytesRead);

        // provide invalid writeBuffer
        bytesRead = 0xDEADBEEF;
        fructose_assert(ADSERR_DEVICE_SYMBOLNOTFOUND ==
                        AdsSyncReadWriteReqEx2(port, &server, 0xF003, 0, sizeof(buffer), &buffer, 3, "xxx",
                                               &bytesRead));
        fructose_assert(0 == bytesRead);

        // provide invalid indexGroup
        bytesRead = 0xDEADBEEF;
        fructose_assert(ADSERR_DEVICE_SRVNOTSUPP ==
                        AdsSyncReadWriteReqEx2(port, &server, 0, 0, sizeof(buffer), &buffer, sizeof(handleName),
                                               handleName,
                                               &bytesRead));
        fructose_assert(0 == bytesRead);

        // provide invalid indexOffset
        bytesRead = 0xDEADBEEF;
        fructose_assert(ADSERR_DEVICE_SRVNOTSUPP ==
                        AdsSyncReadWriteReqEx2(port, &server, 0x4025, 0x10000, sizeof(buffer), &buffer,
                                               sizeof(handleName), handleName,
                                               &bytesRead));
        fructose_assert(0 == bytesRead);
        fructose_assert(0 == AdsPortCloseEx(port));
    }

    void testAdsWriteReqEx(const std::string&)
    {
        const long port = AdsPortOpenEx();
        fructose_assert(0 != port);

        print(server, out);

        uint32_t bytesRead;
        uint32_t buffer;
        uint32_t outBuffer = 0xDEADBEEF;
        for (int i = 0; i < NUM_TEST_LOOPS; ++i) {
            fructose_loop_assert(i, 0 == AdsSyncWriteReqEx(port, &server, 0x4020, 0, sizeof(outBuffer), &outBuffer));
            fructose_loop_assert(i, 0 == AdsSyncReadReqEx2(port,
                                                           &server,
                                                           0x4020,
                                                           0,
                                                           sizeof(buffer),
                                                           &buffer,
                                                           &bytesRead));
            fructose_loop_assert(i, sizeof(buffer) == bytesRead);
            fructose_loop_assert(i, outBuffer == buffer);
            outBuffer = ~outBuffer;
        }

        // provide out of range port
        fructose_assert(ADSERR_CLIENT_PORTNOTOPEN ==
                        AdsSyncWriteReqEx(0, &server, 0x4020, 0, sizeof(outBuffer), &outBuffer));

        // provide nullptr to AmsAddr
        fructose_assert(ADSERR_CLIENT_NOAMSADDR ==
                        AdsSyncWriteReqEx(port, nullptr, 0x4020, 0, sizeof(outBuffer), &outBuffer));

        // provide unknown AmsAddr
        AmsAddr unknown { { 1, 2, 3, 4, 5, 6 }, AMSPORT_R0_PLC_TC3 };
        fructose_assert(GLOBALERR_MISSING_ROUTE ==
                        AdsSyncWriteReqEx(port, &unknown, 0x4020, 0, sizeof(outBuffer), &outBuffer));

        // provide nullptr to writeBuffer
        fructose_assert(ADSERR_CLIENT_INVALIDPARM ==
                        AdsSyncWriteReqEx(port, &server, 0x4020, 0, sizeof(outBuffer), nullptr));

        // provide 0 length writeBuffer
        outBuffer = 0xDEADBEEF;
        buffer = 0;
        fructose_assert(0 == AdsSyncWriteReqEx(port, &server, 0x4020, 0, sizeof(outBuffer), &outBuffer));
        fructose_assert(0 == AdsSyncWriteReqEx(port, &server, 0x4020, 0, 0, &buffer));
        fructose_assert(0 == AdsSyncReadReqEx2(port, &server, 0x4020, 0, sizeof(buffer), &buffer, &bytesRead));
        fructose_assert(outBuffer == buffer);

        // provide invalid indexGroup
        fructose_assert(ADSERR_DEVICE_SRVNOTSUPP == AdsSyncWriteReqEx(port,
                                                                      &server,
                                                                      0,
                                                                      0,
                                                                      sizeof(outBuffer),
                                                                      &outBuffer));

        // provide invalid indexOffset
        fructose_assert(ADSERR_DEVICE_SRVNOTSUPP ==
                        AdsSyncWriteReqEx(port, &server, 0x4025, 0x10000, sizeof(outBuffer), &outBuffer));

        const uint32_t defaultValue = 0;
        fructose_assert(0 == AdsSyncWriteReqEx(port, &server, 0x4020, 0, sizeof(defaultValue), &defaultValue));
        fructose_assert(0 == AdsPortCloseEx(port));
    }

    void testAdsWriteControlReqEx(const std::string&)
    {
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

        // provide out of range port
        fructose_assert(ADSERR_CLIENT_PORTNOTOPEN ==
                        AdsSyncWriteControlReqEx(0, &server, ADSSTATE_STOP, 0, 0, nullptr));

        // provide nullptr to AmsAddr
        fructose_assert(ADSERR_CLIENT_NOAMSADDR ==
                        AdsSyncWriteControlReqEx(port, nullptr, ADSSTATE_STOP, 0, 0, nullptr));

        // provide unknown AmsAddr
        AmsAddr unknown { { 1, 2, 3, 4, 5, 6 }, AMSPORT_R0_PLC_TC3 };
        fructose_assert(GLOBALERR_MISSING_ROUTE == AdsSyncWriteControlReqEx(port,
                                                                            &unknown,
                                                                            ADSSTATE_STOP,
                                                                            0,
                                                                            0,
                                                                            nullptr));

        // provide invalid adsState
        fructose_assert(ADSERR_DEVICE_SRVNOTSUPP ==
                        AdsSyncWriteControlReqEx(port, &server, ADSSTATE_INVALID, 0, 0, nullptr));
        fructose_assert(ADSERR_DEVICE_SRVNOTSUPP ==
                        AdsSyncWriteControlReqEx(port, &server, ADSSTATE_MAXSTATES, 0, 0, nullptr));

        // provide invalid devState
        // TODO find correct parameters for this test

        // provide trash buffer
        uint8_t buffer[10240] {};
        fructose_assert(0 == AdsSyncWriteControlReqEx(port, &server, ADSSTATE_STOP, 0, sizeof(buffer), buffer));
        fructose_assert(0 == AdsSyncWriteControlReqEx(port, &server, ADSSTATE_RUN, 0, sizeof(buffer), buffer));
        fructose_assert(0 == AdsPortCloseEx(port));
    }

    void testAdsNotification(const std::string&)
    {
        const long port = AdsPortOpenEx();

        fructose_assert(0 != port);

        static const size_t MAX_NOTIFICATIONS_PER_PORT = 1024;
        static const size_t LEAKED_NOTIFICATIONS = MAX_NOTIFICATIONS_PER_PORT / 2;
        uint32_t notification[MAX_NOTIFICATIONS_PER_PORT];
        AdsNotificationAttrib attrib = { 1, ADSTRANS_SERVERCYCLE, 0, {1000000} };
        uint32_t hUser = 0xDEADBEEF;

        // provide out of range port
        fructose_assert(ADSERR_CLIENT_PORTNOTOPEN ==
                        AdsSyncAddDeviceNotificationReqEx(0, &server, 0x4020, 0, &attrib, &NotifyCallback, hUser,
                                                          &notification[0]));

        // provide nullptr to AmsAddr
        fructose_assert(ADSERR_CLIENT_NOAMSADDR ==
                        AdsSyncAddDeviceNotificationReqEx(port, nullptr, 0x4020, 0, &attrib, &NotifyCallback, hUser,
                                                          &notification[0]));

        // provide unknown AmsAddr
        AmsAddr unknown { { 1, 2, 3, 4, 5, 6 }, AMSPORT_R0_PLC_TC3 };
        fructose_assert(GLOBALERR_MISSING_ROUTE ==
                        AdsSyncAddDeviceNotificationReqEx(port, &unknown, 0x4020, 0, &attrib, &NotifyCallback, hUser,
                                                          &notification[0]));

        // provide invalid indexGroup
        fructose_assert(ADSERR_DEVICE_SRVNOTSUPP ==
                        AdsSyncAddDeviceNotificationReqEx(port, &server, 0, 0, &attrib, &NotifyCallback, hUser,
                                                          &notification[0]));

        // provide invalid indexOffset
        fructose_assert(ADSERR_DEVICE_SRVNOTSUPP ==
                        AdsSyncAddDeviceNotificationReqEx(port, &server, 0x4025, 0x10000, &attrib, &NotifyCallback,
                                                          hUser,
                                                          &notification[0]));

        // provide nullptr to attrib/callback/hNotification
        fructose_assert(ADSERR_CLIENT_INVALIDPARM ==
                        AdsSyncAddDeviceNotificationReqEx(port, &server, 0x4020, 4, nullptr, &NotifyCallback, hUser,
                                                          &notification[0]));
        fructose_assert(ADSERR_CLIENT_INVALIDPARM ==
                        AdsSyncAddDeviceNotificationReqEx(port, &server, 0x4020, 4, &attrib, nullptr, hUser,
                                                          &notification[0]));
        fructose_assert(ADSERR_CLIENT_INVALIDPARM ==
                        AdsSyncAddDeviceNotificationReqEx(port, &server, 0x4020, 4, &attrib, &NotifyCallback, hUser,
                                                          nullptr));

        // delete nonexisting notification
        fructose_assert(ADSERR_CLIENT_REMOVEHASH == AdsSyncDelDeviceNotificationReqEx(port, &server, 0xDEADBEEF));

        // normal test
        for (hUser = 0; hUser < MAX_NOTIFICATIONS_PER_PORT; ++hUser) {
            fructose_loop_assert(hUser,
                                 0 ==
                                 AdsSyncAddDeviceNotificationReqEx(port, &server, 0x4020, 4, &attrib, &NotifyCallback,
                                                                   hUser,
                                                                   &notification[hUser]));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for (hUser = 0; hUser < MAX_NOTIFICATIONS_PER_PORT - LEAKED_NOTIFICATIONS; ++hUser) {
            fructose_loop_assert(hUser, 0 == AdsSyncDelDeviceNotificationReqEx(port, &server, notification[hUser]));
        }
        fructose_assert(0 == AdsPortCloseEx(port));
    }

    void testAdsTimeout(const std::string&)
    {
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

        timeout = 0;
        // provide out of range port
        fructose_assert(ADSERR_CLIENT_PORTNOTOPEN == AdsSyncGetTimeoutEx(0, &timeout));
        fructose_assert(ADSERR_CLIENT_PORTNOTOPEN == AdsSyncSetTimeoutEx(0, 2000));
        fructose_assert(0 == timeout);

        // provide nullptr to timeout
        fructose_assert(ADSERR_CLIENT_INVALIDPARM == AdsSyncGetTimeoutEx(port, nullptr));
        fructose_assert(0 == AdsPortCloseEx(port));
    }
};

struct TestAdsPerformance : test_base<TestAdsPerformance> {
    std::ostream& out;
    bool runEndurance;

    TestAdsPerformance(std::ostream& outstream)
        : out(outstream),
        runEndurance(false)
    {
        AdsAddRoute(serverNetId, "ads-server");
    }
#ifdef WIN32
    ~TestAdsPerformance()
    {
        // WORKAROUND: On Win7-64 AdsConnection::~AdsConnection() is triggered by the destruction
        //             of the static AdsRouter object and hangs in receive.join()
        AdsDelRoute(serverNetId);
    }
#endif

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

        const auto notification = std::unique_ptr<uint32_t[]>(new uint32_t[numNotifications]);
        AdsNotificationAttrib attrib = { 1, ADSTRANS_SERVERCYCLE, 0, {1000000} };
        uint32_t hUser = 0xDEADBEEF;

        runEndurance = true;
        std::thread threads[1];
        for (auto& t : threads) {
            t = std::thread(&TestAdsPerformance::Read, this, 1024);
        }

        const auto start = std::chrono::high_resolution_clock::now();
        for (hUser = 0; hUser < numNotifications; ++hUser) {
            fructose_assert_eq(0,
                               AdsSyncAddDeviceNotificationReqEx(port, &server, 0x4020, 4, &attrib, &NotifyCallback,
                                                                 hUser,
                                                                 &notification[hUser]));
        }

        std::cout << "Hit ENTER to stop endurance test\n";
        std::cin.ignore();
        runEndurance = false;

        for (hUser = 0; hUser < numNotifications; ++hUser) {
            fructose_assert_eq(0, AdsSyncDelDeviceNotificationReqEx(port, &server, notification[hUser]));
        }
        const auto end = std::chrono::high_resolution_clock::now();
        const auto tmms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        for (auto& t : threads) {
            t.join();
        }
        out << testname << ' ' << 1000 * g_NumNotifications / tmms << " notifications/s (" << g_NumNotifications <<
            '/' << tmms << ")\n";
    }

private:
    void Notifications(size_t numNotifications)
    {
        const long port = AdsPortOpenEx();
        fructose_assert(0 != port);

        const auto notification = std::unique_ptr<uint32_t[]>(new uint32_t[numNotifications]);
        AdsNotificationAttrib attrib = { 1, ADSTRANS_SERVERCYCLE, 0, {1000000} };
        uint32_t hUser = 0xDEADBEEF;

        for (hUser = 0; hUser < numNotifications; ++hUser) {
            fructose_assert_eq(0,
                               AdsSyncAddDeviceNotificationReqEx(port, &server, 0x4020, 4, &attrib, &NotifyCallback,
                                                                 hUser,
                                                                 &notification[hUser]));
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
        for (hUser = 0; hUser < numNotifications; ++hUser) {
            fructose_assert_eq(0, AdsSyncDelDeviceNotificationReqEx(port, &server, notification[hUser]));
        }
        fructose_assert(0 == AdsPortCloseEx(port));
    }

    void Read(const size_t numLoops)
    {
        const long port = AdsPortOpenEx();
        fructose_assert(0 != port);

        uint32_t bytesRead;
        uint32_t buffer;
        do {
            for (size_t i = 0; i < numLoops; ++i) {
                fructose_loop_assert(i,
                                     0 ==
                                     AdsSyncReadReqEx2(port, &server, 0x4020, 0, sizeof(buffer), &buffer, &bytesRead));
                fructose_loop_assert(i, sizeof(buffer) == bytesRead);
                fructose_loop_assert(i, 0 == buffer);
            }
        } while (runEndurance);
        fructose_assert(0 == AdsPortCloseEx(port));
    }
};

int main()
{
    int failedTests = 0;
#if 0
    std::ostream nowhere(0);
    std::ostream& errorstream = nowhere;
#else
    std::ostream& errorstream = std::cout;
#endif
#if 1
    TestAmsAddr amsAddrTest(errorstream);
    amsAddrTest.add_test("testAmsAddrCompare", &TestAmsAddr::testAmsAddrCompare);
    failedTests += amsAddrTest.run();

    TestAmsRouter routerTest(errorstream);
    routerTest.add_test("testAmsRouterAddRoute", &TestAmsRouter::testAmsRouterAddRoute);
    routerTest.add_test("testAmsRouterDelRoute", &TestAmsRouter::testAmsRouterDelRoute);
//    routerTest.add_test("testConcurrentRoutes", &TestAmsRouter::testConcurrentRoutes);
    routerTest.add_test("testAmsRouterSetLocalAddress", &TestAmsRouter::testAmsRouterSetLocalAddress);
    failedTests += routerTest.run();

    TestIpV4 ipv4Test(errorstream);
    ipv4Test.add_test("testComparsion", &TestIpV4::testComparsion);
    failedTests += ipv4Test.run();

    TestRingBuffer ringBufferTest(errorstream);
    ringBufferTest.add_test("testBytesFree", &TestRingBuffer::testBytesFree);
    ringBufferTest.add_test("testWriteChunk", &TestRingBuffer::testWriteChunk);
    failedTests += ringBufferTest.run();
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
    failedTests += adsTest.run();

    TestAdsPerformance performance(errorstream);
    performance.add_test("testManyNotifications", &TestAdsPerformance::testManyNotifications);
    performance.add_test("testParallelReadAndWrite", &TestAdsPerformance::testParallelReadAndWrite);
//	performance.add_test("testEndurance", &TestAdsPerformance::testEndurance);
    failedTests += performance.run();

    std::cout << "Hit ENTER to continue\n";
    std::cin.ignore();
    return failedTests;
}
