#include "stdafx.h"
#include "PPServer.h"



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

		std::cout << "IP: " << aszIPAddresses[iCnt] << std::endl;
	}
}



void PPServer::InitServer()
{
	google::InitGoogleLogging("PinBoxServer");
	google::SetCommandLineOption("GLOG_minloglevel", "2");
	//===========================================================================
	int cfgPort = 1234;
	int cfgMonitorIndex = -1;
	int cfgThreadNum = 8;
	//===========================================================================
	// Screen capture session
	//===========================================================================
	ScreenCapturer = new ScreenCaptureSession();
	ScreenCapturer->initSCreenCapturer(this);
	//===========================================================================
	// Socket server part
	//===========================================================================
	std::string addr = "0.0.0.0:" + std::to_string(cfgPort);
	std::cout << "Running on address: " << addr << std::endl;
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
			std::cout << "[Error]Received Message but do not know from where ??" << std::endl;
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
						std::cout << "Client: " << addrID << " disconnected!" << std::endl;
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
				std::cout << "Client: " << addrID << " connected to server!" << std::endl;
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
	std::cout << "Init Server : Successfully" << std::endl;
	std::cout << "-------------------------------------------" << std::endl;
	std::cout << "Please use one of those IP in your 3DS client to connect to server:" << std::endl;
	std::cout << "(normally it should be the last one)" << std::endl;
	std::cout << "-------------------------------------------" << std::endl;
	PrintIPAddressList();
	std::cout << "-------------------------------------------" << std::endl;
	std::cout << "Wait for connection..." << std::endl;
	//===========================================================================
	loop.Run();
}
