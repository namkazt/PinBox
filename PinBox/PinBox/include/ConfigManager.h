#pragma once
#ifndef _CONFIG_MANAGER_H_
#define _CONFIG_MANAGER_H_

#include <3ds.h>
#include "libconfig.h"
#include <vector>
#include <string>

#define FORCE_OVERRIDE_VERSION 2

typedef struct ServerConfig {
	std::string ip;
	std::string port;
	std::string name;
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