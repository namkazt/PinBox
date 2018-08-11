#include "PPAudio.h"
#include <cstring>
#include <cstdio>

#define SAMPLERATE 22050
#define SAMPLESPERBUF 576
#define BYTESPERSAMPLE 4
/* Channel to play audio on */
#define CHANNEL 0x00

//---------------------------------------------------------------------
static PPAudio* mInstance = nullptr;
PPAudio* PPAudio::Get()
{
	if (mInstance == nullptr)
	{
		mInstance = new PPAudio();
	}
	return mInstance;
}

void PPAudio::AudioInit()
{
	ndspInit();

	pAudioBuffer1 = (s16*)linearAlloc(SAMPLESPERBUF * BYTESPERSAMPLE);
	pAudioBuffer2 = (s16*)linearAlloc(SAMPLESPERBUF * BYTESPERSAMPLE);

	ndspChnReset(CHANNEL);
	ndspChnWaveBufClear(CHANNEL);
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetInterp(CHANNEL, NDSP_INTERP_POLYPHASE);
	ndspChnSetRate(CHANNEL, SAMPLERATE);
	ndspChnSetFormat(CHANNEL, NDSP_FORMAT_STEREO_PCM16);
}

void PPAudio::AudioExit()
{
	ndspChnWaveBufClear(CHANNEL);
	ndspExit();

	linearFree(pAudioBuffer1);
	linearFree(pAudioBuffer2);
}

void PPAudio::FillBuffer(s16* buffer1, s16* buffer2, u32 size)
{
	pIdx++;
	if(!startDecode)
	{
		startDecode = true;
		
		// decode audio buffer
		memset(waveBuf, 0, sizeof(waveBuf));

		memcpy(pAudioBuffer1, (s16*)buffer1, size);
		waveBuf[0].data_vaddr = &pAudioBuffer1[0];
		waveBuf[0].nsamples = size / 4;
		ndspChnWaveBufAdd(CHANNEL, &waveBuf[0]);

		memcpy(pAudioBuffer2, (s16*)buffer2, size);
		waveBuf[1].data_vaddr = &pAudioBuffer2[0];
		waveBuf[1].nsamples = size / 4;
		ndspChnWaveBufAdd(CHANNEL, &waveBuf[1]);

		while (ndspChnIsPlaying(CHANNEL) == false);
	}else
	{
		printf("[Audio] frame size:%d - idx:%d\n", size, pIdx);
		svcSleepThread(10ULL);
		
		if (waveBuf[0].status == NDSP_WBUF_DONE)
		{
			memcpy(pAudioBuffer1, (s16*)buffer1, size);
			waveBuf[0].nsamples = size / 4;
			ndspChnWaveBufAdd(CHANNEL, &waveBuf[0]);		
		}

		if (waveBuf[1].status == NDSP_WBUF_DONE)
		{
			memcpy(pAudioBuffer2, (s16*)buffer2, size);
			waveBuf[1].nsamples = size / 4;
			ndspChnWaveBufAdd(CHANNEL, &waveBuf[1]);
		}

		DSP_FlushDataCache(pAudioBuffer1, size);
		DSP_FlushDataCache(pAudioBuffer2, size);
	}
}
