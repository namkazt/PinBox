#include "stdafx.h"

#include "ScreenCaptureSession.h"
//webp
#include "webp/encode.h"
#include "PPServer.h"

ScreenCaptureSession::ScreenCaptureSession()
{
}


ScreenCaptureSession::~ScreenCaptureSession()
{
	m_frameGrabber.pause();
	m_frameGrabber.destroy();
}


void ScreenCaptureSession::initSCreenCapturer(PPServer* parent)
{
	m_server = parent;
	//---------------------------------------------------------------------
	m_frameGrabber = SL::Screen_Capture::CreateScreeCapture([]()
	{
		auto mons = SL::Screen_Capture::GetMonitors();
		std::vector<SL::Screen_Capture::Monitor> selectedMonitor = std::vector<SL::Screen_Capture::Monitor>();
		//-------------------------- >> Index 0
		selectedMonitor.push_back(mons[1]);
		return selectedMonitor;
	}).onNewFrame([&](const SL::Screen_Capture::Image& img, const SL::Screen_Capture::Monitor& monitor)
		{
			if (!m_isStartStreaming) return;
			if (g_ss_onClientReallyClosed != nullptr) {
				m_isStartStreaming = false;
				g_ss_onClientReallyClosed();
				return;
			}
			//-------------------------------------------------
			// clean up our frame cached
			for(auto iter = m_frameCacheState.begin(); iter != m_frameCacheState.end();)
			{
				if(iter->second == m_numberClients || iter->first < m_frameIndex)
				{
					// free memory
					for (char i = 0; i <  m_numberClients; i++)
					{
						free(m_frameCacheMap[iter->first][i]->piece);
						delete m_frameCacheMap[iter->first][i];
					}
					std::vector<FramePiece*>().swap(m_frameCacheMap[iter->first]);
					m_frameCacheMap.erase(iter->first);
					m_frameCacheState.erase(iter++); 
				}else
				{
					++iter;
				}
			}
			//-------------------------------------------------
			for (std::map<std::string, PPClientSession*> ::iterator iter = m_server->clientSessions.begin(); iter != m_server->clientSessions.end(); ++iter)
			{
				if(iter->second->g_ss_isReceived)
				{
					iter->second->GetPieceDataAndSend();
				}
			}
			//-------------------------------------------------
			// check if we need continue split
			if(m_frameCacheMap.size() > 0)
			{
				if (g_ss_waitForClientReceived)
				{
					g_ss_currentWaitFrame++;
					if (g_ss_currentWaitFrame > g_ss_waitForFrame)
					{
						std::cout << ">>> Smooth frame happen" << std::endl;
						splitFrameToMultiPieces(img);
						g_ss_currentWaitFrame = 0;
					}
				}else
				{
					splitFrameToMultiPieces(img);
					g_ss_currentWaitFrame = 0;
				}
			}else
			{
				g_ss_currentWaitFrame = 0;
				splitFrameToMultiPieces(img);
			}
		}
	).start_capturing();
	m_frameGrabber.setFrameChangeInterval(std::chrono::nanoseconds(10));//100 ms
	m_frameGrabber.pause();
}

void ScreenCaptureSession::splitFrameToMultiPieces(const SL::Screen_Capture::Image& img)
{
	u32 newW = 400.0f * ((float)g_ss_outputScale / 100.0f);
	u32 newH = 240.0f * ((float)g_ss_outputScale / 100.0f);
	//===============================================================
	// get buffer data from image frame
	auto size = Width(img) * Height(img) * 3;
	u8* imgBuffer = (u8*)malloc(size);
	SL::Screen_Capture::ExtractAndConvertToRGB(img, (char*)imgBuffer);
	//===============================================================
	// webp encode
	try {
		WebPConfig config;
		if (!WebPConfigPreset(&config, WEBP_PRESET_DEFAULT, g_ss_outputQuality)) {
			return;  // version error
		}
		//===============================================================
		// Add additional tuning:
		config.sns_strength = 90;
		config.filter_sharpness = 6;
		config.segments = 4;
		config.method = 0;
		config.alpha_compression = 0;
		config.alpha_quality = 0;
		auto config_error = WebPValidateConfig(&config);
		//===============================================================
		// Setup the input data, allocating a picture of width x height dimension
		WebPPicture pic;
		if (!WebPPictureInit(&pic)) {
			return;
		}
		pic.width = Width(img);
		pic.height = Height(img);
		if (!WebPPictureAlloc(&pic)) {
			return;
		}
		WebPPictureImportRGB(&pic, imgBuffer, Width(img) * 3);
		WebPPictureRescale(&pic, newW, newH);
		WebPMemoryWriter writer;
		WebPMemoryWriterInit(&writer);
		pic.writer = WebPMemoryWrite;
		pic.custom_ptr = &writer;
		int ok = WebPEncode(&config, &pic);
		// Always free the memory associated with the input.
		WebPPictureFree(&pic);   
		//===============================================================
		// result send to client
		if (!ok) {
			WebPMemoryWriterClear(&writer);
			free(imgBuffer);
			return;
		}
		//===============================================================
		// we got image here. now split it
		u32 normalPieceSize = writer.size / m_numberClients;
		u32 lastPieceSize = writer.size - (normalPieceSize * m_numberClients) + normalPieceSize;
		//===============================================================
		std::vector<FramePiece*> framePieces;
		for(int i = 0; i < m_numberClients; i++)
		{
			FramePiece* framePiece = new FramePiece();
			if (m_frameIndex > UINT32_MAX - 100)
				framePiece->frameIndex = 0;
			else
				framePiece->frameIndex = m_frameIndex+1;
			framePiece->index = i;
			if (i == m_numberClients - 1) framePiece->size = lastPieceSize;
			else framePiece->size = normalPieceSize;
			framePiece->piece = (u8*)malloc(framePiece->size);
			memcpy(framePiece->piece, writer.mem + (i*normalPieceSize), framePiece->size);
			framePieces.push_back(framePiece);
		}
		//===============================================================
		// increment for frame index
		if (m_frameIndex > UINT32_MAX - 100) m_frameIndex = 0;	// reset frame counter
		m_frameIndex++;
		m_frameCacheMap[m_frameIndex] = framePieces;
		m_frameCacheState[m_frameIndex] = 0;

		std::cout << "SPLIT frame: " << m_frameIndex << std::endl;
		//===============================================================
		// clean up 
		WebPMemoryWriterClear(&writer);
		free(imgBuffer);
	}
	catch (...)
	{
		free(imgBuffer);
	}
}

int ScreenCaptureSession::getAvaliableFramePiece(u32 frameIndex)
{
	if(g_ss_waitForClientReceived)
	{
		// get lowest 
		auto iter = m_frameCacheState.begin();
		while(iter != m_frameCacheState.end())
		{
			if (iter->second < m_numberClients)
			{
				u32 result = iter->second;
				iter->second++;
				return result;
			}
			++iter;
		}
	}else
	{
		auto iter = m_frameCacheState.find(frameIndex);
		if (iter != m_frameCacheState.end())
		{
			if (iter->second == m_numberClients) return -1;
			u32 result = iter->second;
			iter->second++;
			return result;
		}
	}
	return -1;
}

FramePiece* ScreenCaptureSession::getNewFramePieces()
{
	int avaiablePieceIndex = getAvaliableFramePiece(m_frameIndex);
	if (avaiablePieceIndex == -1) return nullptr;
	return m_frameCacheMap[m_frameIndex][avaiablePieceIndex];
}

void ScreenCaptureSession::startStream()
{
	m_isStartStreaming = true;
	m_frameIndex = 0;
	g_ss_currentWaitFrame = 0;
	m_frameGrabber.resume();
}

void ScreenCaptureSession::stopStream()
{
	m_isStartStreaming = false;
	m_frameGrabber.pause();
	for (auto iter = m_frameCacheState.begin(); iter != m_frameCacheState.end();++iter)
	{
		// free memory
		for (char i = 0; i < m_frameCacheMap[iter->first].size(); i++)
		{
			free(m_frameCacheMap[iter->first][i]->piece);
			delete m_frameCacheMap[iter->first][i];
		}
		std::vector<FramePiece*>().swap(m_frameCacheMap[iter->first]);
		m_frameCacheMap.erase(iter->first);
	}
	m_frameCacheState.clear();
	m_frameCacheMap.clear();
}

void ScreenCaptureSession::registerForStopStream(OnClientReallyClose callback)
{
	g_ss_onClientReallyClosed = callback;
}

void ScreenCaptureSession::changeSetting(bool waitForReceived, u32 smoothFrame, u32 quality, u32 scale)
{
	g_ss_waitForClientReceived = waitForReceived;
	g_ss_waitForFrame = smoothFrame;
	g_ss_outputQuality = quality;
	g_ss_outputScale = scale;
}
