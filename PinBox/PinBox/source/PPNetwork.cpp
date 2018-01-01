#include "PPNetwork.h"
#include "PPSession.h"

/*
 * @brief: thread handler function
 */
void PPNetwork::ppNetwork_threadRun(void * arg)
{
	PPNetwork* network = (PPNetwork*)arg;
	if (network == nullptr) return;
	//--------------------------------------------------
	u64 sleepDuration = 1000000ULL * 16;
	while(!network->g_threadExit){
		
		printf("#dwa Thread run");

		if(network->g_connect_state == ppConectState::IDLE)
		{
			//--------------------------------------------------
			// try to connect if not connected to server yet
			network->ppNetwork_connectToServer();
		}

		if (network->g_connect_state == ppConectState::CONNECTED)
		{
			//--------------------------------------------------
			//send queue message
			network->ppNetwork_sendMessage();

			//--------------------------------------------------
			// listen to server when it connected successfully
			network->ppNetwork_listenToServer();
		}

		if (network->g_connect_state == ppConectState::FAIL)
		{
			//--------------------------------------------------
			// Exit thread when connection fail
			network->g_threadExit = true;
			printf("#%d : Connection failed -> exit thread.\n", network->g_session->sessionID);
		}

		svcSleepThread(sleepDuration);
	}
	//--------------------------------------------------
	// close connection if thread is exit
	network->ppNetwork_closeConnection();
}

void PPNetwork::ppNetwork_sendMessage()
{
	g_queueMessageMutex->Lock();
	if (this->g_sendingMessage.size() > 0)
	{
		printf("#%d : Check for message.\n", this->g_session->sessionID);
		QueueMessage* queueMsg = (QueueMessage*)this->g_sendingMessage.front();
		this->g_sendingMessage.pop();
		//--------------------------------------------------
		if (queueMsg->msgSize > 0 && queueMsg->msgBuffer != nullptr)
		{
			u32 totalSent = 0;
			//--------------------------------------------------------
			// send message
			do
			{
				printf("#%d : Send message.\n", this->g_session->sessionID);
				int sendAmount = send(this->g_sock, queueMsg->msgBuffer, queueMsg->msgSize, 0);
				if (sendAmount < 0)
				{
					// ERROR when send message
					return;
				}
				totalSent += sendAmount;
			} while (totalSent < queueMsg->msgSize);
			//--------------------------------------------------------
			// free message
			free(queueMsg->msgBuffer);
			delete queueMsg;
		}
	}
	g_queueMessageMutex->Unlock();
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
		printf("Can't get address information: %s\n", gai_strerror(rv));
		// Error: fail to get address
		g_connect_state = FAIL;
		//continue;
		return;
	}

	//--------------------------------------------------
	// define socket
	g_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_sock == -1)
	{
		printf("Can't create new socket.\n");
		// Error: can't create socket
		g_connect_state = FAIL;
		//continue;
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
	freeaddrinfo(servinfo);


	//struct sockaddr_in addr = { 0 };
	//addr.sin_family = AF_INET;
	//unsigned short nPort = (unsigned short)strtoul(g_port, NULL, 0);
	//addr.sin_port = htons(nPort);
	//inet_pton(addr.sin_family, g_ip, &addr.sin_addr);
	//auto ret = connect(g_sock, (struct sockaddr *) &addr, sizeof(addr));
	//if (ret == -1)
	//{
	//	// Error: can't connect to this ai -> try next
	//	g_connect_state = FAIL;
	//}
	//else
	//{
	//	g_connect_state = CONNECTED;
	//}


	//--------------------------------------------------
	// Note: check if connected
	if (g_connect_state == CONNECTED)
	{
		//--------------------------------------------------
		// set socket to non blocking so we can easy control it
		//fcntl(sockManager->sock, F_SETFL, O_NONBLOCK);
		fcntl(g_sock, F_SETFL, fcntl(g_sock, F_GETFL, 0) | O_NONBLOCK);
		printf("#%d : Connected to server.\n", g_session->sessionID);
		//--------------------------------------------------
		// callback when connected to server
		if (g_onConnectionSuccessed != nullptr)
		{
			g_onConnectionSuccessed(this, nullptr, 1);
		}
	}else
	{
		printf("#%d :Could not connect to server.\n", g_session->sessionID);
	}
}

void PPNetwork::ppNetwork_onReceivedRequest()
{
	//--------------------------------------------------
	// NOTE: callback step
	//--------------------------------------------------
	// we pass a copy version received buffer to where it need to process
	u8 *resultBuffer =(u8*) malloc(g_waitForSize);
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
	if (g_onReceivedRequest != nullptr)
		g_onReceivedRequest(this, resultBuffer, tmpSize, tmpTag);
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
		u32 pieceSize = g_tmpReceivedSize;
		u32 dataLeft = 0;
		bool isFullyReceived = false;
		if (g_tmpReceivedSize >= g_waitForSize)
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

		if (isFullyReceived)
		{
			//--------------------------------------------------
			// we finished current request
			ppNetwork_onReceivedRequest();
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
		//printf("#%d : no request wait for size request.\n", g_session->sessionID);
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

	printf("#%d : start recv data.\n", g_session->sessionID);
	//--------------------------------------------------
	// Recevie data from server 
	const u32 bufferSize = 1024 * 10; 
	u8* recvBuffer = (u8*)malloc(bufferSize);
	int recvAmount = recv(g_sock, recvBuffer, bufferSize, 0);
	//--------------------------------------------------
	// exit thread when have connecting problem
	if (recvAmount < 0) {
		if (errno != EWOULDBLOCK) 
			g_threadExit = true;
		//--------------------------------------------------
		// free data
		free(recvBuffer);
		return;
	}
	else if (recvAmount > bufferSize)
	{
		printf("#%d : recv too much: %d.\n", g_session->sessionID, recvAmount);
		//--------------------------------------------------
		// free data
		free(recvBuffer);
		return;
	}else
	{
		printf("#%d : process recv data.\n", g_session->sessionID);
		//--------------------------------------------------
		if(g_receivedBuffer == nullptr)
			g_receivedBuffer = (u8*)malloc(g_waitForSize);
		//--------------------------------------------------
		u32 pieceSize = recvAmount;
		u32 dataLeft = 0;
		bool isFullyReceived = false;
		//--------------------------------------------------
		// check if data is fully recevied
		if (g_receivedCounter + recvAmount >= g_waitForSize) {
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
			ppNetwork_onReceivedRequest();
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
		Result rc = closesocket(g_sock);
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
	// clear all message sending buffer
	//--------------------------------------------------
	while (g_sendingMessage.size() > 0)
	{
		QueueMessage* queueMsg = (QueueMessage*)g_sendingMessage.front();
		g_sendingMessage.pop();
		//--------------------------------------------------------
		// free message
		free(queueMsg->msgBuffer);
		delete queueMsg;
	}
	std::queue<QueueMessage*>().swap(g_sendingMessage);
	delete g_queueMessageMutex;
	//--------------------------------------------------
	// callback when connection closed
	if (g_onConnectionClosed != nullptr)
	{
		g_onConnectionClosed(this, nullptr, -1);
	}
}


//===============================================================================================
// Controller functions
//===============================================================================================
#define STACKSIZE (3 * 1024 * 1024)
void PPNetwork::Start(const char* ip, const char* port, s32 prio)
{
	if (g_connect_state == IDLE) {
		printf("Start connect to server...\n");
		g_queueMessageMutex = new Mutex();
		g_sendingMessage = std::queue<QueueMessage*>();
		//---------------------------------------------------
		// init variables
		g_ip = strdup(ip);
		g_port = strdup(port);
		g_threadExit = false;
		//---------------------------------------------------
		// start thread
		
		g_thread = threadCreate(ppNetwork_threadRun, this, STACKSIZE, prio - 2, -1, true);
	}else
	{
		// log -> already start network connection
	}
}

void PPNetwork::Stop()
{
	g_threadExit = true;
}

void PPNetwork::SetRequestData(u32 size, u32 tag)
{
	if(g_waitForSize > 0)
	{
		//printf("#%d : Old request data not solved yet! (%d bytes).\n", g_session->sessionID, g_waitForSize);
	}else
	{
		//printf("#%d : new request data tag: %d - (%d bytes).\n", g_session->sessionID, tag, size);
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
		printf("#%d : add mesage to query.\n", g_session->sessionID);
		g_queueMessageMutex->Lock();
		QueueMessage* msg = new QueueMessage();
		msg->msgBuffer = msgBuffer;
		msg->msgSize = msgSize;
		g_sendingMessage.push(msg);
		g_queueMessageMutex->Unlock();
		printf("#%d : added message successfully.\n", g_session->sessionID);
	}else
	{
		printf("#%d : Try to send but not connected yet.\n", g_session->sessionID);
	}
}

PPNetwork::~PPNetwork()
{
	free(&g_ip);
	free(&g_port);
	if (g_receivedBuffer != nullptr) free(g_receivedBuffer);
	if (g_tmpReceivedBuffer != nullptr) free(g_tmpReceivedBuffer);
}
