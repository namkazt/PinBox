#pragma once
#ifndef _PP_SESSION_H_
#define _PP_SESSION_H_

//=======================================================================
// PinBox Video Session
// container streaming data can be movie or screen capture
// With:
// 1, Movie :
// TODO: using ffmpeg to decode movie stream from server
// TODO: support 3D movie on N3DS
// TODO: mainly support RGB565 for lower size and speed up stream
// 2, Screen Capture:
// TODO: capture entire screen or a part of screen that config in server
//-----------------------------------------------------------------------
// Note: each session is running standalone and only 1 session can be run
// at a time.
//=======================================================================

#include <3ds.h>
#include <map>
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
#include <queue>

#include <constant.h>
#include "Mutex.h"
#include "PPMessage.h"
#include "HubItem.h"

enum PPSession_Type { PPSESSION_NONE, PPSESSION_MOVIE, PPSESSION_SCREEN_CAPTURE, PPSESSION_INPUT_CAPTURE};

#define MSG_COMMAND_SIZE 9

#define PPREQUEST_AUTHEN 50
#define PPREQUEST_HEADER 10
#define PPREQUEST_BODY 15
// authentication code
#define MSG_CODE_REQUEST_AUTHENTICATION_SESSION 1

#define MSG_CODE_RESULT_AUTHENTICATION_SUCCESS 5
#define MSG_CODE_RESULT_AUTHENTICATION_FAILED 6
// screen capture code
#define MSG_CODE_REQUEST_START_SCREEN_CAPTURE 10
#define MSG_CODE_REQUEST_STOP_SCREEN_CAPTURE 11
#define MSG_CODE_REQUEST_CHANGE_SETTING_SCREEN_CAPTURE 12
#define MSG_CODE_REQUEST_NEW_SCREEN_FRAME 15
#define MSG_CODE_REQUEST_SCREEN_RECEIVED_FRAME 16
#define MSG_CODE_REQUEST_NEW_AUDIO_FRAME 18
#define MSG_CODE_REQUEST_RECEIVED_AUDIO_FRAME 19


// input
#define MSG_CODE_SEND_INPUT_CAPTURE 42
#define MSG_CODE_SEND_INPUT_CAPTURE_IDLE 44

// hub
#define MSG_CODE_REQUEST_HUB_ITEMS 60
#define MSG_CODE_RECEIVED_HUB_ITEMS 61

// audio
#define AUDIO_CHANNEL	0x08


typedef struct
{
	void		*msgBuffer;
	u32			msgSize;
} QueueMessage;


class PPSessionManager;


enum ppConectState { IDLE, CONNECTING, CONNECTED, FAIL };
typedef std::function<void(u8* buffer, u32 size, u32 tag)> PPNetworkReceivedRequest;
typedef std::function<void(u8* data, u32 code)> PPNetworkCallback;

class PPSession
{
private:
	PPSessionManager				*_manager;
	PPMessage*						_tmpMessage = nullptr;
	bool							_authenticated = false;
	std::queue<QueueMessage*>		_sendingMessages;
	Mutex*							_queueMessageMutex;
private:
	// threading
	bool							_running = false;
	bool							_kill = false;
	Thread							_thread;
	// socket
	const char*						_ip = 0;
	const char*						_port = 0;
	int								_sock = -1;
	ppConectState					_connect_state = IDLE;

	// test connection result
	int	volatile					_testConnectionResult = 0;

	// hub items
	std::vector<HubItem*>			_hubItems;

	void connectToServer();
	void closeConnect();
	void recvSocketData();
	void sendMessageData();

	void processReceivedMsg(u8* buffer, u32 size, u32 tag);;
	void processMessageData(u8* buffer, size_t size);

public:
	PPSession();
	~PPSession();

	int GetTestConnectionResult() const { return _testConnectionResult; }
	void InitTestSession(PPSessionManager* manager, const char* ip, const char* port);
	void threadTest();

	void InitSession(PPSessionManager* manager, const char* ip, const char* port);
	void threadMain();
	void ReleaseSession();
	void CleanUp();

	void StartStream();
	void StopStream();

	void RequestForData(u32 size, u32 tag = 0);
	void AddMessageToQueue(u8 *msgBuffer, int32_t msgSize);

	int GetHubItemCount() { return _hubItems.size(); }
	HubItem* GetHubItem(int i) { return _hubItems.at(i); }

private:
	bool								isInputStarted = false;
	bool								isSessionStarted = false;

public:
	// Authentication
	void								SendMsgAuthentication();

	// Stream
	void								SendMsgStartStream();
	void								SendMsgStopStream();

	// Setting
	void								SendMsgChangeSetting();

	// Hub 
	void								SendMsgRequestHubItems();

	// Input
	bool								SendMsgSendInputData(u32 down, u32 up, short cx, short cy, short ctx, short cty);
};

#endif