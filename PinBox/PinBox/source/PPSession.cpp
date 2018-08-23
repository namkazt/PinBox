#include "PPSession.h"
#include "PPGraphics.h"
#include "PPSessionManager.h"
#include "ConfigManager.h"

#define ONE_MILLISECOND 1000000ULL
#define ONE_MICROSECOND 1000ULL
#define BUFFERSIZE 0x1000
#define BUFFER_POOL_SIZE (BUFFERSIZE * 12)
// static buffer to store socket data
static u8*						g_receivedBuffer;
static u64						g_receivedSize;
static u64						g_waitForSize;
static u32						g_msgTag;

namespace { // helpers
	void createNew(void* arg) {
		static_cast<PPSession*>(arg)->threadMain();
	}
	void createTest(void* arg) {
		static_cast<PPSession*>(arg)->threadTest();
	}
}

PPSession::PPSession()
{
	// init static buffer
	g_receivedBuffer = (u8*)malloc(BUFFER_POOL_SIZE);
	g_receivedSize = 0;
	g_waitForSize = 0;
	g_msgTag = 0;
}

PPSession::~PPSession()
{
	ReleaseSession();

	CleanUp();
	// free static buffer
	free(g_receivedBuffer);
	free(_ip);
	free(_port);
}

void PPSession::InitTestSession(PPSessionManager* manager, const char* ip, const char* port)
{
	_testConnectionResult = 0;
	_manager = manager;
	_ip = strdup(ip);
	_port = strdup(port);
	s32 priority = 0;
	svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
	s32 t = priority - 2;
	if (t < 0x19) t = 0x19;
	// detached thread right after it created
	_thread = threadCreate(createTest, static_cast<void*>(this), 4 * 1024, t, -2, false);
}


void PPSession::InitSession(PPSessionManager* manager, const char* ip, const char* port)
{
	_sendingMessages = std::queue<QueueMessage*>();
	_queueMessageMutex = new Mutex();
	//-------------------------------------------------------------
	_manager = manager;
	_ip = strdup(ip);
	_port = strdup(port);
	//-------------------------------------------------------------
	_authenticated = false;
	s32 priority = 0;
	svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
	s32 t = priority - 2;
	if (t < 0x19) t = 0x19;
	_thread = threadCreate(createNew, static_cast<void*>(this), 4 * 1024, t, -2, false);
}


void PPSession::ReleaseSession()
{
	if (!_running || _kill) return;
	_kill = true;
	_running = false;
	// join thread
	threadJoin(_thread, U64_MAX);
	threadFree(_thread);
	_thread = NULL;
}

void PPSession::CleanUp()
{
	// clean up
	
	g_receivedSize = 0;
	g_waitForSize = 0;
	g_msgTag = 0;
	_authenticated = false;
	if (_tmpMessage)
	{
		delete _tmpMessage;
	}

	// cleanup hub
	for (HubItem* item : _hubItems)
	{
		if (item->thumbSize > 0) free(item->thumbBuf);
		delete item;
	}
	_hubItems.clear();
}

void PPSession::StartStream()
{
	_manager->InitDecoder();

	// send message
}



void PPSession::StopStream()
{
	_manager->ReleaseDecoder();

	//TODO: we should clean all stream relate message left behide

	// send message

}


void PPSession::connectToServer()
{
	_connect_state = CONNECTING;
	//_manager->SetSessionState(SS_CONNECTING);
	//--------------------------------------------------
	// define socket
	_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (_sock < 0)
	{
		printf("Can't create new socket.\n");
		gfxFlushBuffers();
		// Error: can't create socket
		_connect_state = FAIL;
		_manager->SetSessionState(SS_FAILED);
		return;
	}

	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	unsigned short nPort = (unsigned short)strtoul(_port, NULL, 0);
	addr.sin_port = htons(nPort);
	if(inet_pton(addr.sin_family, _ip, &addr.sin_addr) < 0)
	{
		printf("IP and Port not supported.\n", _ip, _port);
		gfxFlushBuffers();
		_connect_state = FAIL;
		_manager->SetSessionState(SS_FAILED);
		return;
	}

	printf("Connect to: %s p:%s.\n", _ip, _port);
	gfxFlushBuffers();

	int ret = connect(_sock, (struct sockaddr *) &addr, sizeof(addr));
	if (ret < 0)
	{
		printf("Could not connect to server.\n");
		gfxFlushBuffers();
		_connect_state = FAIL;
		_manager->SetSessionState(SS_FAILED);
		return;
	}

	_connect_state = CONNECTED;
	_manager->SetSessionState(SS_CONNECTED);

	// set socket to non blocking so we can easy control it
	//fcntl(sockManager->sock, F_SETFL, O_NONBLOCK);
	fcntl(_sock, F_SETFL, fcntl(_sock, F_GETFL, 0) | O_NONBLOCK);
	printf("Connected to server.\n");
	gfxFlushBuffers();
}

void PPSession::closeConnect()
{
	// set kill again to make sure it set

	// close connection
	if(_connect_state == CONNECTED)
	{
		Result rc = closesocket(_sock);
		if(rc != 0)
		{
			printf("Failed when close socket.\n");
		}
	}
	_sock = -1;
	_connect_state = IDLE;

	printf("closed session.\n");
}

void PPSession::recvSocketData()
{
	//---------------------------------------------------------------------------------
	// receive data
	//---------------------------------------------------------------------------------
	int recvAmount = recv(_sock, g_receivedBuffer + g_receivedSize, BUFFERSIZE, 0); //TODO: Citra seem like can't use this method and it crash immedately
	if (recvAmount <= 0)
	{
		if (errno != EWOULDBLOCK) {
			printf("Error receive packet: %d\n", recvAmount);
			_kill = true;
		}
		return;
	}else if(recvAmount > BUFFERSIZE)
	{
		// sound like something are wrong here so we do notthing
		return;
	}else
	{
		g_receivedSize += recvAmount;
	}
	//printf("receive packet s:%d - total: %d - w:%d\n", recvAmount, g_receivedSize, g_waitForSize);
	//---------------------------------------------------------------------------------
	// process data
	//---------------------------------------------------------------------------------
	if (g_receivedSize < g_waitForSize || g_waitForSize == 0) return;

	int dataAfterProcess = g_receivedSize - g_waitForSize;
	do {
		// calculate data left
		u32 lastWaitForSize = g_waitForSize;

		// process message data
		processReceivedMsg(g_receivedBuffer, lastWaitForSize, g_msgTag);

		// shifting mem
		memcpy(g_receivedBuffer, g_receivedBuffer + lastWaitForSize, dataAfterProcess);

		// reset information
		g_receivedSize = dataAfterProcess;
		if(g_receivedSize < g_waitForSize || g_waitForSize == 0) return;

		dataAfterProcess = g_receivedSize - g_waitForSize;
	} while (dataAfterProcess >= 0);
}

void PPSession::sendMessageData()
{
	_queueMessageMutex->Lock();
	while(!_sendingMessages.empty())
	{
		// get top message
		QueueMessage* queueMsg = (QueueMessage*)_sendingMessages.front();
		_sendingMessages.pop();

		if(queueMsg->msgSize > 0 && queueMsg->msgBuffer != nullptr)
		{
			u32 totalSent = 0;
			// send message
			do
			{
				int sendAmount = send(_sock, queueMsg->msgBuffer, queueMsg->msgSize, 0);
				if (sendAmount < 0)
				{
					// SS_FAILED when send message
					printf("Error when send message.\n");
					ReleaseSession();
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
	_queueMessageMutex->Unlock();
}


void PPSession::threadMain()
{
	if (_running) return;
	_running = true;
	u64 sleepDuration = ONE_MILLISECOND * 2;
	// thread loop
	while (!_kill)
	{
		if (_connect_state == IDLE)
		{
			connectToServer();
		}
		if (_connect_state == CONNECTED)
		{
			// check and recv data from server
			recvSocketData();
			// send queue message
			sendMessageData();
		}

		svcSleepThread(sleepDuration);
	}

	// send all message left
	sendMessageData();
	std::queue<QueueMessage*>().swap(_sendingMessages);
	delete _queueMessageMutex;

	// close connection
	closeConnect();
}

void PPSession::threadTest()
{
	if (_running) return;
	_running = true;
	u64 sleepDuration = ONE_MILLISECOND * 2;
	_testConnectionResult = 0;
	// thread loop
	while (!_kill)
	{
		if (_connect_state == IDLE)
		{
			connectToServer();
		}
		if (_connect_state == CONNECTED)
		{
			_testConnectionResult = 1;
		}
		if (_connect_state == FAIL)
		{
			_testConnectionResult = -1;
		}

		svcSleepThread(sleepDuration);
	}
	// close connection
	closeConnect();
}

void PPSession::RequestForData(u32 size, u32 tag)
{
	g_waitForSize = size;
	g_msgTag = tag;
}

void PPSession::AddMessageToQueue(u8* msgBuffer, int32_t msgSize)
{
	if (_running && _connect_state == CONNECTED && !_kill) {
		_queueMessageMutex->Lock();
		QueueMessage *msg = new QueueMessage();
		msg->msgBuffer = msgBuffer;
		msg->msgSize = msgSize;
		_sendingMessages.push(msg);
		_queueMessageMutex->Unlock();
	}
}


void PPSession::processReceivedMsg(u8* buffer, u32 size, u32 tag)
{
	//printf("process receive part size: %d - tag: %d.\n", size, tag);
	//------------------------------------------------------
	// verify authentication
	//------------------------------------------------------
	if (!_authenticated)
	{
		if (tag == PPREQUEST_AUTHEN)
		{
			PPMessage *authenMsg = new PPMessage();
			if (authenMsg->ParseHeader(buffer))
				if (authenMsg->GetMessageCode() == MSG_CODE_RESULT_AUTHENTICATION_SUCCESS)
				{
					printf("Authenticated successfully.\n");
					_authenticated = true;

					_manager->SetBusyState(BS_NONE);
					// send message to get hub items and other information
					SendMsgRequestHubItems();
					return;
				}
				else printf("Authenticaiton failed.\n");
			else printf("Authenticaiton failed.\n");
			delete authenMsg;
		}
		else printf("Client was not authentication.\n");
		RequestForData(MSG_COMMAND_SIZE, PPREQUEST_AUTHEN);
		return;
	}
	//------------------------------------------------------
	// process data by tag
	if (!_tmpMessage) _tmpMessage = new PPMessage();
	switch (tag)
	{
	case PPREQUEST_HEADER:
	{
		if (_tmpMessage->ParseHeader(buffer)) {
			RequestForData(_tmpMessage->GetContentSize(), PPREQUEST_BODY);
		} else {
			_tmpMessage->ClearHeader();
			RequestForData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
		}
		break;
	}
	case PPREQUEST_BODY:
	{
		//// if tmp message is null that mean this is useless data then we avoid it
		//if (_tmpMessage->GetContentSize() == 0) {
		//	_tmpMessage->ClearHeader();
		//	// we should prepare request for new header
		//	RequestForData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
		//	return;
		//}
		// verify buffer size with message estimate size
		if (size == _tmpMessage->GetContentSize())
		{
			processMessageData(buffer, size);
			// Request for next message
			RequestForData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
		}
		//------------------------------------------------------
		// remove message after use
		_tmpMessage->ClearHeader();
		break;
	}
	default: break;
	}
}


void PPSession::processMessageData(u8* buffer, size_t size)
{
	// process message data	by message type
	switch (_tmpMessage->GetMessageCode())
	{
	case MSG_CODE_REQUEST_NEW_SCREEN_FRAME:
		_manager->ProcessVideoFrame(buffer, size);
		break;
	case MSG_CODE_REQUEST_NEW_AUDIO_FRAME:
		//TODO: currently audio sleep make it all slow
		// should be avoid this by doing that in main thread
		_manager->ProcessAudioFrame(buffer, size);
		break;
	case MSG_CODE_RECEIVED_HUB_ITEMS:
		printf("Got hub items data\n");
		// On get hub items list
		u32 cursor = 0;
		int i = 0;
		// read number of hub items
		u16 count = READ_U16(buffer, cursor); cursor += 2;
		u16 size;

		printf("Count: %d\n", count);
		// read hub items
		for(i = 0; i < count; ++i)
		{
			printf("> Read item: %d\n", i);
			HubItem *item = new HubItem();

			// read type : 1 bytes
			u8 type = READ_U8(buffer, cursor); cursor += 1;
			item->type = type;
			printf("> Type: %d\n", type);

			// read uuid
			// size : 2 bytes
			size = READ_U16(buffer, cursor); cursor += 2;
			item->uuid.resize(size);
			memcpy(&item->uuid[0], buffer + cursor, size); 
			cursor += size;
			printf("> uuid: %s :%d\n", item->uuid.c_str(), size);

			// read name
			// size : 2 bytes
			size = READ_U16(buffer, cursor); cursor += 2;
			item->name.resize(size);
			memcpy(&item->name[0], buffer + cursor, size); 
			cursor += size;
			printf("> name: %s : %d\n", item->name.c_str(), size);

			

			if (type != HUB_SCREEN) {
				// read thumbnail
				// size : 4 bytes
				item->thumbSize = READ_U32(buffer, cursor); 
				cursor += 4;
#ifndef USE_CITRA
				// only work on N3ds device
				PPGraphics::Get()->AddCacheImage(&buffer[cursor], item->thumbSize, item->uuid);
#else
				// Work around because of add direct buffer to cache make image unexpected behaviour
				std::string fname = "pinbox/tmp/" + item->uuid + ".png";
				if (PPGraphics::Get()->AddCacheImage(fname.c_str(), item->uuid) == nullptr) {
					printf("Write cache file: %s\n", fname.c_str());
					// save to file
					FILE *f = fopen(fname.c_str(), "wb");
					if (f != NULL)
					{
						fwrite(buffer + cursor, sizeof(u8), item->thumbSize, f);
						fclose(f);
						PPGraphics::Get()->AddCacheImage(fname.c_str(), item->uuid);
					}
				}
#endif
				cursor += item->thumbSize;
			}

			_hubItems.push_back(item);
		}

		_manager->SetBusyState(BS_NONE);
		_manager->SetSessionState(SS_PAIRED);
		break;
	default: 
		break;
	}
}


//-----------------------------------------------------
// screen capture
//-----------------------------------------------------
void PPSession::SendMsgAuthentication()
{
	PPMessage *msg = new PPMessage();
	msg->BuildMessageHeader(MSG_CODE_REQUEST_AUTHENTICATION_SESSION);
	u8* msgBuffer = msg->BuildMessageEmpty();
	AddMessageToQueue(msgBuffer, msg->GetMessageSize());
	RequestForData(MSG_COMMAND_SIZE, PPREQUEST_AUTHEN);
	delete msg;
}

void PPSession::SendMsgStartStream()
{
	if (isSessionStarted) return;
	PPMessage *msg = new PPMessage();
	msg->BuildMessageHeader(MSG_CODE_REQUEST_START_SCREEN_CAPTURE);
	u8* msgBuffer = msg->BuildMessageEmpty();
	AddMessageToQueue(msgBuffer, msg->GetMessageSize());
	isSessionStarted = true;
	delete msg;
}

void PPSession::SendMsgStopStream()
{
	if (!isSessionStarted) return;
	//--------------------------------------
	PPMessage *msg = new PPMessage();
	msg->BuildMessageHeader(MSG_CODE_REQUEST_STOP_SCREEN_CAPTURE);
	u8* msgBuffer = msg->BuildMessageEmpty();
	AddMessageToQueue(msgBuffer, msg->GetMessageSize());
	isSessionStarted = false;
	delete msg;
	//--------------------------------------
	ReleaseSession();
}

void PPSession::SendMsgChangeSetting()
{
	PPMessage *authenMsg = new PPMessage();
	authenMsg->BuildMessageHeader(MSG_CODE_REQUEST_CHANGE_SETTING_SCREEN_CAPTURE);
	//-----------------------------------------------
	// alloc msg content block
	size_t contentSize = 13;
	u8* contentBuffer = (u8*)malloc(sizeof(u8) * contentSize);
	u8* pointer = contentBuffer;
	//----------------------------------------------
	// setting: wait for received frame
	//u8 _setting_waitToReceivedFrame = ConfigManager::Get()->_cfg_wait_for_received ? 1 : 0;
	//WRITE_U8(pointer, _setting_waitToReceivedFrame);
	//// setting: smooth frame number ( only activate if waitForReceivedFrame = true)
	//WRITE_U32(pointer, ConfigManager::Get()->_cfg_skip_frame);
	//// setting: frame quality [0 ... 100]
	//WRITE_U32(pointer, ConfigManager::Get()->_cfg_video_quality);
	//// setting: frame scale [0 ... 100]
	//WRITE_U32(pointer, ConfigManager::Get()->_cfg_video_scale);
	//-----------------------------------------------
	// build message
	u8* msgBuffer = authenMsg->BuildMessage(contentBuffer, contentSize);
	AddMessageToQueue(msgBuffer, authenMsg->GetMessageSize());
	free(contentBuffer);
	delete authenMsg;
}

void PPSession::SendMsgRequestHubItems()
{
	if (_connect_state == CONNECTED && _authenticated && _manager->GetBusyState() == BS_NONE) {
		printf("Request for hub items\n");
		_manager->SetBusyState(BS_HUB_ITEMS);
		// cleanup items
		_hubItems.clear();
		// send request
		PPMessage *msg = new PPMessage();
		msg->BuildMessageHeader(MSG_CODE_REQUEST_HUB_ITEMS);
		u8* msgBuffer = msg->BuildMessageEmpty();
		AddMessageToQueue(msgBuffer, msg->GetMessageSize());
		RequestForData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
		delete msg;
	}
}

//-----------------------------------------------------
// Input
//-----------------------------------------------------
bool PPSession::SendMsgSendInputData(u32 down, u32 up, short cx, short cy, short ctx, short cty)
{
	if (!isSessionStarted) return;
	PPMessage *msg = new PPMessage();
	msg->BuildMessageHeader(MSG_CODE_SEND_INPUT_CAPTURE);
	//-----------------------------------------------
	// alloc msg content block
	const size_t contentSize = 16;
	u8* contentBuffer = (u8*)malloc(sizeof(u8) * contentSize);
	u8* pointer = contentBuffer;
	//----------------------------------------------
	memcpy(pointer, &down, 4);
	memcpy(pointer + 4, &up, 4);
	memcpy(pointer + 8, &cx, 2);
	memcpy(pointer + 10, &cy, 2);
	memcpy(pointer + 12, &ctx, 2);
	memcpy(pointer + 14, &cty, 2);
	//-----------------------------------------------
	// build message and send
	u8* msgBuffer = msg->BuildMessage(contentBuffer, contentSize);
	AddMessageToQueue(msgBuffer, msg->GetMessageSize());
	delete msg;
	return true;
}