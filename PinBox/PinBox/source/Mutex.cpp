#include "Mutex.h"



Mutex::Mutex()
{
	_isLocked = false;
	LightLock_Init(&_handler);
}


Mutex::~Mutex()
{
	if(_isLocked) LightLock_Unlock(&_handler);
}

void Mutex::Lock()
{
	LightLock_Lock(&_handler);
}

void Mutex::TryLock()
{
	if(!LightLock_TryLock(&_handler)) _isLocked = true;
}

void Mutex::Unlock()
{
	LightLock_Unlock(&_handler);
	_isLocked = false;
}
