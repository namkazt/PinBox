#include "stdafx.h"
#include "PPClientSession.h"
#include "PPMessage.h"
#include "PPServer.h"

void PPClientSession::InitSession(evpp::TCPConnPtr conn, PPServer* parent)
{
	_connection = conn;
	_server = parent;
	_continuousBuffer = (u8*)malloc(DEFAULT_CONTINUOUS_BUFFER_SIZE);
	_bufferSize = DEFAULT_CONTINUOUS_BUFFER_SIZE;
	_bufferWriteIndex = 0;
}

void PPClientSession::DisconnectFromServer()
{

}

void PPClientSession::PrepareVideoPacketAndSend(u8* buffer, int bufferSize)
{
	sendMessageWithCodeAndData(MSG_CODE_REQUEST_NEW_SCREEN_FRAME, buffer, bufferSize);
}

void PPClientSession::PrepareAudioPacketAndSend(u8* buffer, int bufferSize, uint64_t pts)
{
	sendMessageWithCodeAndData(MSG_CODE_REQUEST_NEW_AUDIO_FRAME, buffer, bufferSize);
}

void PPClientSession::ProcessMessage(evpp::Buffer* msg)
{
	//--------------------------------------------------
	// if have new data
	if (msg) {
		u8* msgData = (u8*)msg->data();
		size_t msgSize = msg->size();
		// check with current old data
		if (msgSize + _bufferWriteIndex > _bufferSize)
		{
			_bufferSize += DEFAULT_CONTINUOUS_BUFFER_STEP;
			u8* g_continuousBufferNew = (u8*)realloc((void*)_continuousBuffer, _bufferSize * sizeof(u8));
			if (!g_continuousBufferNew)
			{
				// fail to realloc memory
				//TODO: in this cause we need process message first and remove finished part
				// if can't be enough then app crash -> not enough memory
				_bufferSize -= DEFAULT_CONTINUOUS_BUFFER_STEP;
				return;
			}
			else
			{
				// point old pointer to new point after reallocated
				_continuousBuffer = g_continuousBufferNew;
			}
		}
		//------------------------------------------------
		// append into old data
		memcpy(_continuousBuffer + _bufferWriteIndex, msgData, msgSize);
		_bufferWriteIndex += msgSize;
	}
	//------------------------------------------------
	// process message data
	if (this->IsAuthenticated())
	{
		//std::cout << "Incomming message from: " << g_connection->remote_addr() << std::endl;
		// not enough data for header so we just leave it
		if (_bufferWriteIndex < 9 && _currentReadState == PPREQUEST_HEADER) return;
		// start process data
		this->ProcessIncommingMessage();
	}
	else
	{
		//std::cout << "Process authentication from: " << g_connection->remote_addr() << std::endl;
		this->ProcessAuthentication();
		//--------------------------------------------
		// remove 9 bytes of authentication message
		CHOP_N(_continuousBuffer, 9, _bufferSize);
		_bufferWriteIndex -= 9;
	}

	//------------------------------------------------
	// check if still need process message or not
	if (_bufferWriteIndex >= _waitForSize) ProcessMessage(nullptr);
}



void PPClientSession::ProcessIncommingMessage()
{
	//----------------------------------------------------
	if(_currentReadState == PPREQUEST_HEADER)
	{
		//----------------------------------------------------
		// parse for header part
		PPMessage* autheMsg = new PPMessage();
		if (!autheMsg->ParseHeader(_continuousBuffer))
		{
			std::cout << "[Parse Message Error] Vaidation code incorrect - are you sure current is header part ?" << std::endl;
			return;
		}
		//----------------------------------------------------
		// switch to read body part
		_currentReadState = PPREQUEST_BODY;
		_currentMsgCode = autheMsg->GetMessageCode();
		_waitForSize = autheMsg->GetContentSize();
		//----------------------------------------------------
		// pre process header code if need
		preprocessMessageCode(_currentMsgCode);
		//----------------------------------------------------
		// it content size of message is zero so we start 
		// wait for new message
		if (_waitForSize <= 0) {
			_currentReadState = PPREQUEST_HEADER;
			_currentMsgCode = -1;
		}
		//--------------------------------------------
		// remove 9 bytes of header message
		CHOP_N(_continuousBuffer, 9, _bufferSize);
		_bufferWriteIndex -= 9;
	}
	else if(_currentReadState == PPREQUEST_BODY)
	{
		if(_bufferWriteIndex >= _waitForSize)
		{
			u8* bodyBuffer = (u8*)malloc(_waitForSize);
			memcpy(bodyBuffer, _continuousBuffer, _waitForSize);
			//----------------------------------------------------
			// process body buffer
			processMessageBody(bodyBuffer, _currentMsgCode);
			// free buffer data
			free(bodyBuffer);
			//--------------------------------------------
			// remove body data
			CHOP_N(_continuousBuffer, _waitForSize, _bufferSize);
			_bufferWriteIndex -= _waitForSize;
			//----------------------------------------------------
			// switch to read next header part
			_currentReadState = PPREQUEST_HEADER;
			_currentMsgCode = -1;
			_waitForSize = 0;
		}
	}
	
}

void PPClientSession::ProcessAuthentication()
{
	//----------------------------------------------------
	// parse for header part
	PPMessage* autheMsg = new PPMessage();
	if(!autheMsg->ParseHeader(_continuousBuffer))
	{
		std::cout << "[Authentication Error] Vaidation code incorrect " << std::endl;
		sendMessageWithCode(MSG_CODE_RESULT_AUTHENTICATION_FAILED);
		return;
	}
	//----------------------------------------------------
	// validate authentication
	if(autheMsg->GetMessageCode() != MSG_CODE_REQUEST_AUTHENTICATION_SESSION){
		std::cout << "[Authentication Error] Invalid session type: " << autheMsg->GetMessageCode() << std::endl;
		sendMessageWithCode(MSG_CODE_RESULT_AUTHENTICATION_FAILED);
		return;
	}
	
	_authenticated = true;
	// set waiting to a header msg
	_currentReadState = PPREQUEST_HEADER;
	//----------------------------------------------------
	// send back to client to validate authentication successfully
	std::cout << "[Authentication Successed] Session: #" << _connection->remote_addr()  << std::endl;
	sendMessageWithCode(MSG_CODE_RESULT_AUTHENTICATION_SUCCESS);
}


void PPClientSession::sendMessageWithCode(u8 code)
{
	PPMessage* msg = new PPMessage();
	msg->BuildMessageHeader(code);
	void* buffer = msg->BuildMessageEmpty();
	_connection->Send(buffer, 9);
	free(buffer);
}

void PPClientSession::sendMessageWithCodeAndData(u8 code, u8* buffer, size_t bufferSize)
{
	PPMessage* msg = new PPMessage();
	msg->BuildMessageHeader(code);
	void* result = msg->BuildMessage(buffer, bufferSize);
	_connection->Send(result, msg->GetMessageSize());
	free(result);
}

void PPClientSession::preprocessMessageCode(u8 code)
{
	switch (code)
	{

	case MSG_CODE_REQUEST_START_SCREEN_CAPTURE:
		std::cout << "Client send COMMAND: Start Stream" << std::endl;
		_server->ScreenCapturer->startStream();
		break;
	case MSG_CODE_REQUEST_STOP_SCREEN_CAPTURE:
		std::cout << "Client send COMMAND: Stop Stream" << std::endl;
		_server->ScreenCapturer->stopStream();
		break;
	case MSG_CODE_REQUEST_SCREEN_RECEIVED_FRAME: 

		break;
	case MSG_CODE_REQUEST_RECEIVED_AUDIO_FRAME: 

		break;
	case MSG_CODE_REQUEST_CHANGE_SETTING_SCREEN_CAPTURE: 
		break;


	case MSG_CODE_REQUEST_START_INPUT_CAPTURE:
		break;
	case MSG_CODE_REQUEST_STOP_INPUT_CAPTURE:
		break;
	case MSG_CODE_SEND_INPUT_CAPTURE_IDLE:
		break;
	

	default: 
		std::cout << "Client " << _connection->remote_addr() << " send UNKNOW COMMAND" << std::endl;
		break;
	
	}

}

void PPClientSession::processMessageBody(u8* buffer, u8 code)
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
		//_server->ScreenCapturer->changeSetting(_waitForClientReceived, _waitForFrame, _outputQuality, _outputScale);
		break;
	}

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
		_server->InputStreamer->UpdateInput(down, up, cx, cy, ctx, cty);
		break;
	}

	default: 
		std::cout << "Client " << _connection->remote_addr() << " send UNKNOW COMMAND" << std::endl << std::flush;
		break;
	}


}