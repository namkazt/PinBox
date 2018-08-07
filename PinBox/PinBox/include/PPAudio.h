#pragma once
#ifndef _PP_AUDIO_H_
#define _PP_AUDIO_H_

#include <3ds.h>


class PPAudio
{
private:
	ndspWaveBuf				waveBuf[2];
	u32						*audioBuffer;
public:
	~PPAudio();
	static PPAudio* Get();

	void AudioInit();
	void AudioExit();

	void FillBuffer(void* buf, u32 size);
};

#endif