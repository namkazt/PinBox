#include "Mutex.h"



Mutex::Mutex()
{
	g_isLocked = false;
	svcCreateMutex(&g_mutexHandle, false);
}


Mutex::~Mutex()
{
	svcCloseHandle(g_mutexHandle);
}

void Mutex::Lock()
{
	if(g_isLocked)
	{
		svcWaitSynchronization(g_mutexHandle, U64_MAX);
	}else
	{
		g_isLocked = true;
	}
}

void Mutex::Unlock()
{
	if (g_isLocked)
	{
		svcReleaseMutex(g_mutexHandle);
		g_isLocked = false;
	}
}
