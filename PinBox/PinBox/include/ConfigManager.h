#pragma once
#ifndef _CONFIG_MANAGER_H_
#define _CONFIG_MANAGER_H_

#include <3ds.h>
#include "libconfig.h"

class ConfigManager
{
private:
	config_t _config;

public:
	const char* _cfg_ip;
	const char* _cfg_port;
	int _cfg_video_quality;
	int _cfg_video_scale;
	int _cfg_skip_frame;
	bool _cfg_wait_for_received;
public:
	static ConfigManager* Get();
	ConfigManager();

	void InitConfig();
	void Save();
	void Destroy();
};


#endif