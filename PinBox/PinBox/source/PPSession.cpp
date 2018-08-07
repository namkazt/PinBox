#include "PPSession.h"
#include "PPGraphics.h"
#include "PPSessionManager.h"
#include "ConfigManager.h"

PPSession::~PPSession()
{
	if (g_network != nullptr) delete g_network;
}


void PPSession::initSession()
{
	if (g_network != nullptr) return;
	g_network = new PPNetwork();
	g_network->g_session = this;
	g_tmpMessage = new PPMessage();
	//------------------------------------------------------
	// callback when request data is received
	g_network->SetOnReceivedRequest([=](PPNetwork *self, u8* buffer, u32 size, u32 tag) {
		//------------------------------------------------------
		// verify authentication
		//------------------------------------------------------
		if (!g_authenticated)
		{
			if (tag == PPREQUEST_AUTHEN)
			{
				// check for authentication
				PPMessage *authenMsg = new PPMessage();
				if (authenMsg->ParseHeader(buffer))
				{
					if (authenMsg->GetMessageCode() == MSG_CODE_RESULT_AUTHENTICATION_SUCCESS)
					{
						printf("#%d : Authentication successted.\n", sessionID);
						g_authenticated = true;
						if (g_onAuthenSuccessed != nullptr) 
							g_onAuthenSuccessed(self, nullptr, 0);
					}
					else
					{
						printf("#%d : Authenticaiton failed.\n", sessionID);
						return;
					}
				}
				else
				{
					printf("#%d : Authenticaiton failed.\n", sessionID);
					return;
				}
				delete authenMsg;
			}
			else
			{
				printf("Client was not authentication.\n");
				return;
			}
		}
		//------------------------------------------------------
		// process data by tag
		switch (tag)
		{
		case PPREQUEST_HEADER:
		{
			if (g_tmpMessage->ParseHeader(buffer))
				self->SetRequestData(g_tmpMessage->GetContentSize(), PPREQUEST_BODY);
			else
				g_tmpMessage->ClearHeader();
			break;
		}
		case PPREQUEST_BODY:
		{
			//------------------------------------------------------
			// if tmp message is null that mean this is useless data then we avoid it
			if (g_tmpMessage->GetContentSize() == 0) return;
			// verify buffer size with message estimate size
			if (size == g_tmpMessage->GetContentSize())
			{
				// process message
				switch (g_sessionType)
				{
				case PPSESSION_MOVIE:
				{
					processMovieSession(buffer, size);
					break;
				}
				case PPSESSION_SCREEN_CAPTURE:
				{
					processScreenCaptureSession(buffer, size);
					break;
				}
				case PPSESSION_INPUT_CAPTURE:
				{
					processInputSession(buffer, size);
					break;
				}
				}

				//----------------------------------------------------------
				// Request for next message
				self->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
			}
			//------------------------------------------------------
			// remove message after use
			g_tmpMessage->ClearHeader();
			break;
		}
		default: break;
		}
	});
}


void PPSession::InitMovieSession()
{
	initSession();
	//--------------------------------------
	// init specific for movie session
	g_sessionType = PPSESSION_MOVIE;

	printf("#%d : Init new MOVIE session.\n", sessionID);
	gfxFlushBuffers();
}

void PPSession::InitScreenCaptureSession(PPSessionManager* manager)
{
	g_manager = manager;
	initSession();
	//--------------------------------------
	// init specific for movie session
	g_sessionType = PPSESSION_SCREEN_CAPTURE;
	//--------------------------------------
	printf("#%d : Init new SCREEN CAPTURE session.\n", sessionID);
	gfxFlushBuffers();
}

void PPSession::InitInputCaptureSession(PPSessionManager* manager)
{
	g_manager = manager;
	initSession();
	//--------------------------------------
	// init specific for movie session
	g_sessionType = PPSESSION_INPUT_CAPTURE;

	printf("#%d : Init new INPUT session.\n", sessionID);
	gfxFlushBuffers();
}


void PPSession::StartSession(const char* ip, const char* port, s32 prio, PPNetworkCallback authenSuccessed)
{
	if (g_network == nullptr) return;
	g_onAuthenSuccessed = authenSuccessed;
	g_network->SetOnConnectionSuccessed([=](PPNetwork *self, u8* data, u32 size)
	{
		//NOTE: this not called on main thread !
		//--------------------------------------------------
		int8_t code = 0;
		if (g_sessionType == PPSESSION_MOVIE) code = MSG_CODE_REQUEST_AUTHENTICATION_MOVIE;
		else if (g_sessionType == PPSESSION_SCREEN_CAPTURE) code = MSG_CODE_REQUEST_AUTHENTICATION_SCREEN_CAPTURE;
		else if (g_sessionType == PPSESSION_INPUT_CAPTURE) code = MSG_CODE_REQUEST_AUTHENTICATION_INPUT;
		if (code == 0)
		{
			printf("#%d : Invalid session type\n", sessionID);
			gfxFlushBuffers();
			return;
		}
		//--------------------------------------------------
		// screen capture session authen
		PPMessage *authenMsg = new PPMessage();
		authenMsg->BuildMessageHeader(code);
		u8* msgBuffer = authenMsg->BuildMessageEmpty();
		//--------------------------------------------------
		// send authentication message
		self->SendMessageData(msgBuffer, authenMsg->GetMessageSize());
		//--------------------------------------------------
		// set request to get result message
		self->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_AUTHEN);
		delete authenMsg;
	});
	g_network->SetOnConnectionClosed([=](PPNetwork *self, u8* data, u32 size)
	{
		//NOTE: this not called on main thread !
		printf("#%d : Connection interupted.\n", sessionID);
		gfxFlushBuffers();
	});
	g_network->Start(ip, port, prio);
}

void PPSession::CloseSession()
{
	if (g_network == nullptr) return;
	g_network->Stop();
	g_authenticated = false;
	SS_Reset();
}


void PPSession::processMovieSession(u8* buffer, size_t size)
{
	//NOTE: this not called on main thread !
	//------------------------------------------------------
	// process message data	by message type
	//------------------------------------------------------
	switch (g_tmpMessage->GetMessageCode())
	{
	default: break;
	}
}

void PPSession::processScreenCaptureSession(u8* buffer, size_t size)
{
	//NOTE: this not called on main thread !
	//------------------------------------------------------
	// process message data	by message type
	//------------------------------------------------------
	switch (g_tmpMessage->GetMessageCode())
	{
	case MSG_CODE_REQUEST_NEW_SCREEN_FRAME:
	{
		SS_SendReceivedFrame();

		g_manager->SafeTrack(buffer, size);
		
		break;
	}
	case MSG_CODE_REQUEST_NEW_AUDIO_FRAME:
	{
		SS_SendReceivedAudioFrame();

		g_manager->SafeTrackAudio(buffer, size);

		break;
	}
	default: break;
	}
}


void PPSession::processInputSession(u8* buffer, size_t size)
{
	//NOTE: this not called on main thread !
	//------------------------------------------------------
	// process message data	by message type
	//------------------------------------------------------
	switch (g_tmpMessage->GetMessageCode())
	{
	default: break;
	}
}


//-----------------------------------------------------
// screen capture
//-----------------------------------------------------
void PPSession::SS_StartStream()
{
	PPMessage *msg = new PPMessage();
	msg->BuildMessageHeader(MSG_CODE_REQUEST_START_SCREEN_CAPTURE);
	u8* msgBuffer = msg->BuildMessageEmpty();
	g_network->SendMessageData(msgBuffer, msg->GetMessageSize());
	g_network->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
	SS_v_isStartStreaming = true;
	delete msg;
}

void PPSession::SS_StopStream()
{
	if (!SS_v_isStartStreaming) return;
	//--------------------------------------
	PPMessage *msg = new PPMessage();
	msg->BuildMessageHeader(MSG_CODE_REQUEST_STOP_SCREEN_CAPTURE);
	u8* msgBuffer = msg->BuildMessageEmpty();
	g_network->SendMessageData(msgBuffer, msg->GetMessageSize());
	g_network->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
	SS_v_isStartStreaming = false;
	delete msg;
	//--------------------------------------
	this->CloseSession();
}

void PPSession::SS_ChangeSetting()
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
	u8 _setting_waitToReceivedFrame = ConfigManager::Get()->_cfg_wait_for_received ? 1 : 0;
	WRITE_U8(pointer, _setting_waitToReceivedFrame);
	// setting: smooth frame number ( only activate if waitForReceivedFrame = true)
	WRITE_U32(pointer, ConfigManager::Get()->_cfg_skip_frame);
	// setting: frame quality [0 ... 100]
	WRITE_U32(pointer, ConfigManager::Get()->_cfg_video_quality);
	// setting: frame scale [0 ... 100]
	WRITE_U32(pointer, ConfigManager::Get()->_cfg_video_scale);
	//-----------------------------------------------
	// build message
	u8* msgBuffer = authenMsg->BuildMessage(contentBuffer, contentSize);
	g_network->SendMessageData(msgBuffer, authenMsg->GetMessageSize());
	g_network->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
	free(contentBuffer);
	delete authenMsg;
}

void PPSession::SS_SendReceivedFrame()
{
	PPMessage *msgObj = new PPMessage();
	msgObj->BuildMessageHeader(MSG_CODE_REQUEST_SCREEN_RECEIVED_FRAME);
	u8* msgBuffer = msgObj->BuildMessageEmpty();
	g_network->SendMessageData(msgBuffer, msgObj->GetMessageSize());
	delete msgObj;
}


void PPSession::SS_SendReceivedAudioFrame()
{
	PPMessage *msgObj = new PPMessage();
	msgObj->BuildMessageHeader(MSG_CODE_REQUEST_RECEIVED_AUDIO_FRAME);
	u8* msgBuffer = msgObj->BuildMessageEmpty();
	g_network->SendMessageData(msgBuffer, msgObj->GetMessageSize());
	delete msgObj;
}


void PPSession::SS_Reset()
{
	SS_v_isStartStreaming = false;
}

//-----------------------------------------------------
// common
//-----------------------------------------------------

void PPSession::RequestForheader()
{
	g_network->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
}

//-----------------------------------------------------
// Input
//-----------------------------------------------------
void PPSession::IN_Start()
{
	if (IN_isStart) return;
	PPMessage *msg = new PPMessage();
	msg->BuildMessageHeader(MSG_CODE_REQUEST_START_INPUT_CAPTURE);
	u8* msgBuffer = msg->BuildMessageEmpty();
	g_network->SendMessageData(msgBuffer, msg->GetMessageSize());
	g_network->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
	delete msg;

	IN_isStart = true;
}

bool PPSession::IN_SendInputData(u32 down, u32 up, short cx, short cy, short ctx, short cty)
{
	if (!IN_isStart) return false;
	PPMessage *msg = new PPMessage();
	msg->BuildMessageHeader(MSG_CODE_SEND_INPUT_CAPTURE);
	//-----------------------------------------------
	// alloc msg content block
	size_t contentSize = 16;
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
	g_network->SendMessageData(msgBuffer, msg->GetMessageSize());
	g_network->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
	delete msg;
	return true;
}

void PPSession::IN_SendIdleInput()
{
	PPMessage *msg = new PPMessage();
	msg->BuildMessageHeader(MSG_CODE_SEND_INPUT_CAPTURE_IDLE);
	u8* msgBuffer = msg->BuildMessageEmpty();
	g_network->SendMessageData(msgBuffer, msg->GetMessageSize());
	g_network->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
	delete msg;
}

void PPSession::IN_Stop()
{
	if (!IN_isStart) return;
	PPMessage *msg = new PPMessage();
	msg->BuildMessageHeader(MSG_CODE_REQUEST_STOP_INPUT_CAPTURE);
	u8* msgBuffer = msg->BuildMessageEmpty();
	g_network->SendMessageData(msgBuffer, msg->GetMessageSize());
	g_network->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
	delete msg;

	IN_isStart = false;

	this->CloseSession();
}
