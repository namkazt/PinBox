#pragma once

#ifndef _SCREEN_CAPTURE_SESSION_H__
#define _SCREEN_CAPTURE_SESSION_H__

// frame capture
#include "ScreenCapture.h"
#include "PPMessage.h"
#include "PPClientSession.h"

//encode
#include "webp/encode.h"
#include <turbojpeg.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include "AudioStreamSession.h"
#include <thread>


//ffmpeg
extern "C" {
#include <libavutil/common.h>
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

typedef struct
{
	u8* piece;
	u32 size;
	u8 index;
	u32 frameIndex;
} FramePiece;

typedef std::function<void()> OnClientReallyClose;

#define ENCODE_TYPE_WEBP 0x01
#define ENCODE_TYPE_JPEG_TURBO 0x02
#define ENCODE_TYPE_MPEG4 0x03

#define TRANSFER_BUFFER_SIZE 0x50000

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

typedef struct FPSCounter {
	u32 onNewFramecounter = 0;
	u32 currentFPS = 0;
	std::chrono::time_point<std::chrono::steady_clock> onNewFramestart = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> onFrameChanged = std::chrono::high_resolution_clock::now();
};

typedef struct FrameData {
	uint8_t*				DataAddr;
	int						Width;
	int						Height;
	int						StrideWidth;
	bool					Drew;
}FrameData;


class ScreenCaptureSession
{
private:
	PPServer*													m_server;
	PPClientSession*											m_clientSession = nullptr;

	std::shared_ptr<SL::Screen_Capture::IScreenCaptureManager>	m_frameGrabber;
	AudioStreamSession*											m_audioGrabber;
	bool														m_isStartStreaming = false;

	u8															m_encodeType = ENCODE_TYPE_MPEG4;



	int															mFrameRate;

	// Video
	int															iSourceWidth;
	int															iSourceHeight;

	AVCodecContext*												pVideoContext;
	AVPacket*													pVideoPacket;
	AVFrame*													pVideoFrame;
	SwsContext*													pVideoScaler;
	u32															iVideoFrameIndex = 0;
	void														encodeVideoFrame(u8* buf);

	// custom IO
	MemoryBuffer*												pVideoIOBuffer;
	u8*															pTransferBuffer;

	bool														mInitializedCodec = false;
	u32															mLastSentFrame = 0;
	volatile bool												mIsStopEncode = false;
	void														initEncoder();
public:
	ScreenCaptureSession();
	~ScreenCaptureSession();

	bool										mIsFirstFrame = true;
	FrameData*									mLastFrameData = nullptr;
	std::thread									g_thread;
	std::mutex									*g_threadMutex;
	static void									onProcessUpdateThread(void* context);


	void										serverUpdate();

	void										startStream();
	void										stopStream();
	void										registerClientSession(PPClientSession* sesison);
	void										initScreenCaptuure(PPServer* parent);

	u32											currentFrame() const { return iVideoFrameIndex; }
	bool										isStreaming() const { return m_isStartStreaming; }
};

#endif