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



ScreenCaptureSession::ScreenCaptureSession()
{
}


ScreenCaptureSession::~ScreenCaptureSession()
{
	m_frameGrabber->pause();
}



void ScreenCaptureSession::initScreenCaptuure(PPServer* parent)
{
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

			encodeVideoFrame((u8*)img.Data);

			//=================================================================
			captureFPS.onNewFramecounter++;
			if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - captureFPS.onNewFramestart).count() >= 1000) {
				captureFPS.currentFPS = captureFPS.onNewFramecounter;
				captureFPS.onNewFramecounter = 0;
				captureFPS.onNewFramestart = std::chrono::high_resolution_clock::now();
			}
			std::cout << "Capture FPS: " << captureFPS.currentFPS << std::endl << std::flush;
		}
	)->start_capturing();
	int timeDelay = 1000.0f / (float)mFrameRate;
	m_frameGrabber->setFrameChangeInterval(std::chrono::milliseconds(timeDelay));
	m_frameGrabber->pause();
	//-----------------------------------------------------
	// decoder
	iSourceWidth = monitor.Width;
	iSourceHeight = monitor.Height;
	initEncoder();
	//-----------------------------------------------------
	// start loopback record thread
	g_threadMutex = new std::mutex();
}

void ScreenCaptureSession::onProcessUpdateThread(void* context)
{
	ScreenCaptureSession* self = (ScreenCaptureSession*)context;
	int timeDelay = 1000.0f / (float)self->mFrameRate;
	auto timer = new SL::Screen_Capture::Timer<int, std::milli>( std::chrono::duration<int, std::milli>(1));
	//======================================================
	// start loop
	//======================================================
	while(!self->mIsStopEncode)
	{
		//======================================================
		// stream do not start yet so we keep thread alive here
		if (!self->m_isStartStreaming || self->m_clientSession == nullptr)
		{
			timer->start();
			timer->wait();
			continue;
		}

		//======================================================
		// server update
		//======================================================
		timer->start();
		self->serverUpdate();
		timer->wait();


		//======================================================
		// test
		//======================================================
		//if (self->iVideoFrameIndex - self->mLastSentFrame > (self->mFrameRate * 5))
		//{
		//	// flush video frames
		//	avcodec_send_frame(self->pVideoContext, NULL);
		//	self->mIsStopEncode = true;
		//	break;
		//}
	}

	//======================================================
	// test
	//======================================================
	/*FILE *file = fopen("custom_io.flv", "wb");
	fwrite(self->pVideoIOBuffer->pBufferAddr, 1, self->pVideoIOBuffer->iCursor, file);
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	fwrite(endcode, 1, sizeof(endcode), file);
	fclose(file);
	std::cout << "Finish Test Local file" << std::endl;*/

	//======================================================
	// free video encoder
	//======================================================
	avcodec_free_context(&self->pVideoContext);
	av_frame_free(&self->pVideoFrame);
	av_packet_free(&self->pVideoPacket);
	sws_freeContext(self->pVideoScaler);
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
	else if (ret < 0) return;
	//======================================================
	m_clientSession->PreparePacketAndSend(pVideoPacket->data, pVideoPacket->size);

	//======================================================
	// FPS sent
	//======================================================
	vFPS.onNewFramecounter++;
	if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - vFPS.onNewFramestart).count() >= 1000) {
		vFPS.currentFPS = vFPS.onNewFramecounter;
		vFPS.onNewFramecounter = 0;
		vFPS.onNewFramestart = std::chrono::high_resolution_clock::now();
	}
	std::cout << "FPS: " << vFPS.currentFPS << " | FrameSize : " << pVideoPacket->size << std::endl << std::flush;


	//======================================================
	av_packet_unref(pVideoPacket);
}


void ScreenCaptureSession::serverUpdate()
{
	if (!m_isStartStreaming || m_clientSession == nullptr) return;

	//-------------------------------------------------
	// progress input on each frame
	//-------------------------------------------------
	m_server->InputStreamer->ProcessInput();

	//-------------------------------------------------
	// grab packet and send to client
	//-------------------------------------------------
	//m_clientSession->g_ss_isReceived && 
	
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
	pVideoContext->max_b_frames = 1;
	pVideoContext->pix_fmt = AV_PIX_FMT_YUV420P;
	// Open
	ERROR_PRINT(avcodec_open2(pVideoContext, videoCodec, NULL));
	pVideoPacket = av_packet_alloc();
	pVideoFrame = av_frame_alloc();
	pVideoFrame->format = pVideoContext->pix_fmt;
	pVideoFrame->width = pVideoContext->width;
	pVideoFrame->height = pVideoContext->height;
	ERROR_PRINT(av_frame_get_buffer(pVideoFrame, 8));
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
}



void ScreenCaptureSession::startStream()
{
	m_isStartStreaming = true;
	iVideoFrameIndex = 0;
	m_frameGrabber->resume();
	//m_audioGrabber->Resume();
}

void ScreenCaptureSession::stopStream()
{
	m_isStartStreaming = false;
	m_frameGrabber->pause();
}

void ScreenCaptureSession::registerClientSession(PPClientSession* sesison)
{
	m_clientSession = sesison;
}
