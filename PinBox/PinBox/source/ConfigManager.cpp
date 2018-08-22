#include "ConfigManager.h"
#define CONFIG_FILE_NAME "pinbox/pinbox.cfg"

static ConfigManager* ref;

ConfigManager* ConfigManager::Get()
{
	if (ref == nullptr) ref = new ConfigManager();
	return ref;
}

ConfigManager::ConfigManager()
{
}


bool ConfigManager::shouldCreateNewConfigFile()
{
	if (!config_read_file(&_config, CONFIG_FILE_NAME)) {
		printf("> Config file was not found.\n");
		return true;
	}
	config_setting_t *root, *setting;
	root = config_root_setting(&_config);
	int version = 0;
	if (!config_lookup_int(&_config, "version", &version)) {
		printf("> Config do not have version.\n");
		return true;
	}
	if (version < FORCE_OVERRIDE_VERSION) {
		printf("> Config file was outdate.\n");
		return true;
	}
	return false;
}

void ConfigManager::createNewConfigFile()
{
	config_setting_t *root, *setting;
	config_destroy(&_config);
	config_init(&_config);
	root = config_root_setting(&_config);

	// server config group
	setting = config_setting_add(root, "servers", CONFIG_TYPE_LIST);
	setting = config_setting_add(root, "last_using_server", CONFIG_TYPE_INT);
	config_setting_set_int(setting, -1);

	// video config
	setting = config_setting_add(root, "video_bit_rate", CONFIG_TYPE_INT);
	config_setting_set_int(setting, 120000);
	setting = config_setting_add(root, "video_gop", CONFIG_TYPE_INT);
	config_setting_set_int(setting, 18);
	setting = config_setting_add(root, "video_max_b_frames", CONFIG_TYPE_INT);
	config_setting_set_int(setting, 2);

	// audio
	setting = config_setting_add(root, "audio_bit_rate", CONFIG_TYPE_INT);
	config_setting_set_int(setting, 48000);

	// mode
	setting = config_setting_add(root, "wait_for_sync", CONFIG_TYPE_BOOL);
	config_setting_set_bool(setting, true);

	setting = config_setting_add(root, "version", CONFIG_TYPE_INT);
	config_setting_set_int(setting, FORCE_OVERRIDE_VERSION);

	config_set_options(&_config,
		(CONFIG_OPTION_SEMICOLON_SEPARATORS
			| CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS
			| CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE));
	config_write_file(&_config, CONFIG_FILE_NAME);

	// default values
	videoBitRate = 120000;
	videoGOP = 18;
	videoMaxBFrames = 2;
	audioBitRate = 48000;
	waitForSync = true;
	lastUsingServer = -1;
}

void ConfigManager::loadConfigFile()
{
	config_setting_t *root, *setting;
	root = config_root_setting(&_config);
	setting = config_lookup(&_config, "servers");
	if (setting != NULL)
	{
		printf("> Load server profiles\n");
		int count = config_setting_length(setting), i = 0;
		for(i = 0; i < count; ++i)
		{
			config_setting_t* serverCfg = config_setting_get_elem(setting, i);
			ServerConfig server{};
			const char* ip, * port, *name;
			if(!(config_setting_lookup_string(serverCfg, "ip", &ip)
				&& config_setting_lookup_string(serverCfg, "port", &port)
				&& config_setting_lookup_string(serverCfg, "name", &name)))
			{
				continue;
			}
			server.name = std::string(name);
			server.ip = std::string(ip);
			server.port = std::string(port);
			servers.push_back(server);
		}
	}
	if (!config_lookup_int(&_config, "last_using_server", &lastUsingServer)) lastUsingServer = -1;

	// video
	if (!config_lookup_int(&_config, "video_bit_rate", &videoBitRate)) videoBitRate = 120000;
	if (!config_lookup_int(&_config, "video_gop", &videoGOP)) videoGOP = 18;
	if (!config_lookup_int(&_config, "video_max_b_frames", &videoMaxBFrames)) videoMaxBFrames = 2;

	// audio
	if (!config_lookup_int(&_config, "audio_bit_rate", &audioBitRate)) audioBitRate = 48000;

	// mode
	int waitForReceived = 0;
	if (!config_lookup_bool(&_config, "wait_for_sync", &waitForReceived)) {
		waitForSync = true;
	}
	else waitForSync = waitForReceived;
}


void ConfigManager::InitConfig()
{
	config_init(&_config);
	config_set_options(&_config,
		(CONFIG_OPTION_SEMICOLON_SEPARATORS
			| CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS
			| CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE));
	printf("----------------------\nInitialize Config\n----------------------\n");
	if(shouldCreateNewConfigFile())
	{
		printf("> Create new config file\n");
		createNewConfigFile();
	}else
	{
		printf("> Load config file\n");
		loadConfigFile();
	}
}

void ConfigManager::Save()
{
	config_setting_t *root, *setting, *element;
	root = config_root_setting(&_config);

	// reset server list
	setting = config_lookup(&_config, "servers");
	if (setting != NULL) {
		int profileCount = config_setting_length(setting);
		while (profileCount >= 0)
		{
			int ret = config_setting_remove_elem(setting, 0);
			if (ret != CONFIG_TRUE)
			{
				printf("> Failed to detele profile at: %d\n", profileCount);
			}
			--profileCount;
		}
	}
	for (auto& i : servers)
	{
		config_setting_t* server = config_setting_add(setting, NULL, CONFIG_TYPE_GROUP);
		if(server != nullptr)
		{
			printf("> Profile: %s %s:%s.\n", i.name.c_str(), i.ip.c_str(), i.port.c_str());
			element = config_setting_add(server, "ip", CONFIG_TYPE_STRING);
			config_setting_set_string(element, i.ip.c_str());
			element = config_setting_add(server, "port", CONFIG_TYPE_STRING);
			config_setting_set_string(element, i.port.c_str());
			element = config_setting_add(server, "name", CONFIG_TYPE_STRING);
			config_setting_set_string(element, i.name.c_str());
		}
	}
	setting = config_setting_get_member(root, "last_using_server");
	if (!setting) setting = config_setting_add(root, "last_using_server", CONFIG_TYPE_INT);
	config_setting_set_int(setting, lastUsingServer);

	printf("> Write video config.\n");
	// video
	setting = config_setting_get_member(root, "video_bit_rate");
	if (!setting) setting = config_setting_add(root, "video_bit_rate", CONFIG_TYPE_INT);
	config_setting_set_int(setting, videoBitRate);

	setting = config_setting_get_member(root, "video_gop");
	if (!setting) setting = config_setting_add(root, "video_gop", CONFIG_TYPE_INT);
	config_setting_set_int(setting, videoGOP);

	setting = config_setting_get_member(root, "video_max_b_frames");
	if (!setting) setting = config_setting_add(root, "video_max_b_frames", CONFIG_TYPE_INT);
	config_setting_set_int(setting, videoMaxBFrames);

	printf("> Write audio config.\n");
	// audio
	setting = config_setting_get_member(root, "audio_bit_rate");
	if (!setting) setting = config_setting_add(root, "audio_bit_rate", CONFIG_TYPE_INT);
	config_setting_set_int(setting, audioBitRate);

	// mode
	setting = config_setting_get_member(root, "wait_for_sync");
	if (!setting) setting = config_setting_add(root, "wait_for_sync", CONFIG_TYPE_BOOL);
	config_setting_set_bool(setting, waitForSync);

	setting = config_setting_get_member(root, "version");
	if (!setting) setting = config_setting_add(root, "version", CONFIG_TYPE_INT);
	config_setting_set_int(setting, FORCE_OVERRIDE_VERSION);

	// write file
	int ret = config_write_file(&_config, CONFIG_FILE_NAME);
	printf("> Save result: %d\n", ret);
}

void ConfigManager::Destroy()
{
	config_destroy(&_config);
}
