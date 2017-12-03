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

#include "PPNetwork.h"
#include <webp/decode.h>
#include "opusfile.h"
#include "Mutex.h"
#include <map>


enum PPSession_Type { PPSESSION_NONE, PPSESSION_MOVIE, PPSESSION_SCREEN_CAPTURE, PPSESSION_INPUT_CAPTURE};

#define MSG_COMMAND_SIZE 9

#define PPREQUEST_AUTHEN 50
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
#define MSG_CODE_REQUEST_NEW_AUDIO_FRAME 18
#define MSG_CODE_REQUEST_RECEIVED_AUDIO_FRAME 19


#define AUDIO_CHANNEL	0x08
typedef struct
{
	u8			*start;
	u32			size;
	u32			width;
	u32			height;
} QueueFrame;

typedef struct
{
	u32 frameIndex;
	u32 pieceIndex;
	u8* piece;
	u32 pieceSize;
	void release() { if (piece != nullptr) linearFree(piece); }

} FramePiece;
class PPSessionManager;

class PPSession
{
private:
	PPSessionManager				*g_manager;
	PPSession_Type					g_sessionType = PPSESSION_NONE;
	PPNetwork*						g_network = nullptr;
	PPMessage*						g_tmpMessage = nullptr;
	bool							g_authenticated = false;
	PPNetworkCallback				g_onAuthenSuccessed = nullptr;

private:
	void initSession();

	void processMovieSession(u8* buffer, size_t size);
	void processScreenCaptureSession(u8* buffer, size_t size);
	void processInputSession(u8* buffer, size_t size);

public:
	int								sessionID = -1;
	~PPSession();

	void InitMovieSession();
	void InitScreenCaptureSession(PPSessionManager* manager);
	void InitInputCaptureSession();

	void StartSession(const char* ip, const char* port, PPNetworkCallback authenSuccessed);
	void CloseSession();

	//-----------------------------------------------------
	// screen capture
	//-----------------------------------------------------
private:
	//----------------------------------------------------------------------
	// profile setting
	typedef struct
	{
		std::string profileName = "Default";
		bool waitToReceivedFrame = false;
		u32 smoothStepFrames = 0;
		u32 sourceQuality = 100;
		u32 sourceScale = 100;
	} SSProfile;
	//----------------------------------------------------------------------
	bool								SS_v_isStartStreaming = false;
	bool								SS_setting_waitToReceivedFrame = true;
	u32									SS_setting_smoothStepFrames = 2;		// this setting allow frame switch smoother if there is delay when received frame
	u32									SS_setting_sourceQuality = 50;			// webp quality control
	u32									SS_setting_sourceScale = 75;			// frame size control eg: 75% = 0.75 of real size
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	// on each frame - each session store only 1 piece as a piece frame object
	std::map<u32, FramePiece*>			SS_framePiecesCached;
	Mutex*								SS_frameCachedMutex;

public:
	void								SS_StartStream();
	void								SS_StopStream();
	void								SS_ChangeSetting();

	void								SS_Reset();

	//QueueFrame*							SS_PopPendingQueue();
	FramePiece*							SafeGetFramePiece(u32 index);
	void								RequestForheader();
	//-----------------------------------------------------
	// movie
	//-----------------------------------------------------


	//-----------------------------------------------------
	// input
	//-----------------------------------------------------
};

#endif