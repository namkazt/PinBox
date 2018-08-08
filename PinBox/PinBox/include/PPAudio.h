#pragma once
#ifndef _PP_AUDIO_H_
#define _PP_AUDIO_H_

#include <3ds.h>


class PPAudio
{
private:
	u8*						pAudioBuffer1;
	u8*						pAudioBuffer2;
	ndspWaveBuf				waveBuf[2];
	bool					startDecode = false;
public:
	~PPAudio();
	static PPAudio* Get();

	void AudioInit();
	void AudioExit();

	void FillBuffer(void* buffer1, void* buffer2, u32 size);
};

#endif