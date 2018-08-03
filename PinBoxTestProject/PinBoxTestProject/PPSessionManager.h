#pragma once
#include <vector>
#include "PPSession.h"
#include <webp/decode.h>
#include "opusfile.h"
#include "PPDecoder.h"

#ifdef _WIN32 
//===========================================================
// for test only
// openCV
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
//===========================================================
#endif

#define VideoBufferSize  0xA00000

class PPSessionManager
{
private:
	PPDecoder*										m_decoder;
	std::vector<PPSession*>							m_screenCaptureSessions;
	int												m_commandSessionIndex = 0;
	u32												m_connectedSession = 0;
	void											_startStreaming();
	std::map<u32, u32>								m_frameTracker;
	std::mutex										m_frameTrackerMutex;
	u32												m_currentDisplayFrame = 0;
public:
	PPSessionManager();
	~PPSessionManager();

	u8* m_staticVideoBuffer = nullptr;
	u32 m_videoBufferCursor = 0;
	u32 m_videoBufferSize = 0;


	void InitScreenCapture(u32 numberOfSessions);
	void StartStreaming(const char* ip, const char* port);
	void StopStreaming();
	void Close();

	void AppendBuffer(u8* buffer, u32 size);

	void DecodeVideo();
};

