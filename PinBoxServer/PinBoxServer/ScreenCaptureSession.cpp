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

static FILE* testAudioOutFLAC;

ScreenCaptureSession::ScreenCaptureSession()
{
}


ScreenCaptureSession::~ScreenCaptureSession()
{
	m_frameGrabber->pause();

	fclose(testAudioOutFLAC);
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
		//if (!m_isStartStreaming || m_clientSession == nullptr) return;

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

		encodeAudioFrame();

		// encode video
		encodeVideoFrame((u8*)img.Data);

		//=================================================================
		captureFPS.onNewFramecounter++;
		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - captureFPS.onNewFramestart).count() >= 1000) {
			captureFPS.currentFPS = captureFPS.onNewFramecounter;
			captureFPS.onNewFramecounter = 0;
			captureFPS.onNewFramestart = std::chrono::high_resolution_clock::now();
		}
		//std::cout << "Capture FPS: " << captureFPS.currentFPS << std::endl << std::flush;
	})->start_capturing();
	int timeDelay = 1000.0f / (float)mFrameRate;
	m_frameGrabber->setFrameChangeInterval(std::chrono::milliseconds(timeDelay));
	//m_frameGrabber->pause();

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
	if(m_clientSession != nullptr) m_clientSession->PreparePacketAndSend(pVideoPacket->data, pVideoPacket->size);

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

static int init_converted_samples(uint8_t ***converted_input_samples,AVCodecContext *output_codec_context,int frame_size)
{
	int error;

	/* Allocate as many pointers as there are audio channels.
	* Each pointer will later point to the audio samples of the corresponding
	* channels (although it may be NULL for interleaved formats).
	*/
	if (!(*converted_input_samples = (uint8_t **)calloc(output_codec_context->channels,
		sizeof(**converted_input_samples)))) {
		fprintf(stderr, "Could not allocate converted input sample pointers\n");
		return AVERROR(ENOMEM);
	}

	/* Allocate memory for the samples of all channels in one consecutive
	* block for convenience. */
	if ((error = av_samples_alloc(*converted_input_samples, NULL,
		output_codec_context->channels,
		frame_size,
		output_codec_context->sample_fmt, 0)) < 0) {

		av_freep(&(*converted_input_samples)[0]);
		free(*converted_input_samples);
		return error;
	}
	return 0;
}

void ScreenCaptureSession::encodeAudioFrame()
{
	m_audioGrabber->BeginReadAudioBuffer();
	u32 size = m_audioGrabber->GetAudioBufferSize();
	const u32 frameSize = pAudioContext->frame_size;

	while (size >= frameSize * 4)
	{
		int ret = av_frame_make_writable(pAudioFrame);
		if (ret < 0) ERROR_PRINT(ret);
		memcpy(pAudioFrame->data[0], m_audioGrabber->GetAudioBuffer(), frameSize * 4);

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
			fwrite(pAudioPacket->data, 1, pAudioPacket->size, testAudioOutFLAC);

			av_packet_unref(pAudioPacket);
		}
		m_audioGrabber->Cutoff(frameSize * 4, frameSize);
		size = m_audioGrabber->GetAudioBufferSize();
	}

	m_audioGrabber->FinishReadAudioBuffer();

	
	//m_audioGrabber->BeginReadAudioBuffer();
	//u32 size = m_audioGrabber->GetAudioBufferSize();
	//const u32 frameSize = pAudioContext->frame_size;
	//const u32 inputFrameSize = 1152;
	//// TODO: use this frame idx to sync with video
	//u32 syncFrameIdx = iVideoFrameIndex;

	//int finished = 0;
	//while (size > inputFrameSize && av_audio_fifo_size(pAudioFIFO) < frameSize)
	//{
	//	uint8_t **convertedSamples = NULL;
	//	int ret = init_converted_samples(&convertedSamples, pAudioContext, inputFrameSize);
	//	if (ret)
	//	{
	//		std::cout << "[Error][Audio] failed when init converted samples" << std::endl << std::flush;
	//		m_audioGrabber->FinishReadAudioBuffer();
	//		return;
	//	}
	//	const uint8_t *input_samples = (const uint8_t*)m_audioGrabber->GetAudioBuffer();
	//	// convert
	//	ret = swr_convert(pAudioResampler, convertedSamples, frameSize, &input_samples, inputFrameSize);
	//	if (ret < 0)
	//	{
	//		std::cout << "[Error][Audio] failed when convert samples" << std::endl << std::flush;
	//		finished = 1;
	//		break;
	//	}
	//	// stored to fifo
	//	ret = av_audio_fifo_realloc(pAudioFIFO, av_audio_fifo_size(pAudioFIFO) + frameSize);
	//	if (ret < 0)
	//	{
	//		std::cout << "[Error][Audio] failed when realloc fifo" << std::endl << std::flush;
	//		m_audioGrabber->FinishReadAudioBuffer();
	//		return;
	//	}

	//	if (av_audio_fifo_write(pAudioFIFO, (void**)convertedSamples, frameSize) < frameSize)
	//	{
	//		std::cout << "[Error][Audio] could not write data to FIFO" << std::endl << std::flush;
	//		m_audioGrabber->FinishReadAudioBuffer();
	//		return;
	//	}

	//	m_audioGrabber->Cutoff(inputFrameSize * 4, inputFrameSize);
	//	size = m_audioGrabber->GetAudioBufferSize();
	//}

	///* If we have enough samples for the encoder, we encode them.
	//* At the end of the file, we pass the remaining samples to
	//* the encoder. */
	//while (av_audio_fifo_size(pAudioFIFO) >= frameSize || (finished && av_audio_fifo_size(pAudioFIFO) > 0))
	//{
	//	const int frameSizeMin = FFMIN(av_audio_fifo_size(pAudioFIFO), frameSize);
	//	int ret = av_frame_make_writable(pAudioFrame);
	//	if (av_audio_fifo_read(pAudioFIFO, (void**)pAudioFrame->data, frameSizeMin) < frameSizeMin)
	//	{
	//		std::cout << "[Error][Audio] could not read data from FIFO" << std::endl << std::flush;
	//		m_audioGrabber->FinishReadAudioBuffer();
	//		return;
	//	}
	//	if (pAudioFrame)
	//	{
	//		pAudioFrame->pts = iAudioPts;
	//		iAudioPts += pAudioFrame->nb_samples;
	//	}
	//	ret = avcodec_send_frame(pAudioContext, pAudioFrame);
	//	if (ret >= 0)
	//	{
	//		ret = avcodec_receive_packet(pAudioContext, pAudioPacket);
	//		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	//		{
	//			av_packet_unref(pAudioPacket);
	//			m_audioGrabber->FinishReadAudioBuffer();
	//			return;
	//		}
	//		if (ret < 0) {
	//			av_packet_unref(pAudioPacket);
	//			m_audioGrabber->FinishReadAudioBuffer();
	//			return;
	//		}
	//		//TEST: write to file
	//		fwrite(pAudioPacket->data, 1, pAudioPacket->size, testAudioOutFLAC);
	//		av_packet_unref(pAudioPacket);
	//	}
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
	pVideoContext->bit_rate = 960000;
	pVideoContext->width = 400;
	pVideoContext->height = 240;
	pVideoContext->time_base = AVRational { 1, mFrameRate };
	pVideoContext->framerate = AVRational { mFrameRate, 1 };
	pVideoContext->gop_size = 15;
	pVideoContext->max_b_frames = 1;
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
	pAudioContext->sample_rate = 48000;
	pAudioContext->channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);;
	pAudioContext->channel_layout = AV_CH_LAYOUT_STEREO;
	/* Allow the use of the experimental AAC encoder. */
	pAudioContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
	pAudioContext->block_align = 4;

	// Open
	int ret = avcodec_open2(pAudioContext, audioCodec, NULL);

	pAudioPacket = av_packet_alloc();
	pAudioFrame = av_frame_alloc();
	pAudioFrame->nb_samples = pAudioContext->frame_size;
	pAudioFrame->format = pAudioContext->sample_fmt;
	pAudioFrame->channel_layout = pAudioContext->channel_layout;
	ERROR_PRINT(av_frame_get_buffer(pAudioFrame, 0));
	// init resampler
	pAudioResampler = swr_alloc_set_opts(NULL,// output
		pAudioContext->channel_layout,
		pAudioContext->sample_fmt,
		pAudioContext->sample_rate,
		// input
		av_get_default_channel_layout(2),
		AV_SAMPLE_FMT_FLT,
		48000,
		0, NULL);
	if (!pAudioResampler) {
		fprintf(stderr, "Could not allocate resample context\n");
		return;
	}

	int error = 0;
	if ((error = swr_init(pAudioResampler)) < 0) {
		fprintf(stderr, "Could not open resample context\n");
		swr_free(&pAudioResampler);
		return;
	}

	// init audio fifo
	pAudioFIFO = av_audio_fifo_alloc(pAudioContext->sample_fmt, pAudioContext->channels, 1);
	if(!pAudioFIFO)
	{
		fprintf(stderr, "Could not allocate FIFO\n");
		return;
	}

	// test
	testAudioOutFLAC = fopen("test_audio.wav", "wb");

	mInitializedCodec = true;
}

void ScreenCaptureSession::startStream()
{
	if (m_isStartStreaming) return;
	m_isStartStreaming = true;
	iVideoFrameIndex = 0;
	m_frameGrabber->resume();
}

void ScreenCaptureSession::stopStream()
{
	if (!m_isStartStreaming) return;
	m_isStartStreaming = false;
	m_frameGrabber->pause();
}

void ScreenCaptureSession::registerClientSession(PPClientSession* sesison)
{
	m_clientSession = sesison;
}
