#include "stdafx.h"
#include "ServerConfig.h"
#include <ScreenCapture.h>
#include <fstream>
#include <sstream>


void genRandom(char *s, const int len) {
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	s[len] = 0;
}


ServerConfig::ServerConfig()
{
	LoadConfig();
	LoadHubItems();
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

void ServerConfig::LoadHubItems()
{
	libconfig::Config configFile;
	try
	{
		configFile.readFile("hub.cfg");
	}
	catch (const libconfig::FileIOException& fioex)
	{
		std::cout << "[Error] Hub config file was not found." << std::endl;
	}
	catch (const libconfig::ParseException& pex)
	{
		std::cout << "[Error] Hub config file corrupted." << std::endl;
	}

	auto monitors = SL::Screen_Capture::GetMonitors();
	for(auto mon : monitors)
	{
		HubItem *hubItem = new HubItem();
		std::ostringstream stringStream;
		stringStream << "Monitor " << mon.Index;
		hubItem->name = stringStream.str();

		char ranUUID[100];
		genRandom(ranUUID, 16);
		hubItem->uuid = ranUUID;

		hubItem->type = HUB_SCREEN;
		HubItems.push_back(hubItem);
	}


	const libconfig::Setting &root = configFile.getRoot();
	const libconfig::Setting &hub = root["hub"];
	int count = hub.getLength();
	for(int i = 0; i < count; ++i)
	{
		const libconfig::Setting &item = hub[i];
		HubItem *hubItem = new HubItem();

		if(!(item.lookupValue("name", hubItem->name) && 
			item.lookupValue("uuid", hubItem->uuid) &&
			item.lookupValue("thumbImage", hubItem->thumbImage) &&
			item.lookupValue("exePath", hubItem->exePath) &&
			item.lookupValue("processName", hubItem->processName)))
			continue;

		// load image to buf
		std::ifstream thumbFile("tmp\\" + hubItem->thumbImage, std::ifstream::binary);
		if(!thumbFile)
		{
			std::cout << "[Error] thumbnail image was not found." << std::endl;
		}else
		{
			// get file size
			thumbFile.seekg(0, thumbFile.end);
			uint32_t length = thumbFile.tellg();
			thumbFile.seekg(0, thumbFile.beg);

			hubItem->thumbSize = length;
			hubItem->thumbBuf = (u8*)malloc(length);
			thumbFile.read((char*)hubItem->thumbBuf, length);
			thumbFile.close();
		}

		hubItem->type = HUB_APP;
		HubItems.push_back(hubItem);
	}

}


