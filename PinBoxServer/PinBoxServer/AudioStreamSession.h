#pragma once
#ifndef _AUDIO_STREAM_SESSION_H__
#define _AUDIO_STREAM_SESSION_H__

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <thread>
#include "PPMessage.h"
#include <mutex>

class AudioStreamSession
{
private:
	IAudioClient				*pAudioClient;
	IMMDevice					*m_pMMDevice;
	HANDLE						g_StopEvent;
	std::thread					g_thread;
	bool						g_isPaused = false;

	u8*							mTmpAudioBuffer = nullptr;
	u32							mTmpAudioBufferSize = 0;

	u32							mSampleRate = 0;
	std::mutex					*mutex;
	

private:
	void						useDefaultDevice();

public:
	~AudioStreamSession();

	void						StartAudioStream();
	void						Pause();
	void						Resume();
	void						StopStreaming();

	void						loopbackThread();
};

#endif