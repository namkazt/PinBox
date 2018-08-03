#include "PPNetwork.h"
#include <iostream>
#include "PPSession.h"

/*
 * @brief: thread handler function
 */
void PPNetwork::ppNetwork_threadRun(void * arg)
{
	PPNetwork* network = (PPNetwork*)arg;
	if (network == nullptr) return;
	//--------------------------------------------------
	uint64_t sleepDuration = 1000000ULL * 1;
	while(!network->g_threadExit){
		switch (network->g_connect_state)
		{
			case IDLE:
			{
				//--------------------------------------------------
				// try to connect if not connected to server yet
				network->ppNetwork_connectToServer();
				break;
			}
			case CONNECTING:
			{
				// connecting state
				break;
			}
			case CONNECTED:
			{
				//--------------------------------------------------
				//send queue message
				network->ppNetwork_sendMessage();

				//--------------------------------------------------
				// listen to server when it connected successfully
				network->ppNetwork_listenToServer();
				
				break;
			}
			case FAIL:
			{
				//--------------------------------------------------
				// Exit thread when connection fail
				network->g_threadExit = true;
				break;
			}
			default:
			{
				break;
			}
		}
#ifndef _WIN32
		svcSleepThread(sleepDuration);
#else
		std::this_thread::sleep_for(std::chrono::nanoseconds(sleepDuration));
#endif
	}
	//--------------------------------------------------
	// close connection if thread is exit
	network->ppNetwork_closeConnection();
}

void PPNetwork::ppNetwork_sendMessage()
{
	//TODO: is it thread safe ? should we add mutex on this ?
	if (this->g_sendingMessage.size() > 0)
	{
		QueueMessage* queueMsg = (QueueMessage*)this->g_sendingMessage.front();
		this->g_sendingMessage.pop();
		//--------------------------------------------------
		if (queueMsg->msgSize > 0 && queueMsg->msgBuffer != nullptr)
		{
			uint32_t totalSent = 0;
			//--------------------------------------------------------
			// send message
			do
			{
				int sendAmount = send(this->g_sock, (char*)queueMsg->msgBuffer, queueMsg->msgSize, 0);
				if (sendAmount < 0)
				{
					// ERROR when send message
					return;
				}
				totalSent += sendAmount;
			} while (totalSent < queueMsg->msgSize);
			//std::cout << "[Queue]Session: #" << g_session->sessionID << " send message done! " << std::endl;
			//--------------------------------------------------------
			// free message
			free(queueMsg->msgBuffer);
			delete queueMsg;
		}
	}
}

void PPNetwork::ppNetwork_connectToServer()
{
	g_connect_state = CONNECTING;
	//--------------------------------------------------
	// Trying to get address information
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	int rv = getaddrinfo(g_ip, g_port, &hints, &servinfo);
	if(rv != 0)
	{
		printf("Can't get address information.\n");
		// Error: fail to get address
		g_connect_state = FAIL;
		WSACleanup();
		return;
	}
	//--------------------------------------------------
	// define socket
	g_sock = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	if (g_sock == -1)
	{
		printf("Can't create new socket.\n");
		// Error: can't create socket
		g_connect_state = FAIL;
		freeaddrinfo(servinfo);
		WSACleanup();
		return;
	}

	//--------------------------------------------------
	// loop thought all result and connect
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		//--------------------------------------------------
		// try to connect to server
		auto ret = connect(g_sock, p->ai_addr, p->ai_addrlen);
		if (ret == -1)
		{
			// Error: can't connect to this ai -> try next
			g_connect_state = FAIL;
			continue;
		}

		//--------------------------------------------------
		// if it run here so it already connecteds
		g_connect_state = CONNECTED;

		break;
	}

	//--------------------------------------------------
	// Note: check if connected
	if (g_connect_state == CONNECTED)
	{
		//--------------------------------------------------
		// set socket to non blocking so we can easy control it
		//fcntl(sockManager->sock, F_SETFL, O_NONBLOCK);
#ifndef _WIN32
		fcntl(g_sock, F_SETFL, fcntl(g_sock, F_GETFL, 0) | O_NONBLOCK);
#else
		unsigned long on = 1;
		ioctlsocket(g_sock, FIONBIO, &on);
#endif
		//--------------------------------------------------
		// callback when connected to server
		if (g_onConnectionSuccessed != nullptr) g_onConnectionSuccessed(nullptr, 1);
		printf("Connected to server.\n");
	}else
	{
		printf("Could not connect to server.\n");
		freeaddrinfo(servinfo);
		WSACleanup();
	}
}

void PPNetwork::ppNetwork_onReceivedRequet()
{
	//--------------------------------------------------
	// NOTE: callback step
	//--------------------------------------------------
	// we pass a copy version received buffer to where it need to process
	u8 *resultBuffer = (u8*)malloc(g_waitForSize);
	memcpy(resultBuffer, g_receivedBuffer, g_waitForSize);
	int32_t tmpSize = g_waitForSize;
	int32_t tmpTag = g_tag;
	//--------------------------------------------------
	// free this data and point to nullptr
	free(g_receivedBuffer);
	g_receivedBuffer = nullptr;
	g_waitForSize = 0;
	g_receivedCounter = 0;
	g_tag = 0;
	//--------------------------------------------------
	// this buffer need to be free whenever it finish it's job
	if(g_onReceivedRequest != nullptr)
		g_onReceivedRequest(resultBuffer, tmpSize, tmpTag);
	
}

/*
 * @brief: process current tmp buffer data if it have
 */
bool PPNetwork::ppNetwork_processTmpBufferData()
{
	if (g_tmpReceivedBuffer != nullptr)
	{
		//--------------------------------------------------
		// if this data is more than current request data
		int32_t pieceSize = g_tmpReceivedSize;
		int32_t dataLeft = 0;
		bool isFullyReceived = false;
		if(g_tmpReceivedSize >= g_waitForSize)
		{
			isFullyReceived = true;
			pieceSize = g_waitForSize;
			dataLeft = g_tmpReceivedSize - g_waitForSize;
		}
		//--------------------------------------------------
		// copy data into current received buffer
		if (!g_receivedBuffer)
			g_receivedBuffer = (u8*)malloc(g_waitForSize);
		if (!g_receivedBuffer) return false;
		memcpy(g_receivedBuffer + g_receivedCounter, g_tmpReceivedBuffer, pieceSize);
		g_receivedCounter += pieceSize;

		if(isFullyReceived)
		{
			//--------------------------------------------------
			// we finished current request
			ppNetwork_onReceivedRequet();
			//--------------------------------------------------
			// if there is still have data left
			if (dataLeft > 0)
			{
				//--------------------------------------------------
				// cut off received part and create new data
				u8* tmpBuffer = (u8*)malloc(dataLeft);
				memcpy(tmpBuffer, g_tmpReceivedBuffer + pieceSize, dataLeft);
				g_tmpReceivedSize = dataLeft;
				free(g_tmpReceivedBuffer);
				g_tmpReceivedBuffer = tmpBuffer;

				// return false to stop current listen process
				return false;
			}
		}
		free(g_tmpReceivedBuffer);
		g_tmpReceivedBuffer = nullptr;
		g_tmpReceivedSize = 0;
	}
	return true;
}


void PPNetwork::ppNetwork_listenToServer()
{
	//--------------------------------------------------
	// If there is no order to receive any data so we just 
	// continue our threads
	if (g_waitForSize <= 0)
	{
		return;
	}

	//--------------------------------------------------
	// if tmp data is not null so that mean we need add this datao
	// into current request data
	// example case: 2 messages data send together from server
	//---------------------------------------------------
	// stop process if there is have tmp buffer data and it fit
	// request message
	if (!ppNetwork_processTmpBufferData()) return;

	//--------------------------------------------------
	// Recevie data from server 
	const int bufferSize = 1024 * 24; // 24Kb buffer size
	u8* recvBuffer = (u8*)malloc(bufferSize);
	uint32_t recvAmount = -1;
	recvAmount = recv(g_sock, (char*)recvBuffer, bufferSize, 0);
	//--------------------------------------------------
	// exit thread when have connecting problem
	if (recvAmount <= 0) {
#ifndef _WIN32
		if (errno != EWOULDBLOCK) g_threadExit = true;
#else
		if (errno != WSAEWOULDBLOCK) g_threadExit = true;
#endif
		//--------------------------------------------------
		// free data
		free(recvBuffer);
		return;
	}else if(recvAmount > bufferSize)
	{
		//--------------------------------------------------
		// free data
		free(recvBuffer);
		return;
	}else
	{
		//std::cout << "Client: " << g_session->sessionID << " recv size: " << recvAmount << std::endl;
		//--------------------------------------------------
		if(g_receivedBuffer == nullptr)
			g_receivedBuffer = (u8*)malloc(g_waitForSize);
		//--------------------------------------------------
		int32_t pieceSize = recvAmount;
		int32_t dataLeft = 0;
		bool isFullyReceived = false;
		//--------------------------------------------------
		// check if data is fully recevied
		if(g_receivedCounter + recvAmount >= g_waitForSize) {
			pieceSize = g_waitForSize - g_receivedCounter;
			dataLeft = recvAmount - pieceSize;
			isFullyReceived = true;
		}
		//--------------------------------------------------
		// copy data to final buffer
		memcpy(g_receivedBuffer + g_receivedCounter, recvBuffer, pieceSize);
		g_receivedCounter += pieceSize;
		//--------------------------------------------------
		if (isFullyReceived) {
			//--------------------------------------------------
			// set received request and free g_receivedBuffer
			ppNetwork_onReceivedRequet();
			//--------------------------------------------------
			// store data left into temp buffer
			if (dataLeft > 0)
			{
				//---------------------------------------------------
				// free tmp buffer if it not null 
				//NOTE: is it possible if going to this part without being nullptr ?
				if (g_tmpReceivedBuffer != nullptr) {
					free(g_tmpReceivedBuffer);
					g_tmpReceivedBuffer = nullptr;
				}
				g_tmpReceivedBuffer = (u8*)malloc(dataLeft);
				memcpy(g_tmpReceivedBuffer, recvBuffer + pieceSize, dataLeft);
				g_tmpReceivedSize = dataLeft;
			}
		}
	}

	//--------------------------------------------------
	// free data
	free(recvBuffer);
}

void PPNetwork::ppNetwork_closeConnection()
{
	if(g_connect_state == CONNECTED)
	{
		if (g_sock == -1) return;
		int rc = closesocket(g_sock);
		if(rc != 0)
		{
			// Error : ? can't close socket
		}
		g_sock = -1;
		g_connect_state = IDLE;
	}
	//--------------------------------------------------
	//clear all buffer data if need
	if (g_receivedBuffer) {
		free(g_receivedBuffer);
		g_receivedBuffer = nullptr;
		g_waitForSize = 0;
		g_receivedCounter = 0;
		g_tag = 0;
	}
	if(g_tmpReceivedBuffer)
	{
		free(g_tmpReceivedBuffer);
		g_tmpReceivedBuffer = nullptr;
		g_tmpReceivedSize = 0;
	}
	//--------------------------------------------------
	//TODO: clear all message sending buffer
	// for each g_sendingMessage -> delete msg
	//--------------------------------------------------
	// callback when connection closed
	if(g_onConnectionClosed != nullptr)
	{
		g_onConnectionClosed(nullptr, -1);
	}
}


//===============================================================================================
// Controller functions
//===============================================================================================
#define STACKSIZE (30 * 1024)
void PPNetwork::Start(const char* ip, const char* port)
{
	if (g_connect_state == IDLE) {
		printf("Start connect to server...\n");
		g_sendingMessage = std::queue<QueueMessage*>();
		//---------------------------------------------------
		// init variables
#ifndef _WIN32
		g_ip = strdup(ip);
		g_port = strdup(port);
#else
		g_ip = _strdup(ip);
		g_port = _strdup(port);
#endif
		g_threadExit = false;
		//---------------------------------------------------
		// start thread
#ifndef _WIN32
		int32_t prio = 0;
		svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
		g_thread = threadCreate(ppNetwork_threadRun, this, STACKSIZE, prio - 1, -2, true);
#else
		// after create we detach thread to make it continues without blocking and no need to call join.
		g_thread = std::thread(ppNetwork_threadRun, this);
		g_thread.detach();
#endif
	}else
	{
		// log -> already start network connection
	}
}

void PPNetwork::Stop()
{
	g_threadExit = true;
}

void PPNetwork::SetRequestData(int32_t size, int32_t tag)
{
	if(g_waitForSize > 0)
	{
		//std::cout << "Client: " << g_session->sessionID << " send wrong request data. Request for wait still have value: " << g_waitForSize << std::endl;
	}else
	{
		//std::cout << "Client: " << g_session->sessionID << " register wait for size: " << size << " with tag: " << tag << std::endl;
		g_waitForSize = size;
		g_tag = tag;
	}
}

//----------------------------------------------------------
// NOTE: this function will normally run on main thread
//----------------------------------------------------------
void PPNetwork::SendMessageData(u8 *msgBuffer, int32_t msgSize)
{
	if (g_connect_state == CONNECTED) {
		QueueMessage* msg = new QueueMessage();
		msg->msgBuffer = msgBuffer;
		msg->msgSize = msgSize;
		g_sendingMessage.push(msg);
	}else
	{
		std::cout << "Session: #" << g_session->sessionID << " send message when not connected" << std::endl;
	}
}

PPNetwork::~PPNetwork()
{
	free(&g_ip);
	free(&g_port);
	if (g_receivedBuffer != nullptr) free(g_receivedBuffer);
	if (g_tmpReceivedBuffer != nullptr) free(g_tmpReceivedBuffer);
}
