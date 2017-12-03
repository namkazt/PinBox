#pragma once
#include <vector>
#include "PPSession.h"
#include <webp/decode.h>
#include "opusfile.h"

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

class PPSessionManager
{
private:
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

	void InitScreenCapture(u32 numberOfSessions);
	void StartStreaming(const char* ip, const char* port);
	void StopStreaming();
	void Close();

	void SafeTrack(u32 index);
	void UpdateFrameTracker();
};

