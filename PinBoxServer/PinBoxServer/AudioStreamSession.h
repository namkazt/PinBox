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
	IAudioClient				*_pAudioClient;
	IMMDevice					*_pMMDevice;
	HANDLE						_stopEvent;
	std::thread					_thread;
	bool						_isPaused = false;


	
	std::mutex					*_mutex;



	void						useDefaultDevice();


public:

	u32							audioBufferSize = 0;
	u8*							audioBuffer = nullptr;

	int							audioFrames = 0;
	u32							sampleRate = 0;
public:

	// Start audio stream session ( init every thing relate )
	void						StartAudioStream();
	void						Pause();
	void						Resume();
	// Stop audio stream session ( release every thing relate )
	void						StopStreaming();

	// Read input amount of stored audio buffer if available 
	void						ReadFromBuffer(u8* outBuf, u32 readSize);
	// Reset storage buffer to zero ( in case of user want to avoid read old data to avoid laggy )
	void						ResetStorageBuffer();

	void						loopbackThread();
};

#endif