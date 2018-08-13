#include "stdafx.h"
#include "PPClientSession.h"
#include "PPMessage.h"
#include "PPServer.h"

void PPClientSession::InitSession(evpp::TCPConnPtr conn, PPServer* parent)
{
	_connection = conn;
	_server = parent;
	// static buffer
	_receivedBuffer = (u8*)malloc(BUFFER_SIZE);
	_receivedBufferSize = 0;
	// default wait size for authentication
	_waitForSize = 9;
}

void PPClientSession::DisconnectFromServer()
{
	if (_tmpMessage) delete _tmpMessage;
	//TODO: stop all stream

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
	//------------------------------------------------
	// merge msg data into buffer
	memcpy(_receivedBuffer + _receivedBufferSize, msg->data(), msg->size());
	// increment buffer size
	_receivedBufferSize += msg->size();

	if (_receivedBufferSize < _waitForSize || _waitForSize == 0) return;

	// process thought data buffer
	int dataAfterProcessed = _receivedBufferSize - _waitForSize;
	do
	{
		// calculate data left
		u32 lastWaitForSize = _waitForSize;

		// process message data 
		// NOTE: _waitForSize should be changed inside this function
		ProcessIncommingMessage(_receivedBuffer, lastWaitForSize);

		// shifting memory
		memcpy(_receivedBuffer, _receivedBuffer + lastWaitForSize, dataAfterProcessed);

		// update data left
		_receivedBufferSize = dataAfterProcessed;
		if (_receivedBufferSize < _waitForSize || _waitForSize == 0) break;
		dataAfterProcessed = _receivedBufferSize - _waitForSize;
	} while (dataAfterProcessed >= 0);
}

void PPClientSession::ProcessIncommingMessage(u8* buffer, u32 size)
{
	if(!IsAuthenticated())
	{
		//----------------------------------------------------
		// parse for header part
		PPMessage* autheMsg = new PPMessage();
		if (!autheMsg->ParseHeader(buffer))
		{
			std::cout << "[Authentication Error] Vaidation code incorrect " << std::endl;
			sendMessageWithCode(MSG_CODE_RESULT_AUTHENTICATION_FAILED);
			return;
		}
		//----------------------------------------------------
		// validate authentication
		if (autheMsg->GetMessageCode() != MSG_CODE_REQUEST_AUTHENTICATION_SESSION) {
			std::cout << "[Authentication Error] Invalid session type: " << autheMsg->GetMessageCode() << std::endl;
			sendMessageWithCode(MSG_CODE_RESULT_AUTHENTICATION_FAILED);
			return;
		}

		_server->ScreenCapturer->registerClientSession(this);
		_authenticated = true;
		// set waiting to a header msg
		_currentReadState = PPREQUEST_HEADER;
		_waitForSize = MSG_COMMAND_SIZE;
		//----------------------------------------------------
		// send back to client to validate authentication successfully
		std::cout << "[Authentication Successed] Session: #" << _connection->remote_addr() << std::endl;
		sendMessageWithCode(MSG_CODE_RESULT_AUTHENTICATION_SUCCESS);
	}else
	{
		if (!_tmpMessage) _tmpMessage = new PPMessage();
		if (_currentReadState == PPREQUEST_HEADER)
		{
			if (!_tmpMessage->ParseHeader(buffer))
			{
				std::cout << "[Parse Message Error] Vaidation code incorrect - are you sure current is header part ?" << std::endl;
				_tmpMessage->ClearHeader();
				return;
			}
			//----------------------------------------------------
			// switch to read body part
			_currentReadState = PPREQUEST_BODY;
			_waitForSize = _tmpMessage->GetContentSize();
			//----------------------------------------------------
			// pre process header code if need
			preprocessMessageCode(_tmpMessage->GetMessageCode());
			//----------------------------------------------------
			// it content size of message is zero so we start 
			// wait for new message
			if (_tmpMessage->GetContentSize() == 0) {
				_currentReadState = PPREQUEST_HEADER;
				_waitForSize = MSG_COMMAND_SIZE;
			}
		}
		else if (_currentReadState == PPREQUEST_BODY)
		{
			if (size == _tmpMessage->GetContentSize())
			{
				// process body buffer
				processMessageBody(buffer, _tmpMessage->GetMessageCode());
				// switch to read next header part
				_currentReadState = PPREQUEST_HEADER;
				_waitForSize = MSG_COMMAND_SIZE;
				_tmpMessage->ClearHeader();
			}
		}
	}
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

	case MSG_CODE_SEND_INPUT_CAPTURE_IDLE:
		break;
	

	default: 
		std::cout << "Client " << _connection->remote_addr() << " send Header UNKNOW COMMAND" << std::endl;
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
		std::cout << "Client " << _connection->remote_addr() << " send Body UNKNOW COMMAND" << std::endl << std::flush;
		break;
	}


}