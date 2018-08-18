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
	if (_initialized) return;

	ndspInit();

	// buf temporary
	u8 * temp = (u8 *)linearAlloc(SAMPLESPERBUF * BYTESPERSAMPLE * MAX_AUDIO_BUF);
	// fail to alloc tmp memory
	if(temp == nullptr)
	{
		ndspExit();
		return;
	}

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

	memset(_waveBuf, 0, sizeof(_waveBuf));
	_waveBuf[0].data_vaddr = temp;
	_waveBuf[0].nsamples = SAMPLESPERBUF;
	_waveBuf[0].status = NDSP_WBUF_DONE;
	_waveBuf[1].data_vaddr = temp + (SAMPLESPERBUF * BYTESPERSAMPLE);
	_waveBuf[1].nsamples = SAMPLESPERBUF;
	_waveBuf[1].status = NDSP_WBUF_DONE;

	_initialized = true;
}

void PPAudio::AudioExit()
{
	if (!_initialized) return;
	_initialized = false;

	ndspChnWaveBufClear(CHANNEL);
	ndspExit();

	linearFree(_waveBuf[0].data_vaddr);
	_waveBuf[0].data_vaddr = NULL;
}

void PPAudio::FillBuffer(u8* buffer, u32 size)
{
	if (!_initialized) return;

	size_t nextBuf = _nextBuf;
	while (_waveBuf[nextBuf].status != NDSP_WBUF_DONE)
	{
		svcSleepThread(25* 1000ULL);
	}

	memcpy(_waveBuf[nextBuf].data_pcm16, buffer, SAMPLESPERBUF * BYTESPERSAMPLE);
	DSP_FlushDataCache(_waveBuf[nextBuf].data_pcm16, SAMPLESPERBUF * BYTESPERSAMPLE);

	_waveBuf[nextBuf].offset = 0;
	_waveBuf[nextBuf].status = NDSP_WBUF_QUEUED;
	ndspChnWaveBufAdd(CHANNEL, &_waveBuf[nextBuf]);

	_nextBuf = (nextBuf + 1) % MAX_AUDIO_BUF;
}