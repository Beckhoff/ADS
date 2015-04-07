
#include "AdsLib.h"

#include <iostream>

static void NotifyCallback(const AmsAddr* pAddr, const AdsNotificationHeader* pNotification, uint32_t hUser)
{
	std::cout << "hUser 0x" << std::hex << hUser
		<< " sample time: " << std::dec << pNotification->nTimeStamp
		<< " sample size: " << std::dec << pNotification->cbSampleSize
		<< " value: 0x" << std::hex << (int)pNotification->data[0] << '\n';
}

void notificationExample(std::ostream &out, long port, const AmsAddr& server)
{
	const AdsNotificationAttrib attrib = {
		1,
		ADSTRANS_SERVERCYCLE,
		0,
		4000000
	};
	uint32_t hNotify;
	uint32_t hUser;
	
	const long addStatus = AdsSyncAddDeviceNotificationReqEx(port, &server, 0x4020, 4, &attrib, &NotifyCallback, hUser, &hNotify);
	if (addStatus) {
		out << "Add device notification failed with: " << std::dec << addStatus;
		return;
	}

	std::cout << "Hit ENTER to stop notifications\n";
	std::cin.ignore();
	
	const long delStatus = AdsSyncDelDeviceNotificationReqEx(port, &server, hNotify);
	if (delStatus) {
		out << "Delete device notification failed with: " << std::dec << addStatus;
		return;
	}
}

void readExample(std::ostream &out, long port, const AmsAddr& server)
{
	uint32_t bytesRead;
	uint32_t buffer;

	for (size_t i = 0; i < 8; ++i) {
		const long status = AdsSyncReadReqEx2(port, &server, 0x4020, 0, sizeof(buffer), &buffer, &bytesRead);
		if (status) {
			out << "ADS read failed with: " << std::dec << status << '\n';
			return;
		}
		out << "ADS read " << std::dec << bytesRead << " bytes, value: 0x" << std::hex << buffer << '\n';
	}
}

void runExample(std::ostream &out)
{
	static const AmsNetId remoteNetId { 192, 168, 0, 231, 1, 1 };
	static const AmsNetId localNetId  { 192, 168, 0, 164, 1, 1 };
	static const char remoteIpV4[] = "192.168.0.232";

	// register own AMSNetId locally (make sure it was added to the routing table of your EtherCAT Master)
	AdsSetNetId(localNetId);

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

	const AmsAddr remote{ remoteNetId, AMSPORT_R0_PLC_TC3 };
	notificationExample(out, port, remote);
	readExample(out, port, remote);
	
	const long closeStatus = AdsPortCloseEx(port);
	if (closeStatus) {
		out << "Close ADS port failed with: " << std::dec << closeStatus << '\n';
	}

#ifdef WIN32
	// WORKAROUND: On Win7 std::thread::join() called in destructors
	//             of static objects might wait forever...
	AdsDelRoute(AmsNetId{ 192, 168, 0, 231, 1, 1 });
#endif		
}

int main()
{
	runExample(std::cout);
}
