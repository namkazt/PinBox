#include "PPSessionManager.h"
#include "PPGraphics.h"
#include "ConfigManager.h"

#define STATIC_FRAMES_POOL_SIZE 0x100000
#define STATIC_FRAME_SIZE 0x60000

PPSessionManager::PPSessionManager()
{
	_videoFrameMutex = new Mutex();
	g_AudioFrameMutex = new Mutex();
}

PPSessionManager::~PPSessionManager()
{
	delete _videoFrameMutex;
	delete g_AudioFrameMutex;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// For current displaying video frame
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static u8* g_currentFrameBuffer = nullptr;
static u8* g_continuosBuffer = nullptr;
static u32 g_continuosCursor = 0;

// Frameskip config
static volatile bool g_autoSkipFrame = true;
static volatile int g_frameSkip = 1;
static volatile int g_currentFrame = 0;
static volatile int g_frameRate = 30;
static volatile int g_frameSkipAtStep = static_cast<int>(g_frameRate / g_frameSkip); // should be calculated

void PPSessionManager::NewSession()
{
	_session = new PPSession();
	managerState = 0;
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

void PPSessionManager::ProcessVideoFrame(u8* buffer, u32 size)
{
	//---------------------------------------------------------
	// PROCESS WITH BUFFER
	//---------------------------------------------------------
	
	//NOTE: there is some case that maybe override old data that not use yet
	// reset pointer
	if(g_continuosCursor + size >= STATIC_FRAMES_POOL_SIZE)
		g_continuosCursor = 0;

	// copy data to buffer
	memcpy(g_continuosBuffer + g_continuosCursor, buffer, size);

	VideoFrame* frame = new VideoFrame();
	frame->buffer = g_continuosBuffer + g_continuosCursor;
	frame->size = size;
	g_continuosCursor += size;

	_videoFrameMutex->Lock();
	_videoQueue.push(frame);
	_videoFrameMutex->Unlock();

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

void PPSessionManager::UpdateVideoFrame()
{
	_videoFrameMutex->Lock();
	if(!_videoQueue.empty())
	{
		VideoFrame* frame = nullptr;
		int frameSkipped = 0;
		//TODO: skip frame here
		frame = _videoQueue.front();
		_videoQueue.pop();

		g_currentFrame++;
		if (g_autoSkipFrame && g_currentFrame % g_frameSkipAtStep == 0)
		{
			delete frame;
			_videoFrameMutex->Unlock();
			if (g_currentFrame == g_frameRate) g_currentFrame = 0;
			return;
		}
		_videoFrameMutex->Unlock();
		if (g_currentFrame == g_frameRate) g_currentFrame = 0;


		u8* rgbBuffer = _decoder->appendVideoBuffer(frame->buffer, frame->size);
		if (rgbBuffer != nullptr)
		{
			// convert to correct format in 3DS
			int nw3 = _decoder->iFrameWidth * 3;
			for (int i = 0; i < _decoder->iFrameHeight; i++) {
				// i * 512 * 3
				memcpy(g_currentFrameBuffer + (i * 1536), rgbBuffer + (i*nw3), nw3);
			}
#ifndef CONSOLE_DEBUG
			PPGraphics::Get()->UpdateTopScreenSprite(g_currentFrameBuffer, 393216, 400, 240);
#endif
		}

		delete frame;
	} else _videoFrameMutex->Unlock();
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
	_videoQueue = std::queue<VideoFrame*>();
	g_continuosBuffer = (u8*)malloc(STATIC_FRAMES_POOL_SIZE);
	g_continuosCursor = 0;
	g_currentFrameBuffer = (u8*)linearAlloc(STATIC_FRAME_SIZE);
	_decoder = new PPDecoder();
	_decoder->initDecoder();
}

void PPSessionManager::ReleaseDecodeThead()
{
	while (!_videoQueue.empty())
	{
		VideoFrame* queueMsg = (VideoFrame*)_videoQueue.front();
		_videoQueue.pop();
		delete queueMsg;
	}
	std::queue<VideoFrame*>().swap(_videoQueue);
	if (g_continuosBuffer != nullptr) free(g_continuosBuffer);
	if (g_currentFrameBuffer != nullptr) free(g_currentFrameBuffer);
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
