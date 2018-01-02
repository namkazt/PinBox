#include "PPSessionManager.h"
#include "PPGraphics.h"


PPSessionManager::PPSessionManager()
{
	m_frameTrackerMutex = new Mutex();
}


PPSessionManager::~PPSessionManager()
{
	delete m_frameTrackerMutex;
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
				//printf("Add new frame into pool : \n");
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

void PPSessionManager::UpdateFrameTracker()
{
	//---------------------------------------------------
	// this function should be run on a loop on main thread so that
	// we display image on main thread only
	//---------------------------------------------------
	u8* frameData = nullptr;
	u32 totalSize = 0;
	// get first frame in tracker ( first frame alway is the newest )
	m_frameTrackerMutex->Lock();

	auto iter = m_frameTracker.begin();
	if (iter != m_frameTracker.end())
	{
		FrameSet* set = iter->second;
		if (m_connectedSession == 1)
		{
			//--------------------------------------------------
			// incase only 1 session then it is all data we have
			FramePiece* firstPiece = set->pieces[0];
			if (firstPiece != nullptr) {
				frameData = (u8*)malloc(firstPiece->pieceSize);
				memcpy(frameData, firstPiece->piece, firstPiece->pieceSize);
				totalSize = firstPiece->pieceSize;
				// free piece data
				firstPiece->release();
				delete firstPiece;
				set->pieces.clear();
			}
		}
		else {
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
				totalSize = normalPieceSize * m_connectedSession + normalPieceSize;
				frameData = (u8*)malloc(totalSize);
			}
			else if (firstPiece->pieceSize < secondPiece->pieceSize)
			{
				normalPieceSize = firstPiece->pieceSize;
				lastPieceSize = secondPiece->pieceSize;
				totalSize = normalPieceSize * (m_connectedSession - 1) + lastPieceSize;
				frameData = (u8*)malloc(totalSize);
			}
			else
			{
				normalPieceSize = secondPiece->pieceSize;
				lastPieceSize = firstPiece->pieceSize;
				totalSize = normalPieceSize * (m_connectedSession - 1) + lastPieceSize;
				frameData = (u8*)malloc(totalSize);
			}
			//--------------------------------------------------
			memcpy(frameData + (firstPiece->pieceIndex *normalPieceSize), firstPiece->piece, firstPiece->pieceSize);
			memcpy(frameData + (secondPiece->pieceIndex *normalPieceSize), secondPiece->piece, secondPiece->pieceSize);
			// free piece data
			firstPiece->release();
			delete firstPiece;
			// free piece data
			secondPiece->release();
			delete secondPiece;
			//--------------------------------------------------
			for (int i = 2; i < m_connectedSession; i++)
			{
				FramePiece* piece = set->pieces[i];
				memcpy(frameData + (piece->pieceIndex *normalPieceSize), piece->piece, piece->pieceSize);
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
			totalSize = normalPieceSize * (m_connectedSession - 1) + lastPieceSize;
		}

		
		//--------------------------------------------------
		// clear frame tracker map from current iter
		++iter;
		for (;iter != m_frameTracker.end(); ++iter)
		{
			for(int i = 0; i < iter->second->receivedPieces; i++)
			{
				iter->second->pieces[i]->release();
			}
			iter->second->pieces.clear();
		}
		m_frameTracker.clear();
	}
	m_frameTrackerMutex->Unlock();
	//----------------------------------------------------

	if(frameData != nullptr)
	{
		//-------------------------------------------
		// init webp decode config
		u32 nW = 512, nH = 256;
		WebPDecoderConfig config;
		WebPInitDecoderConfig(&config);
		config.options.no_fancy_upsampling = 1;
		config.options.use_scaling = 1;
		config.options.scaled_width = nW;
		config.options.scaled_height = nH;
		if (!WebPGetFeatures(frameData, totalSize, &config.input) == VP8_STATUS_OK)
			return;
		config.output.colorspace = MODE_BGR;
		if (!WebPDecode(frameData, totalSize, &config) == VP8_STATUS_OK)
			return;
		//-------------------------------------------
		// draw
		PPGraphics::Get()->UpdateTopScreenSprite(config.output.private_memory, nW * nH * 3, nW, nH);
		//-------------------------------------------
		// free data
		free(frameData);
		WebPFreeDecBuffer(&config.output);
	}
	
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

void PPSessionManager::UpdateInputStream(u32 down, u32 hold, u32 up, short cx, short cy, short ctx, short cty)
{
	if (m_inputStreamSession == nullptr) return;
	if(down != m_OldDown || hold != m_OldHold || up != m_OldUp || cx != m_OldCX || cy != m_OldCY || ctx != m_OldCTX || cty != m_OldCTY || !m_initInputFirstFrame)
	{
		if(m_inputStreamSession->IN_SendInputData(down, hold, up, cx, cy, ctx, cty))
		{
			m_initInputFirstFrame = true;
			m_OldDown = down;
			m_OldHold = hold;
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
	m_screenCaptureSessions[index]->StartSession(ip, port, mMainThreadPrio, [=](PPNetwork *self, u8* data, u32 code)
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
	m_currentDisplayFrame = 0;
	m_frameTrackTemp.clear();
	m_frameTracker.clear();
	m_connectedSession = 0;

	//--------------------------------------------------
	// get current thread priority
	svcGetThreadPriority(&mMainThreadPrio, CUR_THREAD_HANDLE);
	_oneByOneConnectScreenCapture(m_connectedSession, ip, port, [=](int ret)
	{
		//--------------------------------------------------
		//start input connect when all other connect is done
		if (m_inputStreamSession != nullptr) {
			m_inputStreamSession->StartSession(ip, port, mMainThreadPrio, [=](PPNetwork *self, u8* data, u32 code)
			{
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