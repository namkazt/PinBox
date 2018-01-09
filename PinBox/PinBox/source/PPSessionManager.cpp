#include "PPSessionManager.h"
#include "PPGraphics.h"


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

void PPSessionManager::SafeTrack(FramePiece* piece)
{
	m_frameTrackerMutex->Lock();
	// update into tracker temp
	bool isNewFrame = true;
	for(int i = 0; i < m_frameTrackTemp.size(); i++)
	{
		if(m_frameTrackTemp[i]->frameIndex == piece->frameIndex)
		{
			m_frameTrackTemp[i]->pieces.push_back(piece);
			m_frameTrackTemp[i]->receivedPieces++;
			// if this frame is complete then move it to complete list
			if(m_frameTrackTemp[i]->receivedPieces == m_connectedSession)
			{
				m_frameTracker[m_frameTrackTemp[i]->frameIndex] = m_frameTrackTemp[i];
				// remove this track when all complete
				m_frameTrackTemp.erase(m_frameTrackTemp.begin() + i);
			}
			isNewFrame = false;
			break;
		}
	}
	if(isNewFrame)
	{
		FrameSet* set = new FrameSet();
		set->frameIndex = piece->frameIndex;
		set->receivedPieces = 1;
		set->pieces.push_back(piece);
		if(m_connectedSession > 1)
		{
			m_frameTrackTemp.push_back(set);
		}else
		{
			m_frameTracker[set->frameIndex] = set;
		}
	}
	m_frameTrackerMutex->Unlock();
	
}


static u8* mStaticPreAllocBuffer = nullptr;
static u8* mStaticFrameBuffer = nullptr;
static volatile bool mHaveNewFrame = false;

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
	webpDecodeConfig.output.colorspace = MODE_BGR;
	webpDecodeConfig.output.is_external_memory = 1;
	webpDecodeConfig.output.u.RGBA.size = 3 * 512 * 256;
	webpDecodeConfig.output.u.RGBA.rgba = mStaticPreAllocBuffer;
	webpDecodeConfig.output.u.RGBA.stride = 3 * nW;


	u64 sleepDuration = 1000000ULL * 1;
	while (!self->g_threadExit) {



		bool frameAvailable = false;
		u32 totalSize = 0;
		// get first frame in tracker ( first frame alway is the newest )
		self->m_frameTrackerMutex->Lock();
		std::map<int, FrameSet*>::reverse_iterator iter = self->m_frameTracker.rbegin();
		if (iter != self->m_frameTracker.rend())
		{
			FrameSet* set = iter->second;
			self->m_frameTrackerMutex->Unlock();

			if (self->m_connectedSession == 1)
			{
				//--------------------------------------------------
				// incase only 1 session then it is all data we have
				FramePiece* firstPiece = set->pieces[0];
				if (firstPiece != nullptr) {
					//memcpy(mStaticPreAllocBuffer, firstPiece->piece, firstPiece->pieceSize);
					totalSize = firstPiece->pieceSize;
					if (!WebPGetFeatures(firstPiece->piece, totalSize, &webpDecodeConfig.input) == VP8_STATUS_OK)
					{
						WebPFreeDecBuffer(&webpDecodeConfig.output);
						return;
					}
					if (!WebPDecode(firstPiece->piece, totalSize, &webpDecodeConfig) == VP8_STATUS_OK)
					{
						WebPFreeDecBuffer(&webpDecodeConfig.output);
						return;
					}
					// free piece data
					firstPiece->release();
					delete firstPiece;
					set->pieces.clear();
					frameAvailable = true;
				}
			}
			else {

				WebPIDecoder* idec = WebPINewDecoder(&webpDecodeConfig.output);

				//--------------------------------------------------
				// generate frame
				u32 normalPieceSize = 0;
				u32 lastPieceSize = 0;
				// we get first 2 piece to use as data to guess message size
				FramePiece* firstPiece = set->pieces[0];
				FramePiece* secondPiece = set->pieces[1];
				//--------------------------------------------------
				if (firstPiece->pieceSize == secondPiece->pieceSize)
				{
					// this case we only know normal piece size
					normalPieceSize = firstPiece->pieceSize;
					// we alloc more than 1 pieces size to ensure it enough memory
					totalSize = normalPieceSize * self->m_connectedSession + normalPieceSize;
				}
				else if (firstPiece->pieceSize < secondPiece->pieceSize)
				{
					normalPieceSize = firstPiece->pieceSize;
					lastPieceSize = secondPiece->pieceSize;
					totalSize = normalPieceSize * (self->m_connectedSession - 1) + lastPieceSize;
				}
				else
				{
					normalPieceSize = secondPiece->pieceSize;
					lastPieceSize = firstPiece->pieceSize;
					totalSize = normalPieceSize * (self->m_connectedSession - 1) + lastPieceSize;
				}
				//--------------------------------------------------
				WebPIAppend(idec, firstPiece->piece, firstPiece->pieceSize);
				WebPIAppend(idec, secondPiece->piece, secondPiece->pieceSize);
				//--------------------------------------------------
				for (int i = 2; i < self->m_connectedSession; i++)
				{
					FramePiece* piece = set->pieces[i];
					//memcpy(mStaticPreAllocBuffer + (piece->pieceIndex *normalPieceSize), piece->piece, piece->pieceSize);
					WebPIAppend(idec, piece->piece, piece->pieceSize);

					if (piece->pieceSize != normalPieceSize)
					{
						lastPieceSize = piece->pieceSize;
					}
					// free piece data
					piece->release();
					delete piece;
				}
				set->pieces.clear();
				//--------------------------------------------------
				if (lastPieceSize == 0) lastPieceSize = normalPieceSize;
				totalSize = normalPieceSize * (self->m_connectedSession - 1) + lastPieceSize;


				//memcpy(mStaticPreAllocBuffer + (firstPiece->pieceIndex *normalPieceSize), firstPiece->piece, firstPiece->pieceSize);
				//memcpy(mStaticPreAllocBuffer + (secondPiece->pieceIndex *normalPieceSize), secondPiece->piece, secondPiece->pieceSize);
				// free piece data
				firstPiece->release();
				delete firstPiece;
				// free piece data
				secondPiece->release();
				delete secondPiece;
				
				WebPIDelete(idec);
				frameAvailable = true;
			}
			self->m_frameTrackerMutex->Lock();
			self->m_frameTracker.erase(std::next(iter).base());
			self->m_frameTrackerMutex->Unlock();
		}else
		{
			self->m_frameTrackerMutex->Unlock();
		}
		
		//----------------------------------------------------

		if (frameAvailable)
		{
			
			//-------------------------------------------
			// draw
			//
			self->g_frameMutex->Lock();
			int i, j;
			int nw3 = nW * 3;
			for (i = 0; i < nH; i++) {
				memmove(mStaticFrameBuffer + (i * 1536), mStaticPreAllocBuffer + (i*nw3), nw3);
			}
			self->g_frameMutex->Unlock();
			mHaveNewFrame = true;
			
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

		svcSleepThread(sleepDuration);
	}

	//printf("[Decoder] thread exit.\n");
	//gfxFlushBuffers();
}



void PPSessionManager::StartDecodeThread()
{
	int mSize = 3 * 512 * 256;
	mStaticPreAllocBuffer = (u8*)malloc(mSize);
	mStaticFrameBuffer = (u8*)linearAlloc(mSize);
	memset(mStaticFrameBuffer, 0, mSize);
	//printf("Start decoder thread.\n");
	//gfxFlushBuffers();

	g_thread = threadCreate(UpdateFrameTracker, this, 3 * 1024, mMainThreadPrio - 1, -2, true);
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


void PPSessionManager::StartStreaming(const char* ip, const char* port)
{
	if (!strcmp(ip, "") || !strcmp(port, "")) return;

	m_currentDisplayFrame = 0;
	m_frameTrackTemp.clear();
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
	_oneByOneConnectScreenCapture(m_connectedSession, ip, port, [=](int ret)
	{
		//--------------------------------------------------
		//start input connect when all other connect is done
		if (m_inputStreamSession != nullptr) {
			m_inputStreamSession->StartSession(ip, port, mMainThreadPrio - 3, [=](PPNetwork *self, u8* data, u32 code)
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
