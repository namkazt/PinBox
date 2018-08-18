#include "PPAudio.h"
#include <cstring>
#include <cstdio>

#define SAMPLERATE 22050
#define SAMPLESPERBUF 1152
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


	// buf temporary
	u8 * temp = (u8 *)linearAlloc(SAMPLESPERBUF * BYTESPERSAMPLE * MAX_AUDIO_BUF);
	memset(temp, 0, SAMPLESPERBUF * BYTESPERSAMPLE * MAX_AUDIO_BUF);
	DSP_FlushDataCache(temp, SAMPLESPERBUF * BYTESPERSAMPLE * MAX_AUDIO_BUF);


	ndspChnWaveBufClear(CHANNEL);
	ndspChnReset(CHANNEL);

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetInterp(CHANNEL, NDSP_INTERP_POLYPHASE);
	ndspChnSetRate(CHANNEL, SAMPLERATE);
	ndspChnSetFormat(CHANNEL, NDSP_FORMAT_STEREO_PCM16);

	// set mix
	float mix[12];
	memset(mix, 0, sizeof(mix));
	mix[0] = 1.0;
	mix[1] = 1.0;
	ndspChnSetMix(0, mix);


	memset(waveBuf, 0, sizeof(waveBuf));

	waveBuf[0].data_vaddr = temp;
	waveBuf[0].nsamples = SAMPLESPERBUF;
	waveBuf[0].status = NDSP_WBUF_DONE;

	waveBuf[1].data_vaddr = temp + (SAMPLESPERBUF * BYTESPERSAMPLE);
	waveBuf[1].nsamples = SAMPLESPERBUF;
	waveBuf[1].status = NDSP_WBUF_DONE;
}

void PPAudio::AudioExit()
{
	ndspChnWaveBufClear(CHANNEL);
	ndspExit();

	linearFree(waveBuf[0].data_vaddr);
	waveBuf[0].data_vaddr = NULL;
}

void PPAudio::FillBuffer(u8* buffer, u32 size)
{
	size_t nextBuf = _nextBuf;
	while (waveBuf[nextBuf].status != NDSP_WBUF_DONE)
	{
		svcSleepThread(50* 1000ULL);
	}

	printf("Got new audio buffer: %d - queue on buf:%d\n", SAMPLESPERBUF * BYTESPERSAMPLE, nextBuf);

	memcpy(waveBuf[nextBuf].data_pcm16, buffer, SAMPLESPERBUF * BYTESPERSAMPLE);
	DSP_FlushDataCache(waveBuf[nextBuf].data_pcm16, SAMPLESPERBUF * BYTESPERSAMPLE);

	waveBuf[nextBuf].offset = 0;
	waveBuf[nextBuf].status = NDSP_WBUF_QUEUED;
	ndspChnWaveBufAdd(CHANNEL, &waveBuf[nextBuf]);

	_nextBuf = (nextBuf + 1) % MAX_AUDIO_BUF;
}