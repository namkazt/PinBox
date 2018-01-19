#pragma once

#ifndef _PP_SERVER_H__
#define _PP_SERVER_H__

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
#include "InputStreamSession.h"

class PPServer
{

	
public:
	static void PrintIPAddressList();
public:
	ScreenCaptureSession*							ScreenCapturer;
	InputStreamSession*								InputStreamer;

	void InitServer();
};


#endif