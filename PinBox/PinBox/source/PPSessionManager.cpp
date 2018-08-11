#include "PPSessionManager.h"
#include "PPGraphics.h"
#include "ConfigManager.h"

#define STATIC_FRAMES_POOL_SIZE 0x19000
#define STATIC_FRAME_SIZE 0x60000

PPSessionManager::PPSessionManager()
{
	g_VideoFrameMutex = new Mutex();
	g_AudioFrameMutex = new Mutex();
}


PPSessionManager::~PPSessionManager()
{
	delete g_VideoFrameMutex;
	delete g_AudioFrameMutex;
}

static u32 mFrameIndex = 0;
static u8* mStaticFrameBuffer = nullptr;
static volatile bool mHaveNewFrame = false;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SCREEN STREAM
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void PPSessionManager::NewSession()
{
	_session = new PPSession();
	managerState = 0;
}

void PPSessionManager::ProcessVideoFrame(u8* buffer, u32 size)
{
	g_VideoFrameMutex->Lock();
	u8* rgbBuffer = _decoder->appendVideoBuffer(buffer, size);
	if (rgbBuffer != nullptr)
	{
		// convert to correct format in 3DS
		int i, j;
		int nw3 = _decoder->iFrameWidth * 3;
		for (i = 0; i < _decoder->iFrameHeight; i++) {
			// i * 512 * 3
			memcpy(mStaticFrameBuffer + (i * 1536), rgbBuffer + (i*nw3), nw3);
		}
		mHaveNewFrame = true;
	}
	g_VideoFrameMutex->Unlock();

	// update frame video FPS
	if (!mReceivedFirstFrame)
	{
		mReceivedFirstFrame = true;
		mLastTimeGetFrame = osGetTime();
		mVideoFPS = 0.0f;
		mVideoFrame = 0;
	}
	else
	{
		mVideoFrame++;
		u64 deltaTime = osGetTime() - mLastTimeGetFrame;
		if (deltaTime > 1000)
		{
			mVideoFPS = mVideoFrame / (deltaTime / 1000.0f);
			mVideoFrame = 0;
			mLastTimeGetFrame = osGetTime();
		}
	}
}

void PPSessionManager::ProcessAudioFrame(u8* buffer, u32 size)
{
	g_AudioFrameMutex->Lock();
	_decoder->decodeAudioStream(buffer, size);
	g_AudioFrameMutex->Unlock();
}

void PPSessionManager::UpdateStreamSetting()
{
	//_session->SendMsgChangeSetting();
}

void PPSessionManager::StartDecodeThread()
{
	mStaticFrameBuffer = (u8*)linearAlloc(STATIC_FRAME_SIZE);
	_decoder = new PPDecoder();
	_decoder->initDecoder();
}

void PPSessionManager::ReleaseDecodeThead()
{
	if (mStaticFrameBuffer != nullptr) free(mStaticFrameBuffer);
	_decoder->releaseDecoder();
}

void PPSessionManager::UpdateVideoFrame()
{
	if (!mHaveNewFrame) return;
	g_VideoFrameMutex->Lock();
	PPGraphics::Get()->UpdateTopScreenSprite(mStaticFrameBuffer, 393216, 400, 240);
	mHaveNewFrame = false;
	g_VideoFrameMutex->Unlock();
}

void PPSessionManager::UpdateInputStream(u32 down, u32 up, short cx, short cy, short ctx, short cty)
{
	if (_session == nullptr) return;
	if(down != m_OldDown || up != m_OldUp || cx != m_OldCX || cy != m_OldCY || ctx != m_OldCTX || cty != m_OldCTY || !m_initInputFirstFrame)
	{
		if(_session->SendMsgSendInputData(down, up, cx, cy, ctx, cty))
		{
			m_initInputFirstFrame = true;
			m_OldDown = down;
			m_OldUp = up;
			m_OldCX = cx;
			m_OldCY = cy;
			m_OldCTX = ctx;
			m_OldCTY = cty;
		}
	}
}


void PPSessionManager::StartStreaming(const char* ip)
{
	if (!strcmp(ip, "")) return;
	managerState = 1;
	StartDecodeThread();
	_session->InitSession(this, ip, "1234");
}

void PPSessionManager::StopStreaming()
{
	_session->SendMsgStopSession();
}

void PPSessionManager::StartFPSCounter()
{
	mLastTime = osGetTime();
	mCurrentFPS = 0.0f;
	mFrames = 0;
}

void PPSessionManager::CollectFPSData()
{
	mFrames++;
	u64 deltaTime = osGetTime() - mLastTime;
	if(deltaTime > 1000)
	{
		mCurrentFPS = mFrames / (deltaTime / 1000.0f);
		mFrames = 0;
		mLastTime = osGetTime();
	}
}
