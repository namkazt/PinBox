#include "stdafx.h"

#include "ScreenCaptureSession.h"

#include "PPServer.h"
#include "ServerConfig.h"
#include <locale>

#define ERROR_PRINT(code) printError(code)
static void printError(int errorCode)
{
	if (errorCode >= 0) return;
	char log[AV_ERROR_MAX_STRING_SIZE]{ 0 };
	std::cout << "Error: " << av_strerror(errorCode, log, AV_ERROR_MAX_STRING_SIZE) << std::endl;
}

//=========================================================
// FPS counter
//=========================================================
FPSCounter vFPS;
FPSCounter captureFPS;

//static FILE* testAudioOutFLAC;



// test
int64_t src_ch_layout = AV_CH_LAYOUT_STEREO, dst_ch_layout = AV_CH_LAYOUT_STEREO;
int src_rate = 48000, dst_rate = 22050;
uint8_t **src_data = NULL;
int src_nb_channels = 2, dst_nb_channels = 2;
int src_linesize, dst_linesize;
enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16, dst_sample_fmt = AV_SAMPLE_FMT_S16;
struct SwrContext *swr_ctx;



ScreenCaptureSession::ScreenCaptureSession()
{
}


ScreenCaptureSession::~ScreenCaptureSession()
{
	// destroy screen capture
	m_frameGrabber->pause();
	m_frameGrabber = nullptr;

	releaseEncoder();

	//fclose(testAudioOutFLAC);
}



void ScreenCaptureSession::initScreenCapture()
{
	if (Initialized) return;
	if (!m_server) return;

	Initialized = true;
	mFrameRate = ServerConfig::Get()->CaptureFPS;
	//---------------------------------------------------------------------
	auto mons = SL::Screen_Capture::GetMonitors();
	std::vector<SL::Screen_Capture::Monitor> selectedMonitor = std::vector<SL::Screen_Capture::Monitor>();
	SL::Screen_Capture::Monitor monitor = mons[ServerConfig::Get()->MonitorIndex];
	selectedMonitor.push_back(monitor);

	//---------------------------------------------------------------------
	m_frameGrabber = nullptr;
	m_frameGrabber = SL::Screen_Capture::CreateCaptureConfiguration([&]()
	{
		return selectedMonitor;
	})->onNewFrame([&](const SL::Screen_Capture::Image& img, const SL::Screen_Capture::Monitor& monitor)
	{
		if (!mInitializedCodec) return;
		//if (!m_isStartStreaming || m_clientSession == nullptr) return;

		auto size = Width(img) * Height(img) * sizeof(SL::Screen_Capture::ImageBGRA);
		auto imgbuffer(std::make_unique<unsigned char[]>(size));
		Extract(img, imgbuffer.get(), size);

		int totalSize = 0;
		if (mLastFrameData == nullptr) {
			mLastFrameData = new FrameData();
			mLastFrameData->Width = Width(img);
			mLastFrameData->Height = Height(img);
			mLastFrameData->StrideWidth = mLastFrameData->Width * 4;
			totalSize = mLastFrameData->StrideWidth * mLastFrameData->Height;
		}else
		{
			totalSize = mLastFrameData->StrideWidth * mLastFrameData->Height;
		}

		encodeAudioFrame();

		// encode video
		encodeVideoFrame((u8*)imgbuffer.get());

	})->start_capturing();
	int timeDelay = 1000.0f / (float)mFrameRate;
	m_frameGrabber->setFrameChangeInterval(std::chrono::milliseconds(timeDelay));

	//-----------------------------------------------------
	// decoder
	m_audioGrabber = new AudioStreamSession();
	m_audioGrabber->StartAudioStream();

	//-----------------------------------------------------
	// decoder
	iSourceWidth = monitor.Width;
	iSourceHeight = monitor.Height;
	initEncoder();
}

void ScreenCaptureSession::encodeVideoFrame(u8* buf)
{
	// encode
	const unsigned char *inData[1] = { buf };
	int inLinesize[1] = { mLastFrameData->StrideWidth };
	ERROR_PRINT(sws_scale(pVideoScaler, inData, inLinesize, 0, mLastFrameData->Height, pVideoFrame->data, pVideoFrame->linesize));
	pVideoFrame->pts = iVideoFrameIndex;
	iVideoFrameIndex++;
	int ret = avcodec_send_frame(pVideoContext, pVideoFrame);
	ERROR_PRINT(ret);
	//======================================================
	ret = avcodec_receive_packet(pVideoContext, pVideoPacket);
	ERROR_PRINT(ret);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return;
	if (ret < 0) return;
	//======================================================
	if(m_clientSession != nullptr) m_clientSession->PrepareVideoPacketAndSend(pVideoPacket->data, pVideoPacket->size);

	//======================================================
	// FPS sent
	//======================================================
	vFPS.onNewFramecounter++;
	if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - vFPS.onNewFramestart).count() >= 1000) {
		vFPS.currentFPS = vFPS.onNewFramecounter;
		vFPS.onNewFramecounter = 0;
		vFPS.onNewFramestart = std::chrono::high_resolution_clock::now();
	}
	//======================================================
	av_packet_unref(pVideoPacket);
}

void ScreenCaptureSession::encodeAudioFrame()
{
	float factor = (float)src_rate / (float)dst_rate;

	u32 size = m_audioGrabber->audioBufferSize;

	const u32 frameSize = pAudioFrame->nb_samples * factor;
	const u32 readSize = frameSize * 4;

	while (size >= readSize)
	{
		int ret = av_frame_make_writable(pAudioFrame);
		if (ret < 0)
		{
			fprintf(stderr, "Fail to init Audio frame\n");
			break;
		}

		m_audioGrabber->ReadFromBuffer(src_data[0], readSize);

		/* convert to destination format */
		ret = swr_convert(swr_ctx, pAudioFrame->data, pAudioFrame->nb_samples, (const uint8_t **)src_data, frameSize);
		if (ret < 0) {
			fprintf(stderr, "Error while converting\n");
			break;
		}
		pAudioFrame->pts = iAudioPts;
		iAudioPts += pAudioFrame->nb_samples;
		
		ret = avcodec_send_frame(pAudioContext, pAudioFrame);
		while (ret >= 0)
		{
			ret = avcodec_receive_packet(pAudioContext, pAudioPacket);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			if (ret < 0) break;

			//TODO: do something with packet data
			if (m_clientSession != nullptr) m_clientSession->PrepareAudioPacketAndSend(pAudioPacket->data, pAudioPacket->size, pAudioFrame->pts);
			//fwrite(pAudioPacket->data, 1, pAudioPacket->size, testAudioOutFLAC);

			av_packet_unref(pAudioPacket);
		}

		size = m_audioGrabber->audioBufferSize;
	}
}

void ScreenCaptureSession::initEncoder()
{
	av_register_all();

	//-----------------------------------------------------------------
	// init video encoder
	//-----------------------------------------------------------------
	const AVCodec* videoCodec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
	pVideoContext = avcodec_alloc_context3(videoCodec);
	// TODO: update by config here
	pVideoContext->bit_rate = 1000000;
	pVideoContext->width = 400;
	pVideoContext->height = 240;
	pVideoContext->time_base = AVRational { 1, mFrameRate };
	pVideoContext->framerate = AVRational { mFrameRate, 1 };
	pVideoContext->gop_size = 20;
	pVideoContext->max_b_frames = 2;
	pVideoContext->block_align = 8;
	pVideoContext->pix_fmt = AV_PIX_FMT_YUV420P;
	// Open
	ERROR_PRINT(avcodec_open2(pVideoContext, videoCodec, NULL));
	pVideoPacket = av_packet_alloc();
	pVideoFrame = av_frame_alloc();
	pVideoFrame->format = pVideoContext->pix_fmt;
	pVideoFrame->width = pVideoContext->width;
	pVideoFrame->height = pVideoContext->height;
	ERROR_PRINT(av_frame_get_buffer(pVideoFrame, 8)); // align 8 ?
	pVideoScaler = sws_getContext(
			iSourceWidth, iSourceHeight, AV_PIX_FMT_BGRA,
			pVideoContext->width, pVideoContext->height, AV_PIX_FMT_YUV420P,
				SWS_BICUBIC, 0, 0, 0);

	//-----------------------------------------------------------------
	// init audio encoder
	//-----------------------------------------------------------------
	const AVCodec* audioCodec = avcodec_find_encoder(AV_CODEC_ID_MP2);
	pAudioContext = avcodec_alloc_context3(audioCodec);
	//pAudioContext->time_base = AVRational{ 1 , 30 };
	pAudioContext->bit_rate = 64000;
	pAudioContext->sample_fmt = audioCodec->sample_fmts[0];
	pAudioContext->sample_rate = dst_rate;
	pAudioContext->channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);;
	pAudioContext->channel_layout = AV_CH_LAYOUT_STEREO;
	pAudioContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
	int ret = avcodec_open2(pAudioContext, audioCodec, NULL);

	pAudioPacket = av_packet_alloc();
	pAudioFrame = av_frame_alloc();
	pAudioFrame->nb_samples = pAudioContext->frame_size;
	pAudioFrame->format = pAudioContext->sample_fmt;
	pAudioFrame->channel_layout = pAudioContext->channel_layout;
	ERROR_PRINT(av_frame_get_buffer(pAudioFrame, 0));



	/* create resampler context */
	swr_ctx = swr_alloc();
	if (!swr_ctx) {
		fprintf(stderr, "Could not allocate resampler context\n");
		ret = AVERROR(ENOMEM);
		return;
	}

	/* set options */
	av_opt_set_int(swr_ctx, "in_channel_layout", src_ch_layout, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate", src_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);

	av_opt_set_int(swr_ctx, "out_channel_layout", dst_ch_layout, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate", dst_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

	/* initialize the resampling context */
	if ((ret = swr_init(swr_ctx)) < 0) {
		fprintf(stderr, "Failed to initialize the resampling context\n");
		return;
	}

	float factor = (float)src_rate / (float)dst_rate;

	/* allocate source and destination samples buffers */
	src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);
	ret = av_samples_alloc_array_and_samples(&src_data, &src_linesize, src_nb_channels, factor * pAudioFrame->nb_samples, src_sample_fmt, 0);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate source samples\n");
		return;
	}


	// test
	//testAudioOutFLAC = fopen("test_audio.bin", "wb");

	mInitializedCodec = true;
}

void ScreenCaptureSession::releaseEncoder()
{
	// free video
	avcodec_free_context(&pVideoContext);
	av_frame_free(&pVideoFrame);
	av_packet_free(&pVideoPacket);
	// free audio
	avcodec_free_context(&pAudioContext);
	av_frame_free(&pAudioFrame);
	av_packet_free(&pAudioPacket);

	delete mLastFrameData;
}


void ScreenCaptureSession::setParent(PPServer* parent)
{
	m_server = parent;
}

void ScreenCaptureSession::startStream()
{
	if (m_isStartStreaming) return;
	m_isStartStreaming = true;
	iVideoFrameIndex = 0;

	initScreenCapture();
}

void ScreenCaptureSession::stopStream()
{
	if (!m_isStartStreaming) return;
	m_isStartStreaming = false;
	Initialized = false;
	// release frame grabber
	m_frameGrabber->pause();
	m_frameGrabber = nullptr;
	// release audio
	m_audioGrabber->Pause();

	// release encoder
	releaseEncoder();
}

void ScreenCaptureSession::registerClientSession(PPClientSession* sesison)
{
	m_clientSession = sesison;
}
