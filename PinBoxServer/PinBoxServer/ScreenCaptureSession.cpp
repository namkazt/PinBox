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

ScreenCaptureSession::ScreenCaptureSession()
{
}


ScreenCaptureSession::~ScreenCaptureSession()
{
	m_frameGrabber->pause();

	//fclose(testAudioOutFLAC);
}



void ScreenCaptureSession::initScreenCapture(PPServer* parent)
{
	if (Initialized) return;

	Initialized = true;
	m_server = parent;
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
		if (!m_isStartStreaming || m_clientSession == nullptr) return;

		int totalSize = 0;
		if (mLastFrameData == nullptr) {
			mLastFrameData = new FrameData();
			mLastFrameData->Width = Width(img);
			mLastFrameData->Height = Height(img);
			mLastFrameData->StrideWidth = mLastFrameData->Width * img.Pixelstride + img.RowPadding;
			totalSize = mLastFrameData->StrideWidth * mLastFrameData->Height;
		}else
		{
			totalSize = mLastFrameData->StrideWidth * mLastFrameData->Height;
		}

		//encodeAudioFrame();

		// encode video
		encodeVideoFrame((u8*)img.Data);

	})->start_capturing();
	int timeDelay = 1000.0f / (float)mFrameRate;
	m_frameGrabber->setFrameChangeInterval(std::chrono::milliseconds(timeDelay));
	m_frameGrabber->pause();

	//-----------------------------------------------------
	// decoder
	//m_audioGrabber = new AudioStreamSession();
	//m_audioGrabber->StartAudioStream();
	//m_audioGrabber->Pause();

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
	//m_audioGrabber->BeginReadAudioBuffer();
	//u32 size = m_audioGrabber->GetAudioBufferSize();
	//const u32 frameSize = pAudioContext->frame_size;

	//while (size >= frameSize * 4)
	//{
	//	int ret = av_frame_make_writable(pAudioFrame);
	//	if (ret < 0) ERROR_PRINT(ret);
	//	memcpy(pAudioFrame->data[0], m_audioGrabber->GetAudioBuffer(), frameSize * 4);

	//	pAudioFrame->pts = iAudioPts;
	//	iAudioPts += pAudioFrame->nb_samples;
	//	
	//	ret = avcodec_send_frame(pAudioContext, pAudioFrame);
	//	while (ret >= 0)
	//	{
	//		ret = avcodec_receive_packet(pAudioContext, pAudioPacket);
	//		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	//			break;
	//		if (ret < 0) break;

	//		//TODO: do something with packet data
	//		if (m_clientSession != nullptr) m_clientSession->PrepareAudioPacketAndSend(pAudioPacket->data, pAudioPacket->size, pAudioFrame->pts);
	//		//fwrite(pAudioPacket->data, 1, pAudioPacket->size, testAudioOutFLAC);

	//		av_packet_unref(pAudioPacket);
	//	}
	//	m_audioGrabber->Cutoff(frameSize * 4, frameSize);
	//	size = m_audioGrabber->GetAudioBufferSize();
	//}

	//m_audioGrabber->FinishReadAudioBuffer();
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
	pVideoContext->bit_rate = 720000;
	pVideoContext->width = 400;
	pVideoContext->height = 240;
	pVideoContext->time_base = AVRational { 1, mFrameRate };
	pVideoContext->framerate = AVRational { mFrameRate, 1 };
	pVideoContext->gop_size = 20;
	pVideoContext->max_b_frames = 2;
	//pVideoContext->block_align = 4;
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
			SWS_FAST_BILINEAR, 0, 0, 0);
	// Init custom memory buffer
	pVideoIOBuffer = new MemoryBuffer();
	pVideoIOBuffer->pBufferAddr = (u8*)malloc(MEMORY_BUFFER_SIZE + MEMORY_BUFFER_PADDING);
	memset(pVideoIOBuffer->pBufferAddr + MEMORY_BUFFER_SIZE, 0, MEMORY_BUFFER_PADDING);
	pVideoIOBuffer->iSize = MEMORY_BUFFER_SIZE;
	pVideoIOBuffer->iMaxSize = MEMORY_BUFFER_SIZE;
	pVideoIOBuffer->iCursor = 0;
	pVideoIOBuffer->pMutex = new std::mutex();

	//-----------------------------------------------------------------
	// init audio encoder
	//-----------------------------------------------------------------
	const AVCodec* audioCodec = avcodec_find_encoder(AV_CODEC_ID_MP2);
	pAudioContext = avcodec_alloc_context3(audioCodec);
	pAudioContext->bit_rate = 64000;
	pAudioContext->sample_fmt = audioCodec->sample_fmts[0];
	pAudioContext->sample_rate = 44100;
	pAudioContext->channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);;
	pAudioContext->channel_layout = AV_CH_LAYOUT_STEREO;
	/* Allow the use of the experimental AAC encoder. */
	//pAudioContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
	//pAudioContext->block_align = 4;

	// Open
	int ret = avcodec_open2(pAudioContext, audioCodec, NULL);

	pAudioPacket = av_packet_alloc();
	pAudioFrame = av_frame_alloc();
	pAudioFrame->nb_samples = pAudioContext->frame_size;
	pAudioFrame->format = pAudioContext->sample_fmt;
	pAudioFrame->channel_layout = pAudioContext->channel_layout;
	ERROR_PRINT(av_frame_get_buffer(pAudioFrame, 0));

	// test
	//testAudioOutFLAC = fopen("test_audio.wav", "wb");

	mInitializedCodec = true;
}

void ScreenCaptureSession::startStream()
{
	if (m_isStartStreaming) return;
	m_isStartStreaming = true;
	iVideoFrameIndex = 0;
	m_frameGrabber->resume();
	//m_audioGrabber->Resume();
}

void ScreenCaptureSession::stopStream()
{
	if (!m_isStartStreaming) return;
	m_isStartStreaming = false;
	m_frameGrabber->pause();
	//m_audioGrabber->Pause();
}

void ScreenCaptureSession::registerClientSession(PPClientSession* sesison)
{
	m_clientSession = sesison;
}
