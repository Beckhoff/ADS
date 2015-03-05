#ifndef _AMS_ROUTER_H_
#define _AMS_ROUTER_H_

#include "AdsDef.h"

struct AmsRouter
{
	AmsRouter();

	uint16_t OpenPort();
	long ClosePort(uint16_t port);
	long GetLocalAddress(uint16_t port, AmsAddr* pAddr);

};
#endif /* #ifndef _AMS_ROUTER_H_ */
