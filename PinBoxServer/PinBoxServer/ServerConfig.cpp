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
		std::cout << "[Error] Server config file was not found." << std::endl;
	}
	catch (const libconfig::ParseException& pex)
	{
		std::cout << "[Error] Server config file corrupted." << std::endl;
	}
	const libconfig::Setting& root = configFile.getRoot();
	std::cout << "=========== SERVER CONFIG ===================" << std::endl;
	int monitor = root.lookup("monitor_index");
	MonitorIndex = monitor;
	std::cout << " Monitor Index: " << MonitorIndex << std::endl;

	int captureFPS = root.lookup("capture_fps");
	CaptureFPS = captureFPS;
	if (CaptureFPS <= 0) CaptureFPS = 30;
	std::cout << " FPS: " << CaptureFPS << std::endl;

	int networkThreads = root.lookup("network_threads");
	NetworkThread = networkThreads;
	if (NetworkThread <= 0) NetworkThread = 2;
	std::cout << " Network Threads: " << NetworkThread << std::endl;

	int serverPort = root.lookup("server_port");
	ServerPort = serverPort;
	if (ServerPort <= 0) ServerPort = 1234;
	std::cout << " Server Port: " << ServerPort << std::endl;

	std::cout << "=============================================" << std::endl << std::flush;
}


