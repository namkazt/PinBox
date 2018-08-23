#include "PPSessionManager.h"
#include "PPGraphics.h"
#include "ConfigManager.h"
#include "Logger.h"

PPSessionManager::PPSessionManager()
{
}

PPSessionManager::~PPSessionManager()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// connection test
///////////////////////////////////////////////////////////////////////////////////////////////////////////

int PPSessionManager::TestConnection(ServerConfig* config)
{
	if (_testSession != nullptr) {
		int ret = _testSession->GetTestConnectionResult();

		printf("RET: %d.\n", ret);
		if(ret != 0)
		{
			delete _testSession;
			_testSession = NULL;
			_sessionState = SS_NOT_CONNECTED;
		}
		return ret;
	}

	printf("Init TEST connection to server.\n");
	_testSession = new PPSession();
	_testSession->InitTestSession(this, config->ip.c_str(), config->port.c_str());
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// For current displaying video frame

SessionState PPSessionManager::ConnectToServer(ServerConfig* config)
{
	if (_session == nullptr)
	{
		printf("Init connection to server.\n");
		_sessionState = SS_CONNECTING;
		_session = new PPSession();
		_session->InitSession(this, config->ip.c_str(), config->port.c_str());
	}
	return _sessionState;
}

void PPSessionManager::DisconnectToServer()
{
	if (_sessionState == SS_NOT_CONNECTED) return;
	if (_session == nullptr) return;
	printf("Cleanup session.\n");
	delete _session;
	_session = NULL;
	_sessionState = SS_NOT_CONNECTED;
}

void PPSessionManager::StartStreaming()
{
	if (_sessionState != SS_PAIRED) return;
	//TODO: implement this
}

void PPSessionManager::StopStreaming()
{
	if (_sessionState != SS_PAIRED) return;
	//TODO: implement this
}

void PPSessionManager::ProcessVideoFrame(u8* buffer, u32 size)
{
	u8* rgbBuffer = _decoder->appendVideoBuffer(buffer, size);
	if (rgbBuffer != nullptr) PPGraphics::Get()->UpdateTopScreenSprite(rgbBuffer, 393216);

	//---------------------------------------------------------
	// update frame video FPS
	//---------------------------------------------------------
	if (!_receivedFirstVideoFrame)
	{
		_receivedFirstVideoFrame = true;
		_lastVideoTime = osGetTime();
		_currentVideoFPS = 0.0f;
		_videoFrame = 0;
	}
	else
	{
		_videoFrame++;
		u64 deltaTime = osGetTime() - _lastVideoTime;
		if (deltaTime > 1000)
		{
			_currentVideoFPS = _videoFrame / (deltaTime / 1000.0f);
			_videoFrame = 0;
			_lastVideoTime = osGetTime();
		}
	}
}

void PPSessionManager::DrawVideoFrame()
{
	PPGraphics::Get()->DrawTopScreenSprite();
}

void PPSessionManager::ProcessAudioFrame(u8* buffer, u32 size)
{
	_decoder->decodeAudioStream(buffer, size);
}

void PPSessionManager::UpdateStreamSetting()
{
	//_session->SendMsgChangeSetting();
}

void PPSessionManager::GetControllerProfiles()
{
}

void PPSessionManager::Authentication()
{
	if (_sessionState != SS_CONNECTED || GetBusyState() != BS_NONE) return;
	SetBusyState(BS_AUTHENTICATION);
	_session->SendMsgAuthentication();
}

void PPSessionManager::InitDecoder()
{
	_decoder = new PPDecoder();
	_decoder->initDecoder();
}

void PPSessionManager::ReleaseDecoder()
{
	_decoder->releaseDecoder();
}


void PPSessionManager::UpdateInputStream(u32 down, u32 up, short cx, short cy, short ctx, short cty)
{
	if (_session == nullptr) return;
	if(down != _oldDown || up != _oldUp || cx != _oldCX || cy != _oldCY || ctx != _oldCTX || cty != _oldCTY || !_initInputFirstFrame)
	{
		if(_session->SendMsgSendInputData(down, up, cx, cy, ctx, cty))
		{
			_initInputFirstFrame = true;
			_oldDown = down;
			_oldUp = up;
			_oldCX = cx;
			_oldCY = cy;
			_oldCTX = ctx;
			_oldCTY = cty;
		}
	}
}


void PPSessionManager::StartFPSCounter()
{
	_lastRenderTime = osGetTime();
	_currentRenderFPS = 0.0f;
	_renderFrames = 0;
}

void PPSessionManager::UpdateFPSCounter()
{
	_renderFrames++;
	u64 deltaTime = osGetTime() - _lastRenderTime;
	if(deltaTime > 1000)
	{
		_currentRenderFPS = _renderFrames / (deltaTime / 1000.0f);
		_renderFrames = 0;
		_lastRenderTime = osGetTime();
	}
}
