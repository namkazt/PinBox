// PinBoxServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "PPServer.h"

int main()
{
	PPServer *server = new PPServer();
	server->InitServer();


	//int a = 0;
	//std::cin >> a;

    return 0;
}

#include "winmain-inl.h"