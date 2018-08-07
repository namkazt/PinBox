#include "PPAudio.h"
#include <cstring>

#define SAMPLERATE 48000
#define SAMPLESPERBUF 1152
#define BYTESPERSAMPLE 4

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
	audioBuffer = (u32*)linearAlloc(SAMPLESPERBUF * BYTESPERSAMPLE * 2);

	ndspInit();
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
	ndspChnSetRate(0, SAMPLERATE);
	ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);

	float mix[12];
	memset(mix, 0, sizeof(mix));
	mix[0] = 1.0;
	mix[1] = 1.0;
	ndspChnSetMix(0, mix);

	memset(waveBuf, 0, sizeof(waveBuf));
	waveBuf[0].data_vaddr = &audioBuffer[0];
	waveBuf[0].nsamples = SAMPLESPERBUF;
	waveBuf[1].data_vaddr = &audioBuffer[SAMPLESPERBUF];
	waveBuf[1].nsamples = SAMPLESPERBUF;
}

void PPAudio::AudioExit()
{
	ndspExit();
	linearFree(audioBuffer);
}

void PPAudio::FillBuffer(void* buf, u32 size)
{
	//TODO: copy buf to audioBuffer

	DSP_FlushDataCache(audioBuffer, size);
}
