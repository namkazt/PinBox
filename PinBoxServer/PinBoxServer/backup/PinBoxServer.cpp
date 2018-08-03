// PinBoxServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "PPServer.h"

int main(int argc, char *argv[])
{
	PPServer *server = new PPServer();
	server->InitServer();

	return 0;
}

#include "winmain-inl.h"