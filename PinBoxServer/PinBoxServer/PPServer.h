#pragma once

// socket
#include <winsock2.h>
#include <winsock.h>
#include <ostream>
#include <iostream>

// evpp
#include <glog/config.h>
#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

#include "PPClientSession.h"
#include "ScreenCaptureSession.h"

class PPServer
{

	
public:
	std::map<std::string, PPClientSession*>			clientSessions;
	u32												numberClients = 0;
	std::mutex										mutexCloseServer;
	static void PrintIPAddressList();
public:
	ScreenCaptureSession*							ScreenCapturer;

	void InitServer();
};

