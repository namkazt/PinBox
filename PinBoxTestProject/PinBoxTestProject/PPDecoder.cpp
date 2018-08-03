#include "PPDecoder.h"
#include <iostream>
#include <opencv2/core/mat.hpp>
#include <opencv2/shape/hist_cost.hpp>
#include <opencv2/highgui.hpp>

// 0x1000 = 4096
// 0x800 = 2048
#define READ_DELAY 0x1000
#define ERROR_PRINT(code) printError(code)
void PPDecoder::printError(int errorCode)
{
	if (errorCode >= 0) return;
	char log[AV_ERROR_MAX_STRING_SIZE]{ 0 };
	std::cout << "Error: " << av_make_error_string(log, AV_ERROR_MAX_STRING_SIZE, errorCode) << std::endl;
}

PPDecoder::PPDecoder()
{
}

PPDecoder::~PPDecoder()
{
}

#define READ_BUFFER_SIZE 0x800
static u8* readBuffer = nullptr;
static void videoDecodeThreadFunc(void* arg)
{
	PPDecoder* self = reinterpret_cast<PPDecoder*>(arg);
	//-------------------------------------------------------
	int timeDelay = 1000.0f / (float)self->mFrameRate;
	auto timer = new Timer<int, std::milli>(std::chrono::duration<int, std::milli>(1));
	//-------------------------------------------------------
	readBuffer = (u8*)malloc(READ_BUFFER_SIZE + MEMORY_BUFFER_PADDING);
	memset(readBuffer + READ_BUFFER_SIZE, 0, MEMORY_BUFFER_PADDING);
	self->pVideoPacket->size = -1;
	int ret = 0;
	//-------------------------------------------------------
	while (true)
	{
		timer->start();
		
		self->pVideoPacket->size = self->pVideoIOBuffer->read(self->pVideoPacket->data, READ_BUFFER_SIZE);
		while(self->pVideoPacket->size > 0)
		{
			self->decodeVideoStream();
		}

		timer->wait();
	}
}

void PPDecoder::initDecoder()
{
	av_register_all();
	initVideoStream();
}

void PPDecoder::initVideoStream()
{
	//-----------------------------------------------------------------
	// init video encoder
	//-----------------------------------------------------------------
	const AVCodec* videoCodec = avcodec_find_decoder(AV_CODEC_ID_MPEG4);
	pVideoParser = av_parser_init(videoCodec->id);
	pVideoContext = avcodec_alloc_context3(videoCodec);
	// Open
	int ret = avcodec_open2(pVideoContext, videoCodec, NULL);
	pVideoPacket = av_packet_alloc();
	pVideoFrame = av_frame_alloc();
	// Init custom memory buffer
	pVideoIOBuffer = new MemoryBuffer();
	pVideoIOBuffer->pBufferAddr = (u8*)malloc(MEMORY_BUFFER_SIZE + MEMORY_BUFFER_PADDING);
	memset(pVideoIOBuffer->pBufferAddr + MEMORY_BUFFER_SIZE, 0, MEMORY_BUFFER_PADDING);
	pVideoIOBuffer->iSize = MEMORY_BUFFER_SIZE;
	pVideoIOBuffer->iMaxSize = MEMORY_BUFFER_SIZE;
	pVideoIOBuffer->iCursor = 0;
	pVideoIOBuffer->pMutex = new std::mutex();
}

void PPDecoder::initAudioStream()
{
}

static u8* RGB = nullptr;
void PPDecoder::decodeVideoStream()
{
	int ret = 0;
	ret = avcodec_send_packet(pVideoContext, pVideoPacket);
	if (ret < 0) return;
	//while (ret >= 0) {
		ret = avcodec_receive_frame(pVideoContext, pVideoFrame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return;
		else if (ret < 0) exit(0);

		//printf("saving frame %3d\n", pVideoContext->frame_number);
		//fflush(stdout);
		//----------------------------------------------
		// process frame
		//----------------------------------------------
		int w = pVideoFrame->width;
		int h = pVideoFrame->height;

		u8* Y = (u8*)pVideoFrame->data[0];
		u8* U = (u8*)pVideoFrame->data[1];
		u8* V = (u8*)pVideoFrame->data[2];

		if (RGB == nullptr) RGB = (u8*)malloc(3 * w * h);

		yuv420_rgb24_std(w, h, Y, U, V, pVideoFrame->linesize[0], pVideoFrame->linesize[1], RGB, w * 3, YCBCR_JPEG);

		cv::Mat image(h, w, CV_8UC3, RGB);
		cv::cvtColor(image, image, CV_RGB2BGR);
		cv::imshow("video", image);
		cv::waitKey(1);
	//}
}

void PPDecoder::decodeAudioStream()
{
	int ret = 0;
}

void PPDecoder::startDecodeThread()
{
	initDecoder();
	//-------------------------------------------------------
	//std::thread decodeThread = std::thread(videoDecodeThreadFunc, this);
	//decodeThread.detach();
}


void PPDecoder::appendBuffer(u8* buffer, u32 size)
{
	//pVideoIOBuffer->write(buffer, size);
	if (size <= 0) return;
	pVideoPacket->data = buffer;
	pVideoPacket->size = size;
	decodeVideoStream();

}

