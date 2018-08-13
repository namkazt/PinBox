#pragma once
#include <vector>
#include "PPSession.h"
#include <webp/decode.h>
#include <turbojpeg.h>
#include "opusfile.h"
#include <map>
#include <stack>
#include "PPDecoder.h"

#include "constant.h"


typedef std::function<void(int code)> PPNotifyCallback;

typedef struct
{
	u8		*buffer;
	u32			size;
} VideoFrame;


class PPSessionManager
{
public:
	PPSession*										_session = nullptr;

	// input store
	u32												m_OldDown;
	u32												m_OldUp;
	short											m_OldCX;
	short											m_OldCY;
	short											m_OldCTX;
	short											m_OldCTY;
	bool											m_initInputFirstFrame = false;

	// fps calculate
	u64												mLastTime = 0;
	float											mCurrentFPS = 0.0f;
	u32												mFrames = 0;

	u64												mLastTimeGetFrame = 0;
	float											mVideoFPS = 0.0f;
	u32												mVideoFrame = 0;
	bool											mReceivedFirstFrame = false;

	Mutex*											g_AudioFrameMutex;


	PPDecoder*										_decoder = nullptr;
	//------------------------------------------
	// UI ref variables
	//------------------------------------------
	int												managerState = -1;
	char											mIPAddress[100] = "192.168.31.183";
public:
	PPSessionManager();
	~PPSessionManager();

	void NewSession();

	void StartStreaming(const char* ip);
	void StopStreaming();

	void StartFPSCounter();
	void CollectFPSData();

	
	void UpdateInputStream(u32 down, u32 up, short cx, short cy, short ctx, short cty);
	void ProcessVideoFrame(u8* buffer, u32 size);
	void ProcessAudioFrame(u8* buffer, u32 size);

	void initDecoder();
	void releaseDecoder();

	void DrawVideoFrame();

	int GetManagerState() const { return managerState; };
	void SetManagerState(int v) { managerState = v; };
	float GetFPS() const { return mCurrentFPS; }
	float GetVideoFPS() const { return mVideoFPS; }


	void UpdateStreamSetting();
	//------------------------------------------
	// UI ref functions
	//------------------------------------------
	char* getIPAddress() { return &mIPAddress[0]; }
	void setIPAddress(const char* ip) { snprintf(mIPAddress, sizeof mIPAddress, "%s", ip); }
};

