#pragma once

#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <functional>
#include <thread>
#include "opusenc.h"
#include "PPMessage.h"


typedef std::function<void(u8* buffer, u32 size, u32 frames)> OnRecordDataCallback;
typedef std::function<void(u8* buffer, u32 size)> OnDataCallback;
typedef std::function<void(u32 nChannels, u32 nSampleRate)> OnGetDeviceInfo;
typedef std::function<void()> OnFinishCallback;

class AudioStreamSession
{
private:
	IMMDevice					*m_pMMDevice;
	HANDLE						g_StopEvent;
	std::thread					g_thread;
	u32							g_totalFrameRecorded = 0;
	OnRecordDataCallback		g_onRecordDataCallback = nullptr;

	OggOpusEnc					*g_opus_encoder;
	OggOpusComments				*g_opus_comments;
	OnDataCallback				g_opus_onDataCallback = nullptr;
	OnFinishCallback			g_opus_onFinishCallback = nullptr;
	OnGetDeviceInfo				g_opus_onGetDeviceInfo = nullptr;
public:
	u32							GetLastFramesRecorded() { return g_totalFrameRecorded; }
	

private:
	void						useDefaultDevice();

	static void					loopbackCaptureThreadFunction(void* context);


	static int					opus_onWrite(void *user_data, const unsigned char *ptr, opus_int32 len);
	static int					opus_onClose(void *user_data);
public:
	~AudioStreamSession();

	void						StartOpusAudioStream();
	void						StopStreaming();
	void						SetOnRecordData(OnRecordDataCallback callback) { g_onRecordDataCallback = callback; }
	void						SetOnNewRecordedData(OnDataCallback callback) { g_opus_onDataCallback = callback; }
	void						SetOnEncoderClosed(OnFinishCallback callback) { g_opus_onFinishCallback = callback; }

public: // test functions
	HMMIO						g_tmpWavFile;
	void						Helper_WriteWaveHeader(LPCWSTR fileName, LPCWAVEFORMATEX pwfx, MMCKINFO *pckRIFF, MMCKINFO *pckData);
	void						Helper_FinishWaveFile(LPCWSTR fileName, MMCKINFO *pckRIFF, MMCKINFO *pckData);
	void						Helper_FixWaveFile(LPCWSTR fileName, u32 nFrames);
};

