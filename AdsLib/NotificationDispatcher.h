#ifndef _NOTIFICATION_DISPATCHER_H_
#define _NOTIFICATION_DISPATCHER_H_

#include "AdsDef.h"

struct NotificationDispatcher {
	virtual void Dispatch(AmsAddr amsAddr) const = 0;
};
#endif /* #ifndef _NOTIFICATION_DISPATCHER_H_ */
