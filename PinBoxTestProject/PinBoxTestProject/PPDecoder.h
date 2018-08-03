#pragma once
#include "PPMessage.h"
#include <mutex>

//#include <3ds.h>

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
#include "yuv_rgb.h"
}

#define MEMORY_BUFFER_SIZE 0x1400000
#define MEMORY_BUFFER_PADDING 0x04

typedef struct MemoryBuffer {
	u8* pBufferAddr;
	u32 iCursor;
	u32 iSize;
	u32 iMaxSize;
	std::mutex*	pMutex;

	void write(u8* buf, u32 size)
	{
		if (iSize < size) return;
		pMutex->lock();
		memcpy(pBufferAddr + iCursor, buf, size);
		iCursor += size;
		iSize -= size;
		pMutex->unlock();
	}

	int read(u8* buf, u32 size)
	{
		if (iCursor == 0) return -1;
		pMutex->lock();
		int ret = FFMIN(iCursor, size);
		memcpy(buf, pBufferAddr, ret);
		iSize += ret;
		iCursor -= ret;
		pMutex->unlock();
		return ret;
	}
}MemoryBuffer;


#define AVIO_MIN(a,b) ((a) > (b) ? (b) : (a))

class ITimer {
public:
	ITimer() {};
	virtual ~ITimer() {}
	virtual void start() = 0;
	virtual void wait() = 0;
};
template <class Rep, class Period> class Timer : public ITimer {
	std::chrono::duration<Rep, Period> Rel_Time;
	std::chrono::time_point<std::chrono::high_resolution_clock> StartTime;
	std::chrono::time_point<std::chrono::high_resolution_clock> StopTime;

public:
	Timer(const std::chrono::duration<Rep, Period> &rel_time) : Rel_Time(rel_time) {};
	virtual ~Timer() {}
	virtual void start() { StartTime = std::chrono::high_resolution_clock::now(); }
	virtual void wait()
	{
		auto duration = std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(std::chrono::high_resolution_clock::now() - StartTime);
		auto timetowait = Rel_Time - duration;
		if (timetowait.count() > 0) {
			std::this_thread::sleep_for(timetowait);
		}
	}
};

class PPDecoder
{
public:
	// io context
	MemoryBuffer*												pVideoIOBuffer;

	// video stream
	AVCodecParserContext*		pVideoParser;
	AVCodecContext*				pVideoContext;
	AVPacket*					pVideoPacket;
	AVFrame*					pVideoFrame;
	u32							iVideoFrameIndex = 0;


	int							mFrameRate = 30;
	static void					printError(int errorCode);
public:
	PPDecoder();
	~PPDecoder();

	void initDecoder();

	void appendBuffer(u8* buffer, u32 size);

	void initVideoStream();
	void initAudioStream();

	void decodeVideoStream();
	void decodeAudioStream();

	void startDecodeThread();
};