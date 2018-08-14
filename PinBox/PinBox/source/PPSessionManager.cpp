#include "PPSessionManager.h"
#include "PPGraphics.h"
#include "ConfigManager.h"

PPSessionManager::PPSessionManager()
{
	g_AudioFrameMutex = new Mutex();
}

PPSessionManager::~PPSessionManager()
{
	delete g_AudioFrameMutex;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// For current displaying video frame
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void PPSessionManager::NewSession()
{
	_session = new PPSession();
	managerState = 0;
}

void PPSessionManager::StartStreaming(const char* ip)
{
	if (!strcmp(ip, "")) return;
	managerState = 1;
	initDecoder();
	_session->InitSession(this, ip, "1234");
}

void PPSessionManager::StopStreaming()
{
	_session->SendMsgStopSession();
}

void PPSessionManager::ProcessVideoFrame(u8* buffer, u32 size)
{
	u8* rgbBuffer = _decoder->appendVideoBuffer(buffer, size);
	if (rgbBuffer != nullptr) PPGraphics::Get()->UpdateTopScreenSprite(rgbBuffer, 393216, 400, 240);

	//---------------------------------------------------------
	// update frame video FPS
	//---------------------------------------------------------
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

void PPSessionManager::DrawVideoFrame()
{
	PPGraphics::Get()->DrawTopScreenSprite();
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

void PPSessionManager::initDecoder()
{
	_decoder = new PPDecoder();
	_decoder->initDecoder();
}

void PPSessionManager::releaseDecoder()
{
	_decoder->releaseDecoder();
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
