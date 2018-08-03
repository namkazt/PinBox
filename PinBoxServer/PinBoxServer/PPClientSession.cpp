#include "stdafx.h"
#include "PPClientSession.h"
#include "PPMessage.h"
#include "PPServer.h"

void PPClientSession::InitSession(evpp::TCPConnPtr conn, PPServer* parent)
{
	g_connection = conn;
	m_server = parent;
	g_continuousBuffer = (u8*)malloc(DEFAULT_CONTINUOUS_BUFFER_SIZE * sizeof(u8));
	g_bufferSize = DEFAULT_CONTINUOUS_BUFFER_SIZE;
	g_bufferWriteIndex = 0;
}

void PPClientSession::DisconnectFromServer()
{
	if (g_sessionType == PPSESSION_MOVIE)
	{

	}
	else if (g_sessionType == PPSESSION_SCREEN_CAPTURE)
	{
		//TODO: screen capture disconnect
	}
	else if (g_sessionType == PPSESSION_INPUT_CAPTURE)
	{

	}
}

void PPClientSession::PreparePacketAndSend(u8* buffer, int bufferSize)
{
	g_ss_isReceived = false;
	//-------------------------------------------------------
	//std::cout << "SEND MESSAGE FRAME #" << piece->frameIndex << " PIECE #" << (u32)piece->index << " to " << g_connection->remote_addr() << " | msg size: " << (piece->size + 5) << std::endl;
	sendMessageWithCodeAndData(MSG_CODE_REQUEST_NEW_SCREEN_FRAME, buffer, bufferSize);
}

void PPClientSession::ProcessMessage(evpp::Buffer* msg)
{
	//--------------------------------------------------
	// if have new data
	if (msg) {
		u8* msgData = (u8*)msg->data();
		size_t msgSize = msg->size();
		// check with current old data
		if (msgSize + g_bufferWriteIndex > g_bufferSize)
		{
			g_bufferSize += DEFAULT_CONTINUOUS_BUFFER_STEP;
			u8* g_continuousBufferNew = (u8*)realloc((void*)g_continuousBuffer, g_bufferSize * sizeof(u8));
			if (!g_continuousBufferNew)
			{
				// fail to realloc memory
				//TODO: in this cause we need process message first and remove finished part
				// if can't be enough then app crash -> not enough memory
				g_bufferSize -= DEFAULT_CONTINUOUS_BUFFER_STEP;
				return;
			}
			else
			{
				// point old pointer to new point after reallocated
				g_continuousBuffer = g_continuousBufferNew;
			}
		}
		//------------------------------------------------
		// append into old data
		memcpy(g_continuousBuffer + g_bufferWriteIndex, msgData, msgSize);
		g_bufferWriteIndex += msgSize;
	}
	//------------------------------------------------
	// process message data
	if (this->IsAuthenticated())
	{
		//std::cout << "Incomming message from: " << g_connection->remote_addr() << std::endl;
		// not enough data for header so we just leave it
		if (g_bufferWriteIndex < 9 && g_currentReadState == PPREQUEST_HEADER) return;
		// start process data
		this->ProcessIncommingMessage();
	}
	else
	{
		//std::cout << "Process authentication from: " << g_connection->remote_addr() << std::endl;
		this->ProcessAuthentication();
		//--------------------------------------------
		// remove 9 bytes of authentication message
		CHOP_N(g_continuousBuffer, 9, g_bufferSize);
		g_bufferWriteIndex -= 9;
	}

	//------------------------------------------------
	// check if still need process message or not
	if (g_bufferWriteIndex >= g_waitForSize) ProcessMessage(nullptr);
}



void PPClientSession::ProcessIncommingMessage()
{
	//----------------------------------------------------
	if(g_currentReadState == PPREQUEST_HEADER)
	{
		//----------------------------------------------------
		// parse for header part
		PPMessage* autheMsg = new PPMessage();
		if (!autheMsg->ParseHeader(g_continuousBuffer))
		{
			std::cout << "[Parse Message Error] Vaidation code incorrect - are you sure current is header part ?" << std::endl;
			return;
		}
		//----------------------------------------------------
		// switch to read body part
		g_currentReadState = PPREQUEST_BODY;
		g_currentMsgCode = autheMsg->GetMessageCode();
		g_waitForSize = autheMsg->GetContentSize();
		//----------------------------------------------------
		// pre process header code if need
		preprocessMessageCode(g_currentMsgCode);
		//----------------------------------------------------
		// it content size of message is zero so we start 
		// wait for new message
		if (g_waitForSize <= 0) {
			g_currentReadState = PPREQUEST_HEADER;
			g_currentMsgCode = -1;
		}
		//--------------------------------------------
		// remove 9 bytes of header message
		CHOP_N(g_continuousBuffer, 9, g_bufferSize);
		g_bufferWriteIndex -= 9;
	}
	else if(g_currentReadState == PPREQUEST_BODY)
	{
		if(g_bufferWriteIndex >= g_waitForSize)
		{
			u8* bodyBuffer = (u8*)malloc(g_waitForSize);
			memcpy(bodyBuffer, g_continuousBuffer, g_waitForSize);
			//----------------------------------------------------
			// process body buffer
			processMessageBody(bodyBuffer, g_currentMsgCode);
			// free buffer data
			free(bodyBuffer);
			//--------------------------------------------
			// remove body data
			CHOP_N(g_continuousBuffer, g_waitForSize, g_bufferSize);
			g_bufferWriteIndex -= g_waitForSize;
			//----------------------------------------------------
			// switch to read next header part
			g_currentReadState = PPREQUEST_HEADER;
			g_currentMsgCode = -1;
			g_waitForSize = 0;
		}
	}
	
}

void PPClientSession::ProcessAuthentication()
{
	//----------------------------------------------------
	// parse for header part
	PPMessage* autheMsg = new PPMessage();
	if(!autheMsg->ParseHeader(g_continuousBuffer))
	{
		std::cout << "[Authentication Error] Vaidation code incorrect " << std::endl;
		sendMessageWithCode(MSG_CODE_RESULT_AUTHENTICATION_FAILED);
		return;
	}
	//----------------------------------------------------
	// validate authentication
	if(autheMsg->GetMessageCode() == MSG_CODE_REQUEST_AUTHENTICATION_MOVIE)
	{
		g_sessionType = PPSESSION_MOVIE;
	}else if(autheMsg->GetMessageCode() == MSG_CODE_REQUEST_AUTHENTICATION_SCREEN_CAPTURE)
	{
		g_sessionType = PPSESSION_SCREEN_CAPTURE;
		m_server->ScreenCapturer->registerClientSession(this);
	}
	else if (autheMsg->GetMessageCode() == MSG_CODE_REQUEST_AUTHENTICATION_INPUT)
	{
		g_sessionType = PPSESSION_INPUT_CAPTURE;
	}else
	{
		std::cout << "[Authentication Error] Invalid session type: " << autheMsg->GetMessageCode() << std::endl;
		sendMessageWithCode(MSG_CODE_RESULT_AUTHENTICATION_FAILED);
		return;
	}
	g_authenticated = true;
	// set waiting to a header msg
	g_currentReadState = PPREQUEST_HEADER;
	//----------------------------------------------------
	// send back to client to validate authentication successfully
	std::cout << "[Authentication Successed] Session: #" << g_connection->remote_addr()  << std::endl;
	sendMessageWithCode(MSG_CODE_RESULT_AUTHENTICATION_SUCCESS);
}


void PPClientSession::sendMessageWithCode(u8 code)
{
	PPMessage* msg = new PPMessage();
	msg->BuildMessageHeader(code);
	void* buffer = msg->BuildMessageEmpty();
	g_connection->Send(buffer, 9);
	free(buffer);
}

void PPClientSession::sendMessageWithCodeAndData(u8 code, u8* buffer, size_t bufferSize)
{
	PPMessage* msg = new PPMessage();
	msg->BuildMessageHeader(code);
	void* result = msg->BuildMessage(buffer, bufferSize);
	g_connection->Send(result, msg->GetMessageSize());
	free(result);
}

void PPClientSession::preprocessMessageCode(u8 code)
{
	if(g_sessionType == PPSESSION_MOVIE)
	{
		
	}
	else if (g_sessionType == PPSESSION_SCREEN_CAPTURE)
	{
		switch (code)
		{
		case MSG_CODE_REQUEST_START_SCREEN_CAPTURE:
			std::cout << "Client send COMMAND: Start Stream" << std::endl;
			m_server->ScreenCapturer->startStream();
			break;
		case MSG_CODE_REQUEST_STOP_SCREEN_CAPTURE:
			std::cout << "Client send COMMAND: Stop Stream" << std::endl;
			m_server->ScreenCapturer->stopStream();
			break;
		case MSG_CODE_REQUEST_SCREEN_RECEIVED_FRAME: {
			//std::cout << "Client " << g_connection->remote_addr() << " send RECEIVED_FRAME" << std::endl;
			g_ss_isReceived = true;
			break;
		}
		case MSG_CODE_REQUEST_RECEIVED_AUDIO_FRAME: {

			break;
		}
		case MSG_CODE_REQUEST_CHANGE_SETTING_SCREEN_CAPTURE: {
			break;
		}
		default: {
			std::cout << "Client " << g_connection->remote_addr() << " send UNKNOW COMMAND" << std::endl;
			break;
		}
		}
	}
	else if (g_sessionType == PPSESSION_INPUT_CAPTURE)
	{
		switch (code)
		{
		case MSG_CODE_REQUEST_START_INPUT_CAPTURE:
			//TODO: start input
			break;		
		case MSG_CODE_REQUEST_STOP_INPUT_CAPTURE:
			//TODO: stop input 
			break;
		case MSG_CODE_SEND_INPUT_CAPTURE_IDLE:
			//TODO: stop input 
			break;
		}
	}
}

void PPClientSession::processMessageBody(u8* buffer, u8 code)
{
	if (g_sessionType == PPSESSION_MOVIE)
	{

	}
	else if (g_sessionType == PPSESSION_SCREEN_CAPTURE)
	{
		switch (code)
		{
		case MSG_CODE_REQUEST_CHANGE_SETTING_SCREEN_CAPTURE: {
			//NOTE: do those setting are useless for now.

			// wait for frame
			int8_t waitForReceived = READ_U8(buffer, 0);
			bool _waitForClientReceived = !(waitForReceived == 0);
			// smoother frame
			u32 _waitForFrame = READ_U32(buffer, 1);
			// smoother frame
			u32 _outputQuality = READ_U32(buffer, 5);
			// smoother frame
			u32 _outputScale = READ_U32(buffer, 9);
			///-----------------------------------
			std::cout << "Change Stream Setting: " << std::endl;
			std::cout << "Smooth Frame: " << _waitForFrame << std::endl;
			std::cout << "Quality: " << _outputQuality << std::endl;
			std::cout << "Scale: " << _outputScale << std::endl;
			///-----------------------------------
			//m_server->ScreenCapturer->changeSetting(_waitForClientReceived, _waitForFrame, _outputQuality, _outputScale);
			break;
		}
		default: break;
		}
	}
	else if (g_sessionType == PPSESSION_INPUT_CAPTURE)
	{
		switch (code)
		{
		case MSG_CODE_SEND_INPUT_CAPTURE:
		{
			//--------------------------------------
			//read input data
			u32 down = 0;
			memcpy(&down, buffer, 4);
			u32 up = 0;
			memcpy(&up, buffer + 4, 4);
			short cx = 0;
			memcpy(&cx, buffer + 8, 2);
			short cy = 0;
			memcpy(&cy, buffer + 10, 2);
			short ctx = 0;
			memcpy(&ctx, buffer + 12, 2);
			short cty = 0;
			memcpy(&cty, buffer + 14, 2);
			//--------------------------------------
			m_server->InputStreamer->UpdateInput(down, up, cx, cy, ctx, cty);
			break;
		}
		default: 
			std::cout << "[INPUT] Client " << g_connection->remote_addr() << " send UNKNOW COMMAND" << std::endl << std::flush;
			break;
		
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------/////////////////////////////////////////////////////////////////////
// SCREEN CAPTURE										/////////////////////////////////////////////////////////////////////
//------------------------------------------------------/////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------/////////////////////////////////////////////////////////////////////
// MOVIE												/////////////////////////////////////////////////////////////////////
//------------------------------------------------------/////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------/////////////////////////////////////////////////////////////////////
// INPUT												/////////////////////////////////////////////////////////////////////
//------------------------------------------------------/////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------