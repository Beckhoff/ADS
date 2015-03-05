#ifndef _ADSLIB_H_
#define _ADSLIB_H_

#include <stdint.h>
#include "AdsDef.h"

long AdsPortCloseEx(long port);
long AdsPortOpenEx();
long AdsGetLocalAddressEx(long port, AmsAddr* pAddr);

#endif /* #ifndef _ADSLIB_H_ */