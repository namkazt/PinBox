#include "stdafx.h"
#include "PPServer.h"
#include "ServerConfig.h"
#include <string>

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
	// Init config
	//===========================================================================
	int cfgPort = ServerConfig::Get()->ServerPort;
	int cfgThreadNum = ServerConfig::Get()->NetworkThread;
	ScreenCapturer = new ScreenCaptureSession();
	ScreenCapturer->setParent(this);
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
		PPClientSession* session = evpp::any_cast<PPClientSession*>(conn->context());
		if (session != nullptr) session->ProcessMessage(msg);
		msg->Reset();
	});


	//===========================================================================
	server.SetConnectionCallback([&](const evpp::TCPConnPtr& conn)
	{
		std::string addrID = conn->remote_addr();
		if(conn->IsConnected())
		{
			// new client is connected
			std::cout << "Client: " << addrID << " connected to server!" << std::endl << std::flush;
			PPClientSession* session = new PPClientSession();
			session->InitSession(conn, this);
			evpp::Any context(session);
			conn->set_context(context);

		}else {
			// client is disconnected
			evpp::Any sessionAny = conn->context();
			PPClientSession* session = evpp::any_cast<PPClientSession*>(sessionAny);
			if(session != nullptr)
			{

				std::cout << "Client: " << addrID << " disconnected to server!" << std::endl << std::flush;
				session->DisconnectFromServer();
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
