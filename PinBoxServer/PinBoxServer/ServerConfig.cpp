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
	std::cout << "=========== SERVER CONFIG ===================" << std::endl << std::flush;
	int monitor = root.lookup("monitor_index");
	MonitorIndex = monitor;
	std::cout << " Monitor Index: " << MonitorIndex << std::endl << std::flush;
	int captureFPS = root.lookup("capture_fps");
	CaptureFPS = captureFPS;
	if (CaptureFPS < 0) CaptureFPS = 40;
	std::cout << " FPS: " << CaptureFPS << std::endl << std::flush;

	std::cout << "=============================================" << std::endl << std::flush;
}


