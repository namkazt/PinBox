#pragma once
#ifndef _PP_NETWORK_H_
#define _PP_NETWORK_H_

#include <3ds.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <fcntl.h>
#include <memory>
#include <cerrno>
#include <functional>
#include "PPMessage.h"
#include <queue>

enum ppConectState { IDLE, CONNECTING, CONNECTED, FAIL };
typedef std::function<void(u8* buffer, u32 size, u32 tag)> PPNetworkReceivedRequest;
typedef std::function<void(u8* data, u32 code)> PPNetworkCallback;
class PPSession;
typedef struct
{
	void		*msgBuffer;
	u32			msgSize;
} QueueMessage;

class PPNetwork
{
private:
	// socket
	const char*					g_ip = 0;
	const char*					g_port = 0;
	u32							g_sock = -1;
	ppConectState				g_connect_state = IDLE;
	// in-out variables
	u32							g_tag = 0;
	u32							g_waitForSize = 0;
	u32							g_receivedCounter = 0;
	u8*							g_receivedBuffer = nullptr;
	u8*							g_tmpReceivedBuffer = nullptr;
	u32							g_tmpReceivedSize = 0;
	// thread
	Thread						g_thread = nullptr;
	volatile bool				g_threadExit = false;
	std::queue<QueueMessage*>	g_sendingMessage;
	// callback
	PPNetworkReceivedRequest	g_onReceivedRequest = nullptr;
	PPNetworkCallback			g_onConnectionSuccessed = nullptr;
	PPNetworkCallback			g_onConnectionClosed = nullptr;

private:
	static void ppNetwork_threadRun(void * arg);
	void ppNetwork_listenToServer();
	void ppNetwork_connectToServer();
	void ppNetwork_closeConnection();
	void ppNetwork_onReceivedRequet();
	bool ppNetwork_processTmpBufferData();
	void ppNetwork_sendMessage();

public:
	~PPNetwork();
	PPSession*					g_session;

	void Start(const char *ip, const char *port);
	void Stop();
	ppConectState GetConnectionStatus() const { return g_connect_state; }
	void SetRequestData(u32 size, u32 tag = 0);
	void SendMessageData(u8 *msgBuffer, int32_t msgSize);

	inline void SetOnReceivedRequest(PPNetworkReceivedRequest _callback) { g_onReceivedRequest = _callback; }
	inline void SetOnConnectionSuccessed(PPNetworkCallback _callback) { g_onConnectionSuccessed = _callback; }
	inline void SetOnConnectionClosed(PPNetworkCallback _callback) { g_onConnectionClosed = _callback; }
};

#endif