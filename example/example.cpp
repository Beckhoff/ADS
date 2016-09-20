#include "AdsClient/AdsClient.h"
#include "AdsLib.h"

#include <iostream>

static void NotifyCallback(const AmsAddr* pAddr, const AdsNotificationHeader* pNotification, uint32_t hUser)
{
    const uint8_t* data = reinterpret_cast<const uint8_t*>(pNotification + 1);
    std::cout << "hUser 0x" << std::hex << hUser <<
        " sample time: " << std::dec << pNotification->nTimeStamp <<
        " sample size: " << std::dec << pNotification->cbSampleSize <<
        " value:";
    for (size_t i = 0; i < pNotification->cbSampleSize; ++i) {
        std::cout << " 0x" << std::hex << (int)data[i];
    }
    std::cout << '\n';
}

uint32_t getHandleByNameExample(std::ostream& out, long port, const AmsAddr& server,
                                const std::string handleName)
{
    uint32_t handle = 0;
    const long handleStatus = AdsSyncReadWriteReqEx2(port,
                                                     &server,
                                                     ADSIGRP_SYM_HNDBYNAME,
                                                     0,
                                                     sizeof(handle),
                                                     &handle,
                                                     handleName.size(),
                                                     handleName.c_str(),
                                                     nullptr);
    if (handleStatus) {
        out << "Create handle for '" << handleName << "' failed with: 0x" << std::hex << handleStatus << '\n';
    }
    return handle;
}

void releaseHandleExample(std::ostream& out, long port, const AmsAddr& server, uint32_t handle)
{
    const long releaseHandle = AdsSyncWriteReqEx(port, &server, ADSIGRP_SYM_RELEASEHND, 0, sizeof(handle), &handle);
    if (releaseHandle) {
        out << "Release handle 0x" << std::hex << handle << "' failed with: 0x" << releaseHandle << '\n';
    }
}

void notificationExample(std::ostream& out, long port, const AmsAddr& server)
{
    const AdsNotificationAttrib attrib = {
        1,
        ADSTRANS_SERVERCYCLE,
        0,
        {4000000}
    };
    uint32_t hNotify;
    uint32_t hUser = 0;

    const long addStatus = AdsSyncAddDeviceNotificationReqEx(port,
                                                             &server,
                                                             0x4020,
                                                             4,
                                                             &attrib,
                                                             &NotifyCallback,
                                                             hUser,
                                                             &hNotify);
    if (addStatus) {
        out << "Add device notification failed with: " << std::dec << addStatus << '\n';
        return;
    }

    std::cout << "Hit ENTER to stop notifications\n";
    std::cin.ignore();

    const long delStatus = AdsSyncDelDeviceNotificationReqEx(port, &server, hNotify);
    if (delStatus) {
        out << "Delete device notification failed with: " << std::dec << delStatus;
        return;
    }
}

void notificationByNameExample(std::ostream& out, long port, const AmsAddr& server)
{
    const AdsNotificationAttrib attrib = {
        1,
        ADSTRANS_SERVERCYCLE,
        0,
        {4000000}
    };
    uint32_t hNotify;
    uint32_t hUser = 0;

    uint32_t handle;

    out << __FUNCTION__ << "():\n";
    handle = getHandleByNameExample(out, port, server, "MAIN.byByte");

    const long addStatus = AdsSyncAddDeviceNotificationReqEx(port,
                                                             &server,
                                                             ADSIGRP_SYM_VALBYHND,
                                                             handle,
                                                             &attrib,
                                                             &NotifyCallback,
                                                             hUser,
                                                             &hNotify);
    if (addStatus) {
        out << "Add device notification failed with: " << std::dec << addStatus << '\n';
        return;
    }

    std::cout << "Hit ENTER to stop by name notifications\n";
    std::cin.ignore();

    const long delStatus = AdsSyncDelDeviceNotificationReqEx(port, &server, hNotify);
    if (delStatus) {
        out << "Delete device notification failed with: " << std::dec << delStatus;
        return;
    }
    releaseHandleExample(out, port, server, handle);
}

void readExample(std::ostream& out, long port, const AmsAddr& server)
{
    uint32_t bytesRead;
    uint32_t buffer;

    out << __FUNCTION__ << "():\n";
    for (size_t i = 0; i < 8; ++i) {
        const long status = AdsSyncReadReqEx2(port, &server, 0x4020, 0, sizeof(buffer), &buffer, &bytesRead);
        if (status) {
            out << "ADS read failed with: " << std::dec << status << '\n';
            return;
        }
        out << "ADS read " << std::dec << bytesRead << " bytes, value: 0x" << std::hex << buffer << '\n';
    }
}

void readByNameExample(std::ostream& out, long port, const AmsAddr& server)
{
    uint32_t bytesRead;
    uint32_t buffer;
    uint32_t handle;

    out << __FUNCTION__ << "():\n";
    handle = getHandleByNameExample(out, port, server, "MAIN.byByte");

    for (size_t i = 0; i < 8; ++i) {
        const long status = AdsSyncReadReqEx2(port,
                                              &server,
                                              ADSIGRP_SYM_VALBYHND,
                                              handle,
                                              sizeof(buffer),
                                              &buffer,
                                              &bytesRead);
        if (status) {
            out << "ADS read failed with: " << std::dec << status << '\n';
            return;
        }
        out << "ADS read " << std::dec << bytesRead << " bytes, value: 0x" << std::hex << buffer << '\n';
    }
    releaseHandleExample(out, port, server, handle);
}

void readStateExample(std::ostream& out, long port, const AmsAddr& server)
{
    uint16_t adsState;
    uint16_t devState;

    const long status = AdsSyncReadStateReqEx(port, &server, &adsState, &devState);
    if (status) {
        out << "ADS read failed with: " << std::dec << status << '\n';
        return;
    }
    out << "ADS state: " << std::dec << adsState << " devState: " << std::dec << devState << '\n';
}

void runExample(std::ostream& out)
{
    static const AmsNetId remoteNetId { 192, 168, 0, 231, 1, 1 };
    static const char remoteIpV4[] = "192.168.0.232";

    // add local route to your EtherCAT Master
    if (AdsAddRoute(remoteNetId, remoteIpV4)) {
        out << "Adding ADS route failed, did you specified valid addresses?\n";
        return;
    }

    // open a new ADS port
    const long port = AdsPortOpenEx();
    if (!port) {
        out << "Open ADS port failed\n";
        return;
    }

    const AmsAddr remote { remoteNetId, AMSPORT_R0_PLC_TC3 };
    notificationExample(out, port, remote);
    notificationByNameExample(out, port, remote);
    readExample(out, port, remote);
    readByNameExample(out, port, remote);
    readStateExample(out, port, remote);

    const long closeStatus = AdsPortCloseEx(port);
    if (closeStatus) {
        out << "Close ADS port failed with: " << std::dec << closeStatus << '\n';
    }

#ifdef _WIN32
    // WORKAROUND: On Win7 std::thread::join() called in destructors
    //             of static objects might wait forever...
    AdsDelRoute(remoteNetId);
#endif
}

void runAdsClientExample(std::ostream& out)
{
    static const AmsNetId remoteNetId { 192, 168, 0, 231, 1, 1 };
    static const char remoteIpV4[] = "192.168.0.232";
    static const AmsAddr remoteNetAddress { remoteNetId, 851 };

    out << __FUNCTION__ << "():\n";

    try {
        AdsRoute {remoteNetId, remoteIpV4};
        AdsClient adsClient {remoteNetAddress};

        // Write and read values
        uint8_t valueToWrite = 0x99;
        auto simpleVar = adsClient.GetAdsVariable<uint8_t>("MAIN.byByte[0]");
        simpleVar = valueToWrite;
        out << "Wrote " << (uint32_t)valueToWrite << " to MAIN.byByte and read " << (uint32_t)simpleVar << " back\n";

        // Write and read values by index group/offset
        auto simpleVarIndex = adsClient.GetAdsVariable<uint8_t>(0x4020, 0);
        simpleVarIndex = valueToWrite;
        out << "Wrote " << (uint32_t)valueToWrite << " to MAIN.byByte and read " << (uint32_t)simpleVarIndex <<
            " back\n";

        // Write and read arrays
        std::array<uint8_t, 4> arrayToWrite = { 1, 2, 3, 4 };
        auto arrayVar = adsClient.GetAdsVariable<std::array<uint8_t, 4> >("MAIN.byByte");
        arrayVar = arrayToWrite;
        std::array<uint8_t, 4> readArray = arrayVar;
        out << "Wrote array with first value " << (uint32_t)arrayToWrite[0] << " and last value " <<
        (uint32_t)arrayToWrite[3] << "\n";
        out << "Read back array with first value " << (uint32_t)readArray[0] << " and last value " <<
        (uint32_t)readArray[3] << "\n";

        // Write and read arrays by index group/offset
        auto arrayVarIndex = adsClient.GetAdsVariable<std::array<uint8_t, 4> >(0x4020, 0);
        arrayVarIndex = arrayToWrite;
        std::array<uint8_t, 4> readArrayIndex = arrayVarIndex;
        out << "Wrote array with first value " << (uint32_t)arrayToWrite[0] << " and last value " <<
        (uint32_t)arrayToWrite[3] << "\n";
        out << "Read back array with first value " << (uint32_t)readArrayIndex[0] << " and last value " <<
        (uint32_t)readArrayIndex[3] << "\n";

        // Read device info
        auto deviceInfo = adsClient.ReadDeviceInfo();
        out << "Read device info from device " << deviceInfo.name << ". Version is " <<
        (uint32_t)deviceInfo.version.version << "." << (uint32_t)deviceInfo.version.revision << "." <<
        (uint32_t)deviceInfo.version.build << "\n";
    } catch (const AdsException& ex) {
        auto errorCode = ex.getErrorCode();
        out << "Error: " << errorCode << "\n";
        out << "AdsException message: " << ex.what() << "\n";
    }
}

int main()
{
//    runExample(std::cout);
    runAdsClientExample(std::cout);
}
