#pragma once
#ifndef _PP_SERVER_CONFIG_H__
#define _PP_SERVER_CONFIG_H__
#include <libconfig.h++>
#include <iostream>

class ServerConfig
{
public:
	ServerConfig();
	~ServerConfig();

	static ServerConfig* Get();

	int							MonitorIndex = 0;
	int							CaptureFPS = 30;
	int							NetworkThread = 2;
	int							ServerPort = 1234;
	void LoadConfig();
};

#endif