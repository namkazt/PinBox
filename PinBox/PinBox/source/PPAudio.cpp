#include "PPAudio.h"
#include <cstring>
#include <cstdio>

#define SAMPLERATE 22050
#define SAMPLESPERBUF 1152
#define BYTESPERSAMPLE 4
/* Channel to play audio on */
#define CHANNEL 0x08

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

	pAudioBuffer1 = (u8*)linearAlloc(SAMPLESPERBUF*(BYTESPERSAMPLE / 2));
	pAudioBuffer2 = (u8*)linearAlloc(SAMPLESPERBUF*(BYTESPERSAMPLE / 2));

	ndspChnReset(CHANNEL);
	ndspChnWaveBufClear(CHANNEL);
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetInterp(CHANNEL, NDSP_INTERP_POLYPHASE);
	ndspChnSetRate(CHANNEL, SAMPLERATE);
	ndspChnSetFormat(CHANNEL, NDSP_FORMAT_STEREO_PCM16);

	float mix[12];
	memset(mix, 0, sizeof(mix));
	mix[0] = 1.0;
	mix[1] = 1.0;
	ndspChnSetMix(CHANNEL, mix);
}

void PPAudio::AudioExit()
{
	ndspChnWaveBufClear(CHANNEL);
	ndspExit();

	linearFree(pAudioBuffer1);
	linearFree(pAudioBuffer2);
}

void PPAudio::FillBuffer(void* buffer1, void* buffer2, u32 size)
{
	if(!startDecode)
	{
		startDecode = true;

		printf("[Audio] frame size:%d\n", size);
		
		// decode audio buffer
		memset(waveBuf, 0, sizeof(waveBuf));

		memcpy(pAudioBuffer1, buffer1, size);
		waveBuf[0].data_vaddr = &pAudioBuffer1[0];
		waveBuf[0].nsamples = size / 2;
		waveBuf[0].status = NDSP_WBUF_DONE;

		DSP_FlushDataCache(pAudioBuffer1, size);
		ndspChnWaveBufAdd(CHANNEL, &waveBuf[0]);

		memcpy(pAudioBuffer2, buffer2, size);
		waveBuf[1].data_vaddr = &pAudioBuffer2[0];
		waveBuf[1].nsamples = size / 2;
		waveBuf[1].status = NDSP_WBUF_DONE;


		DSP_FlushDataCache(pAudioBuffer2, size);
		ndspChnWaveBufAdd(CHANNEL, &waveBuf[1]);

		while (ndspChnIsPlaying(CHANNEL) == false);
	}else
	{
		memcpy(pAudioBuffer1, buffer1, size);
		if (waveBuf[0].status == NDSP_WBUF_DONE)
		{
			waveBuf[0].nsamples = size / 2;
			DSP_FlushDataCache(pAudioBuffer1, size);
			ndspChnWaveBufAdd(CHANNEL, &waveBuf[0]);
		}
	

		memcpy(pAudioBuffer2, buffer2, size);
		if (waveBuf[1].status == NDSP_WBUF_DONE)
		{
			waveBuf[1].nsamples = size / 2;
			DSP_FlushDataCache(pAudioBuffer2, size);
			ndspChnWaveBufAdd(CHANNEL, &waveBuf[1]);
		}
	}
}
