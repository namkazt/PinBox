#include "ConfigManager.h"
#include <string>
#define CONFIG_FILE_NAME "pinbox.cfg"

static ConfigManager* ref;
ConfigManager* ConfigManager::Get()
{
	if (ref == nullptr) ref = new ConfigManager();
	return ref;
}

ConfigManager::ConfigManager()
{
}

void ConfigManager::InitConfig()
{
	config_init(&_config);
	if (!config_read_file(&_config, CONFIG_FILE_NAME))
	{
		config_setting_t *root, *setting;
		root = config_root_setting(&_config);
		setting = config_setting_add(root, "ip", CONFIG_TYPE_STRING);
		config_setting_set_string(setting, "");
		setting = config_setting_add(root, "video_quality", CONFIG_TYPE_INT);
		config_setting_set_int(setting, 75);
		setting = config_setting_add(root, "video_scale", CONFIG_TYPE_INT);
		config_setting_set_int(setting, 100);
		setting = config_setting_add(root, "skip_frame", CONFIG_TYPE_INT);
		config_setting_set_int(setting, 1);
		setting = config_setting_add(root, "wait_for_received", CONFIG_TYPE_BOOL);
		config_setting_set_bool(setting, true);

		config_set_options(&_config, 0);
		config_write_file(&_config, CONFIG_FILE_NAME);

		_cfg_ip = "";
		_cfg_video_quality = 75;
		_cfg_video_scale = 100;
		_cfg_skip_frame = 1;
		_cfg_wait_for_received = true;
	}else
	{
		if (!config_lookup_string(&_config, "ip", &_cfg_ip))
		{
			_cfg_ip = "";
		}
		if (!config_lookup_int(&_config, "video_quality", &_cfg_video_quality))
		{
			_cfg_video_quality = 75;
		}
		if (!config_lookup_int(&_config, "video_scale", &_cfg_video_scale))
		{
			_cfg_video_scale = 100;
		}
		if (!config_lookup_int(&_config, "skip_frame", &_cfg_skip_frame))
		{
			_cfg_skip_frame = 1;
		}
		int wait_for_received = 1;
		if (!config_lookup_bool(&_config, "wait_for_received", &wait_for_received))
		{
			_cfg_wait_for_received = wait_for_received;
		}

		printf("IP: %s\n", _cfg_ip);
	}
}

void ConfigManager::Save()
{
	config_setting_t *root, *setting;
	root = config_root_setting(&_config);
	setting = config_setting_get_member(root, "ip");
	config_setting_set_string(setting, _cfg_ip);
	setting = config_setting_get_member(root, "video_quality");
	config_setting_set_int(setting, _cfg_video_quality);
	setting = config_setting_get_member(root, "video_scale");
	config_setting_set_int(setting, _cfg_video_scale);
	setting = config_setting_get_member(root, "skip_frame");
	config_setting_set_int(setting, _cfg_skip_frame);
	setting = config_setting_get_member(root, "wait_for_received");
	config_setting_set_bool(setting, _cfg_wait_for_received);
	config_write_file(&_config, CONFIG_FILE_NAME);

	printf("Save IP: %s\n", _cfg_ip);
}

void ConfigManager::Destroy()
{
	config_destroy(&_config);
}
