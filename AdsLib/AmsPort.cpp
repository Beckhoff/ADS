
#include "AmsPort.h"

AmsPort::AmsPort()
	:isOpen(false)
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