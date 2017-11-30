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

enum PPSession_Type { PPSESSION_NONE, PPSESSION_MOVIE, PPSESSION_SCREEN_CAPTURE, PPSESSION_INPUT_CAPTURE};


#define PPREQUEST_AUTHEN 50
#define PPREQUEST_HEADER 10
#define PPREQUEST_BODY 15

class PPSession
{
private:
	PPSession_Type					g_sessionType = PPSESSION_NONE;
	PPNetwork*						g_network = nullptr;

	bool							g_authenticated = false;
private:
	void initSession();

public:
	~PPSession();

	void InitMovieSession();
	void InitScreenCaptureSession();
	void InitInputCaptureSession();

	void StartSession();
	void CloseSession();
};

#endif