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
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/timestamp.h>
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

#define TRANSFER_BUFFER_SIZE 0x1000

typedef struct FrameData {
	uint8_t*				DataAddr;
	int						Width;
	int						Height;
	int						StrideWidth;
	bool					Drew;
}FrameData;

typedef struct OutputStream
{
	AVStream					*Stream;
	AVCodecContext				*CodecContext;
	AVFrame						*Frame;
	AVFrame						*TmpFrame;
	struct SwsContext			*SWSContext;
	struct SwrContext			*SWRContext;
	AVPacket					*Packet;

	int							SamplesCount;
	int64_t						NextFrameIdx ;
};

class ScreenCaptureSession
{
private:
	PPServer*													m_server;
	PPClientSession*											m_clientSession = nullptr;

	std::shared_ptr<SL::Screen_Capture::IScreenCaptureManager>	m_frameGrabber;
	AudioStreamSession*											m_audioGrabber;
	u32															m_frameIndex = 0;
	bool														m_isStartStreaming = false;

	u8															m_encodeType = ENCODE_TYPE_MPEG4;
	u8*															m_staticPacketBuffer = nullptr;

	//---------------------------------------------------------------------------------------------------------------------------
	//--------------------MPEG4 ENCODE-------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------
	AVFormatContext												*mFormatContext;
	AVOutputFormat												*mOutputFormat;
	OutputStream												mVideoStream;
	OutputStream												mAudioStream;
	AVDictionary												*mDebugVideoFileOptions = NULL;
	const AVCodec												*mVideoCodec;
	const AVCodec												*mAudioCodec;
	int															mFrameRate;

	void														initVideoStream(int srcW, int srcH);
	void														initAudioStream();

	AVFrame*													getVideoFrame(FrameData* frameData);
	void														writeVideoFrame(FrameData* frameData);

	AVFrame*													getAudioFrame();
	void														writeAudioFrame();

	void														closeStream(OutputStream* stream);

	bool														mInitializedCodec = false;
	u32															mLastSentFrame = 0;
	volatile bool												mIsStopEncode = false;
	void														initEncoder(FrameData* frameData);
public:
	ScreenCaptureSession();
	~ScreenCaptureSession();

	bool										mIsFirstFrame = true;
	FrameData*									mLastFrameData = nullptr;
	std::thread									g_thread;
	std::mutex									*g_threadMutex;
	static void									onProcessUpdateThread(void* context);

	/*std::thread								g_serverThread;
	std::mutex									*g_serverMutex;
	static void									serverProgress(void* context);*/

	void										serverUpdate();

	void										startStream();
	void										stopStream();
	void										registerClientSession(PPClientSession* sesison);
	void										initScreenCaptuure(PPServer* parent);

	u32											currentFrame() const { return m_frameIndex; }
	bool										isStreaming() const { return m_isStartStreaming; }
};

#endif