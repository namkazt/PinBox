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

void PPSessionManager::InitScreenCapture(u32 numberOfSessions)
{
	if (numberOfSessions <= 0) numberOfSessions = 1;
	if (m_screenCaptureSessions.size() > 0) return;
	m_screenCaptureSessions = std::vector<PPSession*>();
	for(int i = 0; i < numberOfSessions; i++)
	{
		PPSession* session = new PPSession();
		session->sessionID = i;
		session->InitScreenCaptureSession(this);
		m_screenCaptureSessions.push_back(session);
	}
}

void PPSessionManager::StartStreaming(const char* ip, const char* port)
{
	m_currentDisplayFrame = 0;
	m_frameTracker.clear();
	m_connectedSession = 0;
	for (int i = 0; i < m_screenCaptureSessions.size(); i++)
	{
		// start connect all session to server
		m_screenCaptureSessions[i]->StartSession(ip, port, [=](u8* data, u32 code)
		{
			printf("#%d: Authen add ", i);
			m_connectedSession++;
			if(m_connectedSession == m_screenCaptureSessions.size())
			{
				// when all session is authenticated
				// start streaming here
				//std::cout << "All sessions is connected to server : Start Streaming" << std::endl;
				_startStreaming();
			}
		});
	}
}

void PPSessionManager::StopStreaming()
{
	for (int i = 0; i < m_screenCaptureSessions.size(); i++)
	{
		m_screenCaptureSessions[i]->SS_StopStream();
	}
}

void PPSessionManager::Close()
{
	for (int i = 0; i < m_screenCaptureSessions.size(); i++)
	{
		m_screenCaptureSessions[i]->CloseSession();
	}
}

void PPSessionManager::SafeTrack(u32 index)
{
	m_frameTrackerMutex->Lock();
	auto iter = m_frameTracker.find(index);
	if(iter == m_frameTracker.end())
	{
		m_frameTracker.insert(std::pair<u32, u32>(index, 1));
		auto lastiter = m_frameTracker.find(index - 1);
		if(lastiter != m_frameTracker.end())
		{
			m_frameTracker.erase(lastiter);
		}
	}else
	{
		iter->second++;
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
	// get first frame in tracker
	m_frameTrackerMutex->Lock();
	auto iter = m_frameTracker.begin();
	if (iter != m_frameTracker.end())
	{
		if(iter->second == m_connectedSession)
		{
			if(m_connectedSession == 1)
			{
				//--------------------------------------------------
				// incase only 1 session then it is all data we have
				FramePiece* firstPiece = m_screenCaptureSessions[0]->SafeGetFramePiece(iter->first);
				frameData = (u8*)malloc(firstPiece->pieceSize);
				memcpy(frameData, firstPiece->piece, firstPiece->pieceSize);
				totalSize = firstPiece->pieceSize;
				// free piece data
				firstPiece->release();
				delete firstPiece;
			}
			else {
				//--------------------------------------------------
				// generate frame
				u32 normalPieceSize = 0;
				u32 lastPieceSize = 0;
				FramePiece* firstPiece = m_screenCaptureSessions[0]->SafeGetFramePiece(iter->first);
				FramePiece* secondPiece = m_screenCaptureSessions[1]->SafeGetFramePiece(iter->first);
				//--------------------------------------------------
				if(firstPiece->pieceSize == secondPiece->pieceSize)
				{
					// this case we only know normal piece size
					normalPieceSize = firstPiece->pieceSize;
					// we alloc more than 1 pieces size to ensure it enough memory
					totalSize = normalPieceSize * m_connectedSession + normalPieceSize;
					frameData = (u8*)malloc(totalSize);
				}else if(firstPiece->pieceSize < secondPiece->pieceSize)
				{
					normalPieceSize = firstPiece->pieceSize;
					lastPieceSize = secondPiece->pieceSize;
					totalSize = normalPieceSize * (m_connectedSession - 1) + lastPieceSize;
					frameData = (u8*)malloc(totalSize);
				}else
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
					FramePiece* piece = m_screenCaptureSessions[i]->SafeGetFramePiece(iter->first);
					memcpy(frameData + (piece->pieceIndex *normalPieceSize), piece->piece, piece->pieceSize);
					if(piece->pieceSize != normalPieceSize)
					{
						lastPieceSize = piece->pieceSize;
					}
					// free piece data
					piece->release();
					delete piece;
				}
				//--------------------------------------------------
				if (lastPieceSize == 0) lastPieceSize = normalPieceSize;
				totalSize = normalPieceSize * (m_connectedSession - 1) + lastPieceSize;
			}
			m_frameTracker.erase(iter);
		}
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
		ppGraphicsDrawFrame(config.output.private_memory, nW * nH * 3, nW, nH);
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
