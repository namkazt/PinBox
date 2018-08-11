#pragma once
#include <3ds.h>
#include <3ds/svc.h>

class Mutex
{
private:
	Handle								g_mutexHandle = 0;
	bool								g_isLocked = false;

public:
	Mutex();
	~Mutex();

	void Lock();
	void Unlock();
};

