#ifndef _ROUTER_H_
#define _ROUTER_H_

#include "AdsDef.h"

struct Router {
	static const size_t NUM_PORTS_MAX = 128;
	static const uint16_t PORT_BASE = 30000;
	static_assert(NUM_PORTS_MAX + PORT_BASE <= UINT16_MAX, "Port limit is out of range");

	virtual long GetLocalAddress(uint16_t port, AmsAddr* pAddr) = 0;
};
#endif /* #ifndef _ROUTER_H_ */