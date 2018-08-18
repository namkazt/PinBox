#pragma once
#include <3ds.h>
#include <3ds/svc.h>

class Mutex
{
private:
	LightLock							_handler;
	bool								_isLocked = false;

public:
	Mutex();
	~Mutex();

	void Lock();
	void TryLock();
	void Unlock();
};

