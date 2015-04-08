#ifndef _AMS_PORT_H_
#define _AMS_PORT_H_

struct AmsPort
{
	AmsPort();
	void Close();
	bool IsOpen() const;
	void Open();
private:
	bool isOpen;	
};
#endif /* #ifndef _AMS_PORT_H_ */