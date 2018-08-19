#pragma once
#ifndef _CONFIG_MANAGER_H_
#define _CONFIG_MANAGER_H_

#include <3ds.h>
#include "libconfig.h"
#include <vector>

#define FORCE_OVERRIDE_VERSION 1

typedef struct ServerConfig {
	const char* ip;
	const char* port;
	const char* name;
};

class ConfigManager
{
private:
	config_t _config;
	bool shouldCreateNewConfigFile();
	void createNewConfigFile();
	void loadConfigFile();

public:

	ServerConfig* activateServer;

	std::vector<ServerConfig> servers;
	int lastUsingServer = -1;

	int videoBitRate;
	int videoGOP;
	int videoMaxBFrames;

	int audioBitRate;

	bool waitForSync;
public:
	static ConfigManager* Get();
	ConfigManager();

	void InitConfig();
	void Save();
	void Destroy();
};


#endif