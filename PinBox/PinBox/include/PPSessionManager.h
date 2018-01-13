#pragma once
#include <vector>
#include "PPSession.h"
#include <webp/decode.h>
#include "opusfile.h"
#include <map>
#include <stack>


typedef std::function<void(int code)> PPNotifyCallback;

class PPSessionManager
{
public:

	PPSession*										m_inputStreamSession = nullptr;
	u32												m_OldDown;
	u32												m_OldUp;
	short											m_OldCX;
	short											m_OldCY;
	short											m_OldCTX;
	short											m_OldCTY;
	bool											m_initInputFirstFrame = false;

	s32												mMainThreadPrio = 0;

	u64												mLastTime = 0;
	float											mCurrentFPS = 0.0f;
	u32												mFrames = 0;

	u64												mLastTimeGetFrame = 0;
	float											mVideoFPS = 0.0f;
	u32												mVideoFrame = 0;
	bool											mReceivedFirstFrame = false;

	Thread											g_thread = nullptr;
	volatile bool									g_threadExit = false;
	Mutex*											g_frameMutex;

	std::vector<PPSession*>							m_screenCaptureSessions;
	int												m_commandSessionIndex = 0;
	u32												m_connectedSession = 0;
	void											_startStreaming();
	void											_oneByOneConnectScreenCapture(int index, const char* ip, const char* port, PPNotifyCallback callback);

	std::map<int, FramePiece*>						m_frameTracker;
	FramePiece*										m_activeFramePiece = nullptr;
	Mutex*											m_frameTrackerMutex;
	u32												m_currentDisplayFrame = 0;
	//------------------------------------------
	// UI ref variables
	//------------------------------------------
	int												mManagerState = -1;
	char											mIPAddress[100] = "";
public:
	PPSessionManager();
	~PPSessionManager();

	void InitScreenCapture(u32 numberOfSessions);
	void StartStreaming(const char* ip);
	void StopStreaming();
	void Close();

	void StartFPSCounter();
	void CollectFPSData();

	void InitInputStream();
	void UpdateInputStream(u32 down, u32 up, short cx, short cy, short ctx, short cty);

	void SafeTrack(FramePiece* piece);
	void StartDecodeThread();
	void ReleaseDecodeThead();
	void UpdateVideoFrame();

	int GetManagerState() const { return mManagerState; };
	float GetFPS() const { return mCurrentFPS; }
	float GetVideoFPS() const { return mVideoFPS; }
	int GetFrameLeft() const { return m_frameTracker.size(); }
	u32 getFrameIndex() const;
	//------------------------------------------
	// UI ref functions
	//------------------------------------------
	char* getIPAddress() { return &mIPAddress[0]; }
	void setIPAddress(const char* ip) { snprintf(mIPAddress, sizeof mIPAddress, "%s", ip); }
};

