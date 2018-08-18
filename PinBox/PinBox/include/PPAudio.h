#pragma once
#ifndef _PP_AUDIO_H_
#define _PP_AUDIO_H_

#include <3ds.h>
#include <libavutil/frame.h>
#include "Mutex.h"

#define MAX_AUDIO_BUF 2


class PPAudio
{
private:
	bool						_initialized = false;
	ndspWaveBuf					_waveBuf[MAX_AUDIO_BUF];
	int							_nextBuf = 0;

public:
	~PPAudio();
	static PPAudio* Get();

	void AudioInit();
	void AudioExit();


	void FillBuffer(u8* buffer, u32 size);


};

#endif