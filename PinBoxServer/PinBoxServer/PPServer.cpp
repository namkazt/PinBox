#include "stdafx.h"
#include "PPServer.h"
#include "ServerConfig.h"


void PPServer::PrintIPAddressList()
{
	// Get local host name
	char szHostName[128] = "";

	if (::gethostname(szHostName, sizeof(szHostName)))
	{
		// Error handling -> call 'WSAGetLastError()'
	}
	// Get local IP addresses
	struct sockaddr_in SocketAddress;
	struct hostent* pHost = 0;
	pHost = ::gethostbyname(szHostName);
	if (!pHost)
	{
		// Error handling -> call 'WSAGetLastError()'
	}
	char aszIPAddresses[10][16]; // maximum of ten IP addresses
	for (int iCnt = 0; ((pHost->h_addr_list[iCnt]) && (iCnt < 10)); ++iCnt)
	{
		memcpy(&SocketAddress.sin_addr, pHost->h_addr_list[iCnt], pHost->h_length);
		strcpy(aszIPAddresses[iCnt], inet_ntoa(SocketAddress.sin_addr));

		std::cout << "IP: " << aszIPAddresses[iCnt] << std::endl << std::flush;
	}
}



void PPServer::InitServer()
{
	google::InitGoogleLogging("PinBoxServer");
	google::SetCommandLineOption("GLOG_minloglevel", "2");
	//===========================================================================
	int cfgPort = 1234;
	int cfgThreadNum = 4;
	//===========================================================================
	// Init config
	//===========================================================================
	ServerConfig::Get();
	//===========================================================================
	// Screen capture session
	//===========================================================================
	ScreenCapturer = new ScreenCaptureSession();
	ScreenCapturer->initSCreenCapturer(this);
	//===========================================================================
	// Screen capture session
	//===========================================================================
	InputStreamer = new InputStreamSession();
	//===========================================================================
	// Socket server part
	//===========================================================================
	std::string addr = "0.0.0.0:" + std::to_string(cfgPort);
	std::cout << "Running on address: " << addr << std::endl << std::flush;
	evpp::EventLoop loop;
	evpp::TCPServer server(&loop, addr, "PinBoxServer", cfgThreadNum);
	//===========================================================================
	server.SetMessageCallback([&](const evpp::TCPConnPtr& conn, evpp::Buffer* msg)
	{
		mutexCloseServer.lock();
		auto iter = clientSessions.find(conn->remote_addr());
		if(iter != clientSessions.end())
		{
			PPClientSession* session = iter->second;
			mutexCloseServer.unlock();
			session->ProcessMessage(msg);
		}else
		{
			mutexCloseServer.unlock();
			std::cout << "[Error]Received Message but do not know from where ??" << std::endl << std::flush;
		}
		
		//--------------------------------------
		// need reset to empty msg after using.
		msg->Reset();
	});
	//===========================================================================
	server.SetConnectionCallback([&](const evpp::TCPConnPtr& conn)
	{
		std::string addrID = conn->remote_addr();
		std::map<std::string, PPClientSession*>::iterator iter = clientSessions.find(addrID);
		if (iter != clientSessions.end())
		{
			// remove session
			if (!conn->IsConnected())
			{
				ScreenCapturer->registerForStopStream([=]()
				{
					ScreenCapturer->stopStream();
					for(auto it = clientSessions.begin(); it != clientSessions.end();++it)
					{
						it->second->DisconnectFromServer();
						std::cout << "Client: " << addrID << " disconnected!" << std::endl << std::flush;
					}
					clientSessions.clear();
					ScreenCapturer->removeForStopStream();
				});
			}
		}
		else
		{
			// add new session
			if (conn->IsConnected())
			{
				std::cout << "Client: " << addrID << " connected to server!" << std::endl << std::flush;
				PPClientSession* session = new PPClientSession();
				session->InitSession(conn, this);
				mutexCloseServer.lock();
				clientSessions.insert(std::pair<std::string, PPClientSession*>(addrID, session));
				mutexCloseServer.unlock();
			}
		}
	});
	server.Init();
	server.Start();
	//===========================================================================
	// print screen
	//===========================================================================
	std::cout << "Init Server : Successfully" << std::endl << std::flush;
	std::cout << "-------------------------------------------" << std::endl << std::flush;
	std::cout << "Please use one of those IP in your 3DS client to connect to server:" << std::endl << std::flush;
	std::cout << "(normally it should be the last one)" << std::endl << std::flush;
	std::cout << "-------------------------------------------" << std::endl << std::flush;
	PrintIPAddressList();
	std::cout << "-------------------------------------------" << std::endl << std::flush;
	std::cout << "Wait for connection..." << std::endl << std::flush;
	//===========================================================================
	loop.Run();
}
