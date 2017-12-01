#pragma once

#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <functional>
#include <thread>

#include "PPMessage.h"


typedef std::function<void(u8* buffer, u32 size, u32 frames)> OnRecordDataCallback;

class AudioStreamSession
{
private:
	IMMDevice					*m_pMMDevice;
	HANDLE						g_StopEvent;
	std::thread					g_thread;
	u32							g_totalFrameRecorded = 0;
	OnRecordDataCallback		g_onRecordDataCallback = nullptr;

public:
	u32							GetLastFramesRecorded() { return g_totalFrameRecorded; }

private:
	void						useDefaultDevice();

	static void					loopbackCaptureThreadFunction(void* context);
public:
	AudioStreamSession();
	~AudioStreamSession();

	void						StartAudioStream();
	void						StopStreaming();
	void						SetOnRecordData(OnRecordDataCallback callback) { g_onRecordDataCallback = callback; }

public: // test functions
	HMMIO						g_tmpWavFile;
	void						Helper_WriteWaveHeader(LPCWSTR fileName, LPCWAVEFORMATEX pwfx, MMCKINFO *pckRIFF, MMCKINFO *pckData);
	void						Helper_FinishWaveFile(LPCWSTR fileName, MMCKINFO *pckRIFF, MMCKINFO *pckData);
	void						Helper_FixWaveFile(LPCWSTR fileName, u32 nFrames);
};

