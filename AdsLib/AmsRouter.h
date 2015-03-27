#ifndef _AMS_ROUTER_H_
#define _AMS_ROUTER_H_

#include "AdsDef.h"
#include "AmsConnection.h"
#include "AdsNotification.h"

#include <array>
#include <bitset>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

struct AmsRouter : NotificationDispatcher
{
	AmsRouter(AmsNetId netId = AmsNetId{0, 0, 0, 0, 0, 0});

	uint16_t OpenPort();
	long ClosePort(uint16_t port);
	long GetLocalAddress(uint16_t port, AmsAddr* pAddr);
	long GetTimeout(uint16_t port, uint32_t& timeout);
	long SetTimeout(uint16_t port, uint32_t timeout);
	long Read(uint16_t port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, void* buffer, uint32_t *bytesRead);
	long ReadDeviceInfo(uint16_t port, const AmsAddr* pAddr, char* devName, AdsVersion* version);
	long ReadState(uint16_t port, const AmsAddr* pAddr, uint16_t* adsState, uint16_t* deviceState);
	long ReadWrite(uint16_t port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t readLength, void* readData, uint32_t writeLength, const void* writeData, uint32_t *bytesRead);
	long Write(uint16_t port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, uint32_t bufferLength, const void* buffer);
	long WriteControl(uint16_t port, const AmsAddr* pAddr, uint16_t adsState, uint16_t devState, uint32_t bufferLength, const void* buffer);
	long AddNotification(uint16_t port, const AmsAddr* pAddr, uint32_t indexGroup, uint32_t indexOffset, const AdsNotificationAttrib* pAttrib, PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t *pNotification);
	long DelNotification(uint16_t port, const AmsAddr* pAddr, uint32_t hNotification);

	bool AddRoute(AmsNetId ams, const IpV4& ip);
	void DelRoute(const AmsNetId& ams);
	void SetNetId(AmsNetId ams);
	AmsConnection* GetConnection(const AmsNetId& pAddr);

	void Dispatch(Frame &frame, AmsAddr amsAddr) const;

private:
	static const uint32_t DEFAULT_TIMEOUT;
	static const size_t NUM_PORTS_MAX = 8;
	static const uint16_t PORT_BASE = 30000;
	static_assert(PORT_BASE + NUM_PORTS_MAX <= UINT16_MAX, "Port limit is out of range");

	std::bitset<NUM_PORTS_MAX> ports;
	std::array<uint32_t, NUM_PORTS_MAX> portTimeout;
	AmsNetId localAddr;
	std::mutex mutex;
	std::map<IpV4, std::unique_ptr<AmsConnection>> connections;
	std::map<AmsNetId, AmsConnection*> mapping;

	std::map<IpV4, std::unique_ptr<AmsConnection>>::iterator __GetConnection(const AmsNetId& pAddr);
	void DeleteIfLastConnection(const AmsConnection* conn);
	void Recv();

	template<class T>
	long AdsRequest(Frame& request, const AmsAddr& destAddr, uint16_t port, uint16_t cmdId, uint32_t bufferLength = 0, void* buffer = nullptr, uint32_t *bytesRead = nullptr);

	using NotifyTable = std::map < uint32_t, Notification >;
	using NotifyPair = std::pair < AmsAddr, uint32_t >;
	using TableRef = std::unique_ptr<NotifyTable>;
	std::map<AmsAddr, TableRef> tableMapping;
	std::mutex notificationLock;
	void CreateNotifyMapping(uint16_t port, AmsAddr destAddr, PAdsNotificationFuncEx pFunc, uint32_t hUser, uint32_t length, uint32_t hNotify);
	bool DeleteNotifyMapping(const AmsAddr &addr, uint32_t hNotify);
	std::vector<NotifyPair> CollectOrphanedNotifications(uint16_t port);

};
#endif /* #ifndef _AMS_ROUTER_H_ */
