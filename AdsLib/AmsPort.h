#ifndef _AMS_PORT_H_
#define _AMS_PORT_H_

#include <cstdint>

struct AmsPort
{
	AmsPort();
	void Close();
	bool IsOpen() const;
	void Open();
	uint32_t tmms;
private:
	static const uint32_t DEFAULT_TIMEOUT = 5000;
	bool isOpen;	
};
#endif /* #ifndef _AMS_PORT_H_ */