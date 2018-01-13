#include "PPSessionManager.h"
#include "PPGraphics.h"
#include "ConfigManager.h"

#define STATIC_FRAMES_POOL_SIZE 0x500000
#define STATIC_FRAME_SIZE 0x60000

PPSessionManager::PPSessionManager()
{
	m_frameTrackerMutex = new Mutex();
	g_frameMutex = new Mutex();
}


PPSessionManager::~PPSessionManager()
{
	delete m_frameTrackerMutex;
	delete g_frameMutex;
}

static u32 mFrameIndex = 0;
static u8* mStaticFramesPool = nullptr;
static u32 mStaticFramesIndex = 0;
static u8* mStaticPreAllocBuffer = nullptr;
static u8* mStaticFrameBuffer = nullptr;
static volatile bool mHaveNewFrame = false;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SCREEN STREAM
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void PPSessionManager::InitScreenCapture(u32 numberOfSessions)
{
	if (numberOfSessions <= 0) numberOfSessions = 1;
	if (m_screenCaptureSessions.size() > 0) return;
	m_screenCaptureSessions = std::vector<PPSession*>();
	for(int i = 1; i <= numberOfSessions; i++)
	{
		PPSession* session = new PPSession();
		session->sessionID = i;
		session->InitScreenCaptureSession(this);
		session->SS_setting_smoothStepFrames = ConfigManager::Get()->_cfg_skip_frame;
		session->SS_setting_sourceQuality = ConfigManager::Get()->_cfg_video_quality;
		session->SS_setting_sourceScale = ConfigManager::Get()->_cfg_video_scale;
		session->SS_setting_waitToReceivedFrame = ConfigManager::Get()->_cfg_wait_for_received;
		m_screenCaptureSessions.push_back(session);
	}
	mManagerState = 0;
}

void PPSessionManager::SafeTrack(FramePiece* piece)
{
	m_frameTrackerMutex->Lock();

	if(mStaticFramesIndex + piece->pieceSize > STATIC_FRAMES_POOL_SIZE)
	{
		// move the cursor to the start of memory block and start again
		mStaticFramesIndex = 0;
	}

	// move piece addr into new block
	memmove(mStaticFramesPool + mStaticFramesIndex, piece->pieceAddr, piece->pieceSize);
	piece->pieceAddr = mStaticFramesPool + mStaticFramesIndex;
	piece->pieceStart = mStaticFramesIndex;
	mStaticFramesIndex += piece->pieceSize;
	//m_frameTracker[piece->frameIndex] = piece;

	m_activeFramePiece = piece;

	m_frameTrackerMutex->Unlock();
	
}

u32 PPSessionManager::getFrameIndex() const
{
	return mFrameIndex;
}

void UpdateFrameTracker(void *arg)
{
	//printf("[Decoder] Start thread.\n");
	//gfxFlushBuffers();

	PPSessionManager* self = (PPSessionManager*)arg;
	if (self == nullptr) {
		//printf("[Decoder] invalid session manager.\n");
		//gfxFlushBuffers();
		return;
	}
	//---------------------------------------------------
	// this function should be run on a loop on main thread so that
	// we display image on main thread only
	//---------------------------------------------------
	//-------------------------------------------
	// init webp decode config=
	u32 nW = 400, nH = 240;
	WebPDecoderConfig webpDecodeConfig;
	WebPInitDecoderConfig(&webpDecodeConfig);
	webpDecodeConfig.options.no_fancy_upsampling = 1;
	webpDecodeConfig.options.bypass_filtering = 1;
	webpDecodeConfig.output.colorspace = MODE_BGR;
	webpDecodeConfig.output.is_external_memory = 1;
	webpDecodeConfig.output.u.RGBA.size = STATIC_FRAME_SIZE;
	webpDecodeConfig.output.u.RGBA.rgba = mStaticPreAllocBuffer;
	webpDecodeConfig.output.u.RGBA.stride = 3 * nW;


	u64 sleepDuration = 1000000ULL * 16;
	while (!self->g_threadExit) {

		// get first frame in tracker ( first frame alway is the newest )
		self->m_frameTrackerMutex->Lock();
		if(self->m_activeFramePiece != nullptr && mFrameIndex != self->m_activeFramePiece->frameIndex)
		{
			mFrameIndex = self->m_activeFramePiece->frameIndex;
			//-------------------------------------------
			// decode frame
			if (!WebPGetFeatures(self->m_activeFramePiece->pieceAddr, self->m_activeFramePiece->pieceSize, &webpDecodeConfig.input) == VP8_STATUS_OK)
			{
				WebPFreeDecBuffer(&webpDecodeConfig.output);
				return;
			}
			if (!WebPDecode(self->m_activeFramePiece->pieceAddr, self->m_activeFramePiece->pieceSize, &webpDecodeConfig) == VP8_STATUS_OK)
			{
				WebPFreeDecBuffer(&webpDecodeConfig.output);
				return;
			}

			//-------------------------------------------
			// convert to correct format
			self->g_frameMutex->Lock();
			int i, j;
			int nw3 = nW * 3;
			for (i = 0; i < nH; i++) {
				memmove(mStaticFrameBuffer + (i * 1536), mStaticPreAllocBuffer + (i*nw3), nw3);
			}
			self->g_frameMutex->Unlock();
			mHaveNewFrame = true;

			//-------------------------------------------
			// update frame video FPS
			if (!self->mReceivedFirstFrame)
			{
				self->mReceivedFirstFrame = true;
				self->mLastTimeGetFrame = osGetTime();
				self->mVideoFPS = 0.0f;
				self->mVideoFrame = 0;
			}
			else
			{
				self->mVideoFrame++;
				u64 deltaTime = osGetTime() - self->mLastTimeGetFrame;
				if (deltaTime > 1000)
				{
					self->mVideoFPS = self->mVideoFrame / (deltaTime / 1000.0f);
					self->mVideoFrame = 0;
					self->mLastTimeGetFrame = osGetTime();
				}

			}
		}
		self->m_frameTrackerMutex->Unlock();

		svcSleepThread(sleepDuration);
	}

	self->ReleaseDecodeThead();
}



void PPSessionManager::StartDecodeThread()
{
	mStaticFramesPool = (u8*)malloc(STATIC_FRAMES_POOL_SIZE);
	mStaticFramesIndex = 0;
	//-----------------------------------------------------------------
	// init static buffer for 1 frame only
	mStaticPreAllocBuffer = (u8*)malloc(STATIC_FRAME_SIZE);
	mStaticFrameBuffer = (u8*)linearAlloc(STATIC_FRAME_SIZE);
	memset(mStaticFrameBuffer, 0, STATIC_FRAME_SIZE);

	g_thread = threadCreate(UpdateFrameTracker, this, 3 * 1024, mMainThreadPrio - 1, -2, true);
}

void PPSessionManager::ReleaseDecodeThead()
{
	free(mStaticFramesPool);
	free(mStaticPreAllocBuffer);
	free(mStaticFrameBuffer);
}

void PPSessionManager::UpdateVideoFrame()
{
	if (!mHaveNewFrame) return;

	g_frameMutex->Lock();
	u32 nW = 400, nH = 240;
	PPGraphics::Get()->UpdateTopScreenSprite(mStaticFrameBuffer, nW * nH * 3, nW, nH);
	mHaveNewFrame = false;
	g_frameMutex->Unlock();
}


void PPSessionManager::_startStreaming()
{
	m_screenCaptureSessions[0]->SS_ChangeSetting();
	m_screenCaptureSessions[0]->SS_StartStream();
	for (int i = 1; i < m_screenCaptureSessions.size(); i++)
	{
		m_screenCaptureSessions[i]->RequestForheader();
	}
	
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// INPUT
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void PPSessionManager::InitInputStream()
{
	if (m_inputStreamSession != nullptr) return;
	m_inputStreamSession = new PPSession();
	m_inputStreamSession->sessionID = 0;
	m_inputStreamSession->InitInputCaptureSession(this);
	m_initInputFirstFrame = false;
}

void PPSessionManager::UpdateInputStream(u32 down, u32 up, short cx, short cy, short ctx, short cty)
{
	if (m_inputStreamSession == nullptr) return;
	if(down != m_OldDown || up != m_OldUp || cx != m_OldCX || cy != m_OldCY || ctx != m_OldCTX || cty != m_OldCTY || !m_initInputFirstFrame)
	{
		if(m_inputStreamSession->IN_SendInputData(down, up, cx, cy, ctx, cty))
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// COMMON
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void PPSessionManager::_oneByOneConnectScreenCapture(int index, const char* ip, const char* port, PPNotifyCallback callback)
{
	m_screenCaptureSessions[index]->StartSession(ip, port, mMainThreadPrio - 4, [=](PPNetwork *self, u8* data, u32 code)
	{
		m_connectedSession++;
		if (m_connectedSession == m_screenCaptureSessions.size())
		{
			if (callback != nullptr) callback(0);
		}else
		{
			_oneByOneConnectScreenCapture(m_connectedSession, ip, port, callback);
		}
	});
}


void PPSessionManager::StartStreaming(const char* ip)
{
	if (!strcmp(ip, "")) return;

	m_currentDisplayFrame = 0;
	m_frameTracker.clear();
	m_connectedSession = 0;

	mManagerState = 1;

	//--------------------------------------------------
	// get current thread priority
	svcGetThreadPriority(&mMainThreadPrio, CUR_THREAD_HANDLE);

	//------------------------------------------------
	// start video decode thread
	StartDecodeThread();

	//------------------------------------------------
	_oneByOneConnectScreenCapture(m_connectedSession, ip, "1234", [=](int ret)
	{
		//--------------------------------------------------
		//start input connect when all other connect is done
		if (m_inputStreamSession != nullptr) {
			m_inputStreamSession->StartSession(ip, "1234", mMainThreadPrio - 3, [=](PPNetwork *self, u8* data, u32 code)
			{
				mManagerState = 2;
				m_inputStreamSession->IN_Start();
				//--------------------------------------------------
				// when all session is authenticated
				// start streaming here
				printf("[Success] All clients is connected to server \n");
				gfxFlushBuffers();
				_startStreaming();
			});
		}
	});

	
}

void PPSessionManager::StopStreaming()
{
	for (int i = 0; i < m_screenCaptureSessions.size(); i++)
	{
		m_screenCaptureSessions[i]->SS_StopStream();
	}

	if (m_inputStreamSession != nullptr)
	{
		m_inputStreamSession->IN_Stop();
		m_inputStreamSession->CloseSession();
	}
}

void PPSessionManager::Close()
{
	for (int i = 0; i < m_screenCaptureSessions.size(); i++)
	{
		m_screenCaptureSessions[i]->CloseSession();
	}

	if (m_inputStreamSession != nullptr)
	{
		m_inputStreamSession->CloseSession();
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
