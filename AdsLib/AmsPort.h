#ifndef _AMS_PORT_H_
#define _AMS_PORT_H_

#include "AdsDef.h"

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
private:
	static const uint32_t DEFAULT_TIMEOUT = 5000;
	const AmsNetId *const localAddr;
};
#endif /* #ifndef _AMS_PORT_H_ */