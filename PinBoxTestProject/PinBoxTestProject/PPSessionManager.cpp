#include "PPSessionManager.h"




PPSessionManager::PPSessionManager()
{
}


PPSessionManager::~PPSessionManager()
{
}

void PPSessionManager::InitScreenCapture(u32 numberOfSessions)
{
	m_decoder = new PPDecoder();
	m_decoder->startDecodeThread();

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
	if (m_staticVideoBuffer == nullptr)
	{
		m_staticVideoBuffer = (u8*)malloc(VideoBufferSize);
		m_videoBufferSize = 0;
		m_videoBufferCursor = 0;
	}
	//==============================================


	m_currentDisplayFrame = 0;
	m_frameTracker.clear();
	m_connectedSession = 0;
	for (int i = 0; i < m_screenCaptureSessions.size(); i++)
	{
		// start connect all session to server
		m_screenCaptureSessions[i]->StartSession(ip, port, [=](u8* data, u32 code)
		{
			m_connectedSession++;
			if(m_connectedSession == m_screenCaptureSessions.size())
			{
				// when all session is authenticated
				// start streaming here
				std::cout << "All sessions is connected to server : Start Streaming" << std::endl;
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

void PPSessionManager::AppendBuffer(u8* buffer, u32 size)
{
	m_decoder->appendBuffer(buffer, size);
	//memmove(m_staticVideoBuffer + m_videoBufferSize, buffer, size);
	//m_videoBufferSize += size;
}

void PPSessionManager::DecodeVideo()
{
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
