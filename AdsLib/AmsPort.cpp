
#include "AmsPort.h"

AmsPort::AmsPort()
	: tmms(DEFAULT_TIMEOUT),
	isOpen(false)
{}

void AmsPort::Close()
{
	isOpen = false;
}

bool AmsPort::IsOpen() const
{
	return isOpen;
}

void AmsPort::Open()
{
	isOpen = true;
}