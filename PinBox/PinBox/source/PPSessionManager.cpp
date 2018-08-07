#include "PPSessionManager.h"
#include "PPGraphics.h"
#include "ConfigManager.h"

#define STATIC_FRAMES_POOL_SIZE 0x19000
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
		m_screenCaptureSessions.push_back(session);
	}
	mManagerState = 0;
}

void PPSessionManager::SafeTrack(u8* buffer, u32 size)
{
	m_frameTrackerMutex->Lock();
	//------------------------------------------------
	// move piece addr into new block
	//------------------------------------------------
	//printf("Tracked frame at : %d / %d\n", mStaticFramesIndex, STATIC_FRAMES_POOL_SIZE);
	memcpy(mStaticFramesPool, buffer, size);
	if (m_activeFrame == nullptr) m_activeFrame = new FrameData();
	m_activeFrame->pieceAddr = mStaticFramesPool;
	m_activeFrame->pieceSize = size;
	m_activeFrame->decoded = false;
	//------------------------------------------------
	m_frameTrackerMutex->Unlock();
	
}

u32 PPSessionManager::getFrameIndex() const
{
	return mFrameIndex;
}

void PPSessionManager::UpdateStreamSetting()
{
	m_screenCaptureSessions[0]->SS_ChangeSetting();
}

void UpdateFrameTracker(void *arg)
{
	PPSessionManager* self = (PPSessionManager*)arg;
	if (self == nullptr) {
		return;
	}
	printf("Starting decode thread....\n");
	//-----------------------------------------------------------------
	// init decoder
	//-----------------------------------------------------------------
	self->m_decoder = new PPDecoder();
	self->m_decoder->initDecoder();

	//---------------------------------------------------
	// this function should be run on a loop on main thread so that
	// we display image on main thread only
	//---------------------------------------------------

	u64 sleepDuration = 1000000ULL * 0.2;
	while (!self->g_threadExit) {

		// get first frame in tracker ( first frame alway is the newest )
		self->m_frameTrackerMutex->Lock();
		if(self->m_activeFrame != nullptr && !self->m_activeFrame->decoded)
		{
			self->m_activeFrame->decoded = true;
			//-------------------------------------------
			mFrameIndex++;

			//decode frame
			u8* rgbBuffer = self->m_decoder->appendVideoBuffer(self->m_activeFrame->pieceAddr, self->m_activeFrame->pieceSize);
			if(rgbBuffer != nullptr)
			{
				//printf("Decoded frame : %d\n", mFrameIndex);
				//-------------------------------------------
				// convert to correct format in 3DS
				//-------------------------------------------
				self->g_frameMutex->Lock();
				int i, j;
				int nw3 = self->m_decoder->iFrameWidth * 3;
				for (i = 0; i < self->m_decoder->iFrameHeight ; i++) {
					// i * 512 * 3
					memcpy(mStaticFrameBuffer + (i * 1536), rgbBuffer + (i*nw3), nw3);
				}
				self->g_frameMutex->Unlock();
				mHaveNewFrame = true;
			}

			//-------------------------------------------
			// update frame video FPS
			//-------------------------------------------
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
	printf("Releasing decode thread....\n");
	self->ReleaseDecodeThead();
}



void PPSessionManager::StartDecodeThread()
{
	mStaticFramesPool = (u8*)malloc(STATIC_FRAMES_POOL_SIZE);
	//-----------------------------------------------------------------
	// init static buffer for 1 frame only
	//-----------------------------------------------------------------
	mStaticPreAllocBuffer = (u8*)malloc(STATIC_FRAME_SIZE);
	mStaticFrameBuffer = (u8*)linearAlloc(STATIC_FRAME_SIZE);
	
	//-----------------------------------------------------------------
	// start thread
	//-----------------------------------------------------------------
	g_thread = threadCreate(UpdateFrameTracker, this, 100 * 1024, mMainThreadPrio - 2, -2, true);
}

void PPSessionManager::ReleaseDecodeThead()
{
	if(mStaticFramesPool!= nullptr) free(mStaticFramesPool);
	if (mStaticPreAllocBuffer != nullptr) free(mStaticPreAllocBuffer);
	if (mStaticFrameBuffer != nullptr) free(mStaticFrameBuffer);
	m_decoder->releaseDecoder();
}

void PPSessionManager::UpdateVideoFrame()
{
	if (!mHaveNewFrame) return;
	//printf("Uploading frame \n");

	g_frameMutex->Lock();
	PPGraphics::Get()->UpdateTopScreenSprite(mStaticFrameBuffer, 393216, 400, 240);
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
	m_screenCaptureSessions[index]->StartSession(ip, port, mMainThreadPrio - 2, [=](PPNetwork *self, u8* data, u32 code)
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
			m_inputStreamSession->StartSession(ip, "1234", mMainThreadPrio - 1, [=](PPNetwork *self, u8* data, u32 code)
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
