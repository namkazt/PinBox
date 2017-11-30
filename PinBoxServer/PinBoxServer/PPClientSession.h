#pragma once
#include <fstream>

// evpp
#include <glog/config.h>
#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

// frame capture
#include "ScreenCapture.h"

// openCV
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

#include "PPMessage.h"


enum PPSession_Type { PPSESSION_NONE, PPSESSION_MOVIE, PPSESSION_SCREEN_CAPTURE, PPSESSION_INPUT_CAPTURE };

#define MSG_COMMAND_SIZE 9

#define PPREQUEST_NONE 0
#define PPREQUEST_HEADER 10
#define PPREQUEST_BODY 15
// authentication code
#define MSG_CODE_REQUEST_AUTHENTICATION_MOVIE 1
#define MSG_CODE_REQUEST_AUTHENTICATION_SCREEN_CAPTURE 2
#define MSG_CODE_REQUEST_AUTHENTICATION_INPUT 3
#define MSG_CODE_RESULT_AUTHENTICATION_SUCCESS 5
#define MSG_CODE_RESULT_AUTHENTICATION_FAILED 6
// screen capture code
#define MSG_CODE_REQUEST_START_SCREEN_CAPTURE 10
#define MSG_CODE_REQUEST_STOP_SCREEN_CAPTURE 11
#define MSG_CODE_REQUEST_CHANGE_SETTING_SCREEN_CAPTURE 12
#define MSG_CODE_REQUEST_NEW_SCREEN_FRAME 15
#define MSG_CODE_REQUEST_SCREEN_RECEIVED_FRAME 16

// 8Mb default size just enough
#define DEFAULT_CONTINUOUS_BUFFER_SIZE 1024*1024* 8 
// 24Kb for each step
#define DEFAULT_CONTINUOUS_BUFFER_STEP 1024*24 

#define CHOP_N(BUFFER, COUNT, LENGTH) memmove(BUFFER, BUFFER + COUNT, LENGTH)

class PPClientSession
{
private:
	evpp::TCPConnPtr			g_connection;
	PPSession_Type				g_sessionType = PPSESSION_NONE;
	bool						g_authenticated = false;

	u8*							g_continuousBuffer = nullptr;
	u32							g_bufferWriteIndex = 0;
	u32							g_bufferSize = 0;

	u8							g_currentReadState = PPREQUEST_NONE;
	u8							g_currentMsgCode = 0;
	u32							g_waitForSize = 0;
private:
	void						sendMessageWithCode(u8 code);
	void						sendMessageWithCodeAndData(u8 code, u8* buffer, size_t bufferSize);
	void						preprocessMessageCode(u8 code);
	void						processMessageBody(u8* buffer, u8 code);

	void						ProcessIncommingMessage();
	void						ProcessAuthentication();
public:
	void						InitSession(evpp::TCPConnPtr conn);
	void						ProcessMessage(evpp::Buffer* msg);
	void						DisconnectFromServer();

	bool						IsAuthenticated() const { return g_authenticated; }

private:
	//---------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------/////////////////////////////////////////////////////////////////////
	// SCREEN CAPTURE										/////////////////////////////////////////////////////////////////////
	//------------------------------------------------------/////////////////////////////////////////////////////////////////////
	//---------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------

	SL::Screen_Capture::ScreenCaptureManager	g_ss_framgrabber;
	bool										g_ss_isStartStreaming = false;
	bool										g_ss_isReceivedLastFrame = true;
	//---------------------------------------------------------------------------------------------------------------------------
	//--------------------OPTIONS------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------
	//											Server will wait until client received frame after send new frame. Default: true
	bool										g_ss_waitForClientReceived = true;
	u32											g_ss_waitForFrame = 10;
	u32											g_ss_currentWaitedFrame = 0;
	//---------------------------------------------------------------------------------------------------------------------------
	void										ss_initFrameGraber();
	void										ss_stopFrameGraber();
	void										ss_sendFrameToClient(const SL::Screen_Capture::Image& img);

public:
	//test
	void										test_initFrameGraber();


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
};

