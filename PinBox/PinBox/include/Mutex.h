#pragma once
#include <3ds.h>

class Mutex
{
private:
	Handle								g_mutexHandle = 0;
	volatile bool						g_isLocked = false;

public:
	Mutex();
	~Mutex();

	void Lock();
	void Unlock();
};

