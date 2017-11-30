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

class PPServer
{
private:
	std::map<std::string, PPClientSession*> clientSessions;

public:
	static void PrintIPAddressList();


public:
	void InitServer();
};

