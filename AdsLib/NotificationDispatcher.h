#ifndef _NOTIFICATION_DISPATCHER_H_
#define _NOTIFICATION_DISPATCHER_H_

#include "AdsDef.h"
#include "Frame.h"

struct NotificationDispatcher {
	virtual void Dispatch(Frame &frame, AmsAddr amsAddr) const = 0;
	Frame& GetFrame() { return frame; };
private:
	Frame frame{ 10240 };
};
#endif /* #ifndef _NOTIFICATION_DISPATCHER_H_ */
