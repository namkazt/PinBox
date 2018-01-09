#include "PPNetwork.h"
#include "PPSession.h"

#define STACKSIZE (3 * 1024)
#define POOLSIZE (40 * 1024)
#define BUFFERSIZE (10024)

/*
 * @brief: thread handler function
 */

void PPNetwork::ppNetwork_threadRun(void * arg)
{
	PPNetwork* network = (PPNetwork*)arg;
	if (network == nullptr) return;

	network->g_receivedBuffer = (u8*)malloc(POOLSIZE);
	network->g_receivedCounter = 0;
	//--------------------------------------------------
	u64 sleepDuration = 1000000ULL * 30;
	while(!network->g_threadExit){
		
		//printf("Thread run\n");
		//gfxFlushBuffers();

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


			network->ppNetwork_processPoolData();
		}

		if (network->g_connect_state == ppConectState::FAIL)
		{
			//--------------------------------------------------
			// Exit thread when connection fail
			network->g_threadExit = true;
			printf("#%d : Connection failed -> exit thread.\n", network->g_session->sessionID);
			gfxFlushBuffers();
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
		//printf("#%d : Check for message.\n", this->g_session->sessionID);
		//gfxFlushBuffers();
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
				//printf("#%d : Send message.\n", this->g_session->sessionID);
				//gfxFlushBuffers();
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
	// define socket
	g_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_sock == -1)
	{
		printf("Can't create new socket.\n");
		gfxFlushBuffers();
		// Error: can't create socket
		g_connect_state = FAIL;
		//continue;
		return;
	}

	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	unsigned short nPort = (unsigned short)strtoul(g_port, NULL, 0);
	addr.sin_port = htons(nPort);
	inet_pton(addr.sin_family, g_ip, &addr.sin_addr);
	auto ret = connect(g_sock, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1)
	{
		// Error: can't connect to this ai -> try next
		g_connect_state = FAIL;
	}
	else
	{
		g_connect_state = CONNECTED;
	}

	//--------------------------------------------------
	// Note: check if connected
	if (g_connect_state == CONNECTED)
	{
		//--------------------------------------------------
		// set socket to non blocking so we can easy control it
		//fcntl(sockManager->sock, F_SETFL, O_NONBLOCK);
		fcntl(g_sock, F_SETFL, fcntl(g_sock, F_GETFL, 0) | O_NONBLOCK);
		printf("#%d : Connected to server.\n", g_session->sessionID);
		gfxFlushBuffers();
		//--------------------------------------------------
		// callback when connected to server
		if (g_onConnectionSuccessed != nullptr)
		{
			g_onConnectionSuccessed(this, nullptr, 1);
		}
	}else
	{
		printf("#%d :Could not connect to server.\n", g_session->sessionID);
		gfxFlushBuffers();
	}
}

void PPNetwork::ppNetwork_listenToServer()
{
	if (g_receivedCounter + BUFFERSIZE > POOLSIZE) return;

	//--------------------------------------------------
	// Recevie data from server 
	const u32 bufferSize = BUFFERSIZE;
	//u8* recvBuffer = (u8*)malloc(bufferSize);
	int recvAmount = recv(g_sock, g_receivedBuffer + g_receivedCounter, bufferSize, 0);
	//--------------------------------------------------
	// exit thread when have connecting problem
	if (recvAmount <= 0) {
		if (errno != EWOULDBLOCK) 
			g_threadExit = true;
		//--------------------------------------------------
		// free data
		//free(recvBuffer);
		return;
	}
	else if (recvAmount > bufferSize)
	{
		/*printf("#%d : recv too much: %d.\n", g_session->sessionID, recvAmount);
		gfxFlushBuffers();*/
		//--------------------------------------------------
		// free data
		//free(recvBuffer);
		return;
	}else
	{
		g_receivedCounter += recvAmount;

		/*printf("#%d : recv data: %d - pool: %d.\n", g_session->sessionID, recvAmount, g_receivedCounter);
		gfxFlushBuffers();*/
	}

	//--------------------------------------------------
	// free data
	//free(recvBuffer);
}

void PPNetwork::ppNetwork_processPoolData()
{
	if (g_receivedCounter < g_waitForSize || g_waitForSize == 0) return;
	//--------------------------------------------------
	// free this data and point to nullptr
	u32 dataLeft = g_receivedCounter - g_waitForSize;
	//printf("#%d : process pool: %d / %d - data left: %d.\n", g_session->sessionID, g_waitForSize, g_receivedCounter, dataLeft);

	u32 tmpWfz = g_waitForSize;
	u32 tmpTag = g_tag;
	g_waitForSize = 0;
	g_tag = 0;
	//--------------------------------------------------
	// this buffer need to be free whenever it finish it's job
	if (g_onReceivedRequest != nullptr)
		g_onReceivedRequest(this, g_receivedBuffer, tmpWfz, tmpTag);

	// clean
	if (dataLeft > 0)
		memmove(g_receivedBuffer, g_receivedBuffer + tmpWfz, dataLeft);
	g_receivedCounter = dataLeft;
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

void PPNetwork::Start(const char* ip, const char* port, s32 prio)
{
	if (g_connect_state == IDLE) {
		printf("Start connect to server...\n");
		gfxFlushBuffers();
		g_queueMessageMutex = new Mutex();
		g_sendingMessage = std::queue<QueueMessage*>();
		//---------------------------------------------------
		// init variables
		g_ip = strdup(ip);
		g_port = strdup(port);
		g_threadExit = false;
		//---------------------------------------------------
		// start thread
		
		g_thread = threadCreate(ppNetwork_threadRun, this, STACKSIZE, prio, -2, true);
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
	{/*
		printf("#%d : Old request data not solved yet! (%d bytes).\n", g_session->sessionID, g_waitForSize);
		gfxFlushBuffers();*/
	}else
	{
		/*printf("#%d : request data: %d - (%d bytes).\n", g_session->sessionID, tag, size);
		gfxFlushBuffers();*/
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
		//printf("#%d : add mesage to query.\n", g_session->sessionID);
		//gfxFlushBuffers();
		g_queueMessageMutex->Lock();
		QueueMessage* msg = new QueueMessage();
		msg->msgBuffer = msgBuffer;
		msg->msgSize = msgSize;
		g_sendingMessage.push(msg);
		g_queueMessageMutex->Unlock();
		//printf("#%d : added message successfully.\n", g_session->sessionID);
		//gfxFlushBuffers();
	}else
	{
		printf("#%d : Try to send but not connected yet.\n", g_session->sessionID);
		gfxFlushBuffers();
	}
}

PPNetwork::~PPNetwork()
{
	free(&g_ip);
	free(&g_port);
	if (g_receivedBuffer != nullptr) free(g_receivedBuffer);
}
