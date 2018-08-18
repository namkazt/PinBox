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


	
	std::mutex					*mutex;



	void						useDefaultDevice();


public:

	u32							audioBufferSize = 0;
	u8*							audioBuffer = nullptr;

	int							audioFrames = 0;
	u32							sampleRate = 0;
public:
	~AudioStreamSession();

	void						StartAudioStream();
	void						Pause();
	void						Resume();
	void						StopStreaming();

	void						ReadFromBuffer(u8* buf, u32 size);

	void						loopbackThread();
};

#endif