#pragma once
#ifndef _PP_AUDIO_H_
#define _PP_AUDIO_H_

#include <3ds.h>


class PPAudio
{
private:
	uint64_t					pIdx;
	s16*						pAudioBuffer1;
	s16*						pAudioBuffer2;
	ndspWaveBuf				waveBuf[2];
	bool					startDecode = false;
public:
	~PPAudio();
	static PPAudio* Get();

	void AudioInit();
	void AudioExit();

	void FillBuffer(s16* buffer1, s16* buffer2, u32 size);
};

#endif