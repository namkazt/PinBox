#pragma once
#include <vector>
#include "PPSession.h"
#include <webp/decode.h>
#include <turbojpeg.h>
#include "opusfile.h"
#include <map>
#include <stack>
#include "PPDecoder.h"

#include "constant.h"
#include "ConfigManager.h"

enum SessionState
{
	SS_NOT_CONNECTED = 0,
	SS_CONNECTING,
	SS_CONNECTED,
	SS_PAIRED,
	SS_STREAMING,

	SS_FAILED
};

typedef struct
{
	u8		*buffer;
	u32			size;
} VideoFrame;

enum BusyState
{
	BS_NONE = 0,
	BS_AUTHENTICATION,
	BS_HUB_ITEMS
};

class PPSessionManager
{
protected:
	PPSession*										_session = nullptr;
	PPSession*										_testSession = nullptr;

	//--------------------------------------------
	// decoder
	//--------------------------------------------
	PPDecoder*										_decoder = nullptr;

	//--------------------------------------------
	// input
	//--------------------------------------------
	u32												_oldDown;
	u32												_oldUp;
	short											_oldCX;
	short											_oldCY;
	short											_oldCTX;
	short											_oldCTY;
	bool											_initInputFirstFrame = false;

	//--------------------------------------------
	// fps
	//--------------------------------------------
	u64												_lastRenderTime = 0;
	float											_currentRenderFPS = 0.0f;
	u32												_renderFrames = 0;

	u64												_lastVideoTime = 0;
	float											_currentVideoFPS = 0.0f;
	u32												_videoFrame = 0;
	bool											_receivedFirstVideoFrame = false;


	
	SessionState									_sessionState = SS_NOT_CONNECTED;
	BusyState										_busyState = BS_NONE;
public:
	PPSessionManager();
	~PPSessionManager();

	/**
	 * \brief Test connections
	 * \param config server informations 
	 * \return  0 as pending\n
	 *			-1 as failed\n
	 *			1 as successfully
	 */
	int TestConnection(ServerConfig *config);

	/**
	 * \brief Must call after @ref InitNewSession to connect to server by config.
	 * update follow @ref GetSessionState to get connection state
	 * @ref GetSessionState return @ref SS_FAILED if connect error
	 * 						return @ref SS_PAIRED if connect and authentication successfully
	 * \param config server informations
	 * \return @ref SessionState
	 */
	SessionState ConnectToServer(ServerConfig* config);

	/**
	 * \brief Disconnect to Paired server
	 *  @ref GetSessionState will return @ref SS_NOT_CONNECTED
	 */
	void DisconnectToServer();

	int GetHubItemCount() { return _session->GetHubItemCount(); }
	HubItem* GetHubItem(int i) { return _session->GetHubItem(i); }

	/**
	 * \brief Get current session state
	 * \return @ref SessionState
	 */
	SessionState GetSessionState() const { return _sessionState; };


	/**
	 * \brief Set current session state
	 * \param v new state that apply for session
	 */
	void SetSessionState(SessionState v) { _sessionState = v; };


	/**
	 * \brief init video and audio decoder
	 */
	void InitDecoder();

	/**
	 * \brief release video and audio decoder
	 */
	void ReleaseDecoder();


	/**
	 * \brief update new stream setting to server
	 * user need update config from @ref PPConfigManager first before call this function
	 */
	void UpdateStreamSetting();

	/**
	* \brief get server's controller profiles ( it only return key name of each profiles )
	*/
	void GetControllerProfiles();


	/**
	 * \brief Send authentication message
	 */
	void Authentication();


	BusyState GetBusyState() { return _busyState; }
	void SetBusyState(BusyState bs) { _busyState = bs; }

	/**
	 * \brief Tell server that client want to start streaming
	 * 
	 */
	void StartStreaming();

	void StopStreaming();

	void StartFPSCounter();
	void UpdateFPSCounter();

	void UpdateInputStream(u32 down, u32 up, short cx, short cy, short ctx, short cty);
	void ProcessVideoFrame(u8* buffer, u32 size);
	void ProcessAudioFrame(u8* buffer, u32 size);

	

	void DrawVideoFrame();

	

	// fps
	float GetFPS() const { return _currentRenderFPS; }
	float GetVideoFPS() const { return _currentVideoFPS; }


};

