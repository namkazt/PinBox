#pragma once

// frame capture
#include "ScreenCapture.h"
#include "PPMessage.h"
#include "PPClientSession.h"


typedef struct
{
	u8* piece;
	u32 size;
	u8 index;
	u32 frameIndex;
} FramePiece;

typedef std::function<void()> OnClientReallyClose;

class ScreenCaptureSession
{
private:
	PPServer*									m_server;
	SL::Screen_Capture::ScreenCaptureManager	m_frameGrabber;
	u32											m_frameIndex = 0;
	u32											m_numberClients = 0;
	bool										m_isStartStreaming = false;
	std::map<u32, std::vector<FramePiece*>>		m_frameCacheMap;
	std::map<u32, u32>							m_frameCacheState;
	//---------------------------------------------------------------------------------------------------------------------------
	//--------------------OPTIONS------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------
	//											Server will wait until client received frame after send new frame. Default: true
	bool										g_ss_waitForClientReceived = true;
	u32											g_ss_waitForFrame = 2;
	u32											g_ss_currentWaitFrame = 0;
	//											Output scale from 0 -> 100 percent [100 is best]
	u32											g_ss_outputScale = 100;
	//											Output quality from 0 -> 100 percent [100 is best]
	u32											g_ss_outputQuality = 50;
	OnClientReallyClose							g_ss_onClientReallyClosed = nullptr;
public:
	ScreenCaptureSession();
	~ScreenCaptureSession();

	void										startStream();
	void										stopStream();
	void										registerForStopStream(OnClientReallyClose callback = nullptr);
	void										removeForStopStream() { g_ss_onClientReallyClosed = nullptr; }
	void										changeSetting(bool waitForReceived, u32 smoothFrame, u32 quality, u32 scale);
	void										initSCreenCapturer(PPServer* parent);
	void										splitFrameToMultiPieces(const SL::Screen_Capture::Image& img);
	int											getAvaliableFramePiece(u32 frameIndex);
	FramePiece*									getNewFramePieces();


	u32											currentFrame() { return m_frameIndex; }
	void										updateClientNumber(int addition) { m_numberClients += addition; if (m_numberClients < 0) m_numberClients = 0; }
	bool										isStreaming() { return m_isStartStreaming; }
};

