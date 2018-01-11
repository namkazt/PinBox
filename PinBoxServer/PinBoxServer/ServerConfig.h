#pragma once

#include <libconfig.h++>
#include <iostream>

class ServerConfig
{
public:
	ServerConfig();
	~ServerConfig();

	static ServerConfig* Get();

	int							MonitorIndex = 0;
	int							CaptureFPS = 40;
	void LoadConfig();
};

