#ifndef _AMS_PORT_H_
#define _AMS_PORT_H_

#include "AdsDef.h"

#include <mutex>
#include <set>

struct AmsPort
{
	AmsPort(const AmsNetId *__localAddr = nullptr);
	void operator=(const AmsPort &ref);
	void Close();
	long GetLocalAddress(AmsAddr* pAddr) const;
	bool IsOpen() const;
	uint16_t Open(uint16_t __port);
	uint32_t tmms;
	uint16_t port;

	void AddNotification(size_t hash);
	void DelNotification(size_t hash);
	std::set<size_t> notifications;
private:
	static const uint32_t DEFAULT_TIMEOUT = 5000;
	const AmsNetId *const localAddr;
	std::mutex mutex;
};
#endif /* #ifndef _AMS_PORT_H_ */