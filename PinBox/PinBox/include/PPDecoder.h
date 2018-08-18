#pragma once
#include <3ds.h>
#include "Mutex.h"
#include "yuv_rgb.h"
#include <3ds/services/y2r.h>
//ffmpeg
extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#define MEMORY_BUFFER_SIZE 0x1400000
#define MEMORY_BUFFER_PADDING 0x04

typedef struct MemoryBuffer {
	u8* pBufferAddr;
	u32 iCursor;
	u32 iSize;
	u32 iMaxSize;
	Mutex* pMutex;

	void write(u8* buf, u32 size)
	{
		if (iSize < size) return;
		pMutex->Lock();
		memcpy(pBufferAddr + iCursor, buf, size);
		iCursor += size;
		iSize -= size;
		pMutex->Unlock();
	}

	int read(u8* buf, u32 size)
	{
		if (iCursor == 0) return -1;
		pMutex->Lock();
		int ret = FFMIN(iCursor, size);
		memcpy(buf, pBufferAddr, ret);
		iSize += ret;
		iCursor -= ret;
		pMutex->Unlock();
		return ret;
	}
}MemoryBuffer;

typedef struct DecodeState{
	Y2RU_ConversionParams y2rParams;
	Handle endEvent;
}DecodeState;

class PPDecoder
{
private:

	// video stream
	//AVCodecParserContext*		pVideoParser;
	AVCodecContext*				pVideoContext;
	AVPacket*					pVideoPacket;
	AVFrame*					pVideoFrame;
	DecodeState*				pDecodeState;
	u8*							decodeVideoStream();
	void						initY2RImageConverter();
	void						convertColor();

	// audio stream
	AVCodecContext*				pAudioContext;
	AVPacket*					pAudioPacket;
	AVFrame*					pAudioFrame;

public:
	PPDecoder();
	~PPDecoder();

	u32 iFrameWidth;
	u32 iFrameHeight;

	void initDecoder();
	void releaseDecoder();

	u8* appendVideoBuffer(u8* buffer, u32 size);
	void decodeAudioStream(u8* buffer, u32 size);
};