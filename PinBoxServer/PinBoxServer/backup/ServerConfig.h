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
	int							CaptureFPS = 40;
	void LoadConfig();
};

#endif