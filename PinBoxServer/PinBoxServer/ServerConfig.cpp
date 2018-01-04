#include "stdafx.h"
#include "ServerConfig.h"



ServerConfig::ServerConfig()
{
	LoadConfig();
}


ServerConfig::~ServerConfig()
{
}

static ServerConfig* mInstance = nullptr;
ServerConfig* ServerConfig::Get()
{
	if(mInstance == nullptr)
	{
		mInstance = new ServerConfig();
	}
	return mInstance;
}

void ServerConfig::LoadConfig()
{
	//-----------------------------------------------
	// load config
	//-----------------------------------------------
	libconfig::Config configFile;
	try
	{
		configFile.readFile("server.cfg");
	}
	catch (const libconfig::FileIOException& fioex)
	{
		std::cout << "[Error] Server config file was not found." << std::endl << std::flush;
	}
	catch (const libconfig::ParseException& pex)
	{
		std::cout << "[Error] Server config file corrupted." << std::endl << std::flush;
	}
	const libconfig::Setting& root = configFile.getRoot();

	int monitor = root.lookup("monitor_index");
	MonitorIndex = monitor;
}


