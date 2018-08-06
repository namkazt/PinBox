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

	IMMDevice					*m_pMMDevice;
	HANDLE						g_StopEvent;
	std::thread					g_thread;
	u32							g_totalFrameRecorded = 0;
	bool						g_isPaused = false;

	u8*							mTmpAudioBuffer = nullptr;
	u32							mTmpAudioBufferSize = 0;
	int							mTmpAudioFrames = 0;
	u32							mSampleRate = 0;
	std::mutex					*mutex;
public:
	u32							GetLastFramesRecorded() { return g_totalFrameRecorded; }
	

private:
	void						useDefaultDevice();

	static void					loopbackCaptureThreadFunction(void* context);

public:
	~AudioStreamSession();

	void						StartAudioStream();
	void						Pause() { g_isPaused = true; }
	void						Resume() { g_isPaused = false; }
	void						StopStreaming();

	void						BeginReadAudioBuffer();
	u8*							GetAudioBuffer() { return mTmpAudioBuffer; }
	u32							GetAudioBufferSize() { return mTmpAudioBufferSize; }
	int							GetAudioFrames() { return mTmpAudioFrames; }
	u32							GetSampleRate() { return mSampleRate; }
	void						FinishReadAudioBuffer();
	int							Cutoff(u32 sizeRead, int framesRead);

public: // test functions
	HMMIO						g_tmpWavFile;
	void						Helper_WriteWaveHeader(LPCWSTR fileName, LPCWAVEFORMATEX pwfx, MMCKINFO *pckRIFF, MMCKINFO *pckData);
	void						Helper_FinishWaveFile(LPCWSTR fileName, MMCKINFO *pckRIFF, MMCKINFO *pckData);
	void						Helper_FixWaveFile(LPCWSTR fileName, u32 nFrames);
};

#endif