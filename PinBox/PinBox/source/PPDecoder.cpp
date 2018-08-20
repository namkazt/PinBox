#include "PPDecoder.h"
#include "PPAudio.h"

static u8* pRGBBuffer = nullptr;

volatile bool initialized = false;


PPDecoder::PPDecoder()
{
}

PPDecoder::~PPDecoder()
{
	
}

void PPDecoder::initDecoder()
{
	if (initialized) return;
	initialized = true;

	av_log_set_level(AV_LOG_QUIET);
	av_register_all();
	//-----------------------------------------------------------------
	// init video encoder
	//-----------------------------------------------------------------
	const AVCodec* videoCodec = avcodec_find_decoder(AV_CODEC_ID_MPEG4);
	pVideoContext = avcodec_alloc_context3(videoCodec);
	pVideoContext->width = 400;
	pVideoContext->height = 240;
	pVideoContext->pix_fmt = AV_PIX_FMT_YUV420P;
	pVideoContext->thread_count = 4;
	pVideoContext->thread_type = FF_THREAD_FRAME;
	// Open
	int ret = avcodec_open2(pVideoContext, videoCodec, NULL);
	if (ret < 0)
	{
		printf("failed to open Video decoder: %d\n", ret);
	}
	pVideoPacket = av_packet_alloc();
	pVideoFrame = av_frame_alloc();
	initY2RImageConverter();

	//-----------------------------------------------------------------
	// init audio encoder
	//-----------------------------------------------------------------
	const AVCodec *audioCodec = avcodec_find_decoder(AV_CODEC_ID_MP2);
	if(!audioCodec)
	{
		printf("[Audio] Codec not found!\n");
	}
	pAudioContext = avcodec_alloc_context3(audioCodec);
	pAudioContext->bit_rate = 64000;
	pAudioContext->sample_fmt = AV_SAMPLE_FMT_S16;
	pAudioContext->request_sample_fmt = AV_SAMPLE_FMT_S16;
	pAudioContext->request_channel_layout = AV_CH_LAYOUT_STEREO;
	pAudioContext->sample_rate = 22050;
	pAudioContext->channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);;
	pAudioContext->channel_layout = AV_CH_LAYOUT_STEREO;
	ret = avcodec_open2(pAudioContext, audioCodec, NULL);
	if(ret < 0)
	{
		printf("[Audio] Failed to open context: %d\n", ret);
	}
	pAudioPacket = av_packet_alloc();
	pAudioFrame = av_frame_alloc();
}

void PPDecoder::releaseDecoder()
{
	if (!initialized) return;
	initialized = false;
	printf("Release audio/video decoder.\n");
	// free video
	avcodec_free_context(&pVideoContext);
	av_frame_free(&pVideoFrame);
	av_packet_free(&pVideoPacket);
	// free audio
	avcodec_free_context(&pAudioContext);
	av_frame_free(&pAudioFrame);
	av_packet_free(&pAudioPacket);
	//---------------------------------------------------
	bool is_busy = 0;
	Y2RU_StopConversion();
	Y2RU_IsBusyConversion(&is_busy);
	y2rExit();

	linearFree(pRGBBuffer);
}

u8* PPDecoder::appendVideoBuffer(u8* buffer, u32 size)
{
	if (!initialized) return nullptr;
	if (size <= 0) return nullptr;
	pVideoPacket->data = buffer;
	pVideoPacket->size = size;
	return decodeVideoStream();
}

void PPDecoder::initY2RImageConverter()
{
	//--------------------------------------------------------------------------------------
	// NOTE: code borrow from 3DS video player
	// ref: https://github.com/Lectem/3Damnesic/blob/master/source/color_converter.c
	//--------------------------------------------------------------------------------------
	pDecodeState = new DecodeState();
	Result res = 0;
	res = y2rInit();
	if (res != 0) printf("Error when init Y2R\n");
	res = Y2RU_StopConversion();
	if (res != 0) printf("Error on Y2RU_StopConversion\n");
	bool is_busy = 0;
	int tries = 0;
	do
	{
		svcSleepThread(25 * 1000ull);
		res = Y2RU_StopConversion();
		if (res != 0) printf("Error on Y2RU_StopConversion\n");
		res = Y2RU_IsBusyConversion(&is_busy);
		if (res != 0) printf("Error on Y2RU_IsBusyConversion\n");
		tries += 1;
	} while (is_busy && tries < 100);
	pDecodeState->y2rParams.input_format = INPUT_YUV420_INDIV_8;
	pDecodeState->y2rParams.output_format = OUTPUT_RGB_24;
	pDecodeState->y2rParams.rotation = ROTATION_NONE;
	pDecodeState->y2rParams.block_alignment = BLOCK_8_BY_8;
	pDecodeState->y2rParams.input_line_width = 400;
	pDecodeState->y2rParams.input_lines = 240;
	if (pDecodeState->y2rParams.input_lines % 8) {
		pDecodeState->y2rParams.input_lines += 8 - (pDecodeState->y2rParams.input_lines % 8);
	}
	pDecodeState->y2rParams.standard_coefficient = COEFFICIENT_ITU_R_BT_601;
	pDecodeState->y2rParams.unused = 0;
	pDecodeState->y2rParams.alpha = 0xFF;
	res = Y2RU_SetConversionParams(&pDecodeState->y2rParams);
	if (res != 0) printf("Error on Y2RU_SetConversionParams\n");
	res = Y2RU_SetTransferEndInterrupt(true);
	if (res != 0) printf("Error on Y2RU_SetTransferEndInterrupt\n");
	pDecodeState->endEvent = 0;
	res = Y2RU_GetTransferEndEvent(&pDecodeState->endEvent);
	if (res != 0) printf("Error on Y2RU_GetTransferEndEvent\n");
}

void PPDecoder::convertColor()
{
	Result res;
	const s16 img_w = pDecodeState->y2rParams.input_line_width;
	const s16 img_h = pDecodeState->y2rParams.input_lines;
	const u32 img_size = img_w * img_h;
	const u32 img_w_UV = img_w >> 1;
	size_t src_Y_size = 0;
	size_t src_UV_size = 0;

	switch (pDecodeState->y2rParams.input_format)
	{
	case INPUT_YUV422_INDIV_8:
		src_Y_size = img_size;
		src_UV_size = img_size / 2;
		break;
	case INPUT_YUV420_INDIV_8:
		src_Y_size = img_size;
		src_UV_size = img_size / 4;
		break;
	case INPUT_YUV422_INDIV_16:
		src_Y_size = img_size * 2;
		src_UV_size = img_size / 2 * 2;
		break;
	case INPUT_YUV420_INDIV_16:
		src_Y_size = img_size * 2;
		src_UV_size = img_size / 4 * 2;
		break;
	case INPUT_YUV422_BATCH:
		src_Y_size = img_size * 2;
		src_UV_size = img_size * 2;
		break;
	}

	u8 *src_Y = (u8*)pVideoFrame->data[0];
	u8 *src_U = (u8*)pVideoFrame->data[1];
	u8 *src_V = (u8*)pVideoFrame->data[2];
	const s16 src_Y_padding = pVideoFrame->linesize[0] - img_w;
	const s16 src_UV_padding = pVideoFrame->linesize[1] - img_w_UV;

	Y2RU_StopConversion();

	res = Y2RU_SetSendingY(src_Y, src_Y_size, img_w, src_Y_padding);
	if (res != 0) 
		printf("Error on Y2RU_SetSendingY\n");
	res = Y2RU_SetSendingU(src_U, src_UV_size, img_w_UV, src_UV_padding);
	if (res != 0) 
		printf("Error on Y2RU_SetSendingU\n");
	res = Y2RU_SetSendingV(src_V, src_UV_size, img_w_UV, src_UV_padding);
	if (res != 0) 
		printf("Error on Y2RU_SetSendingV\n");

	const u16 pixSize = 3;
	size_t rgb_size = (512*256) * pixSize;
	s16 transfer_unit = 8;
	s16 gap = (512 - img_w) * transfer_unit * pixSize;
	//TODO: try this with get frame buffer
	res = Y2RU_SetReceiving(pRGBBuffer, rgb_size, img_w * transfer_unit * pixSize, gap);
	if (res != 0) 
		printf("Error on Y2RU_SetReceiving\n");
	res = Y2RU_StartConversion();
	if (res != 0) 
		printf("Error on Y2RU_StartConversion\n");
	res = svcWaitSynchronization(pDecodeState->endEvent, 1000000000ull);
	if (res != 0) 
		printf("Error on svcWaitSynchronization\n");
}

u8* PPDecoder::decodeVideoStream()
{
	int ret = 0;
	ret = avcodec_send_packet(pVideoContext, pVideoPacket);
	if (ret < 0) return nullptr;
	ret = avcodec_receive_frame(pVideoContext, pVideoFrame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return nullptr;
	if (ret < 0) return nullptr;

	//----------------------------------------------
	// decode video frame
	//----------------------------------------------
	iFrameWidth = pVideoFrame->width;
	iFrameHeight =pVideoFrame->height;
	if (pRGBBuffer == nullptr) pRGBBuffer = (u8*)linearMemAlign(3 * 512 * 256, 0x80);
	convertColor();

	av_packet_unref(pVideoPacket);

	return pRGBBuffer;
}

void PPDecoder::decodeAudioStream(u8* buffer, u32 size)
{

	if (!initialized) return;
	if (size == 0) return;

	int ret = 0;
	av_packet_unref(pVideoPacket);
	pAudioPacket->data = buffer;
	pAudioPacket->size = size;

	ret = avcodec_send_packet(pAudioContext, pAudioPacket);
	if (ret < 0) {
		printf("[Audio] Error submitting the packet to the decoder: %d\n", ret);
	}
	else {
		while (ret >= 0) {
			ret = avcodec_receive_frame(pAudioContext, pAudioFrame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return;
			if (ret < 0)
			{
				printf("[Audio] get frame failed: %d\n", ret);
				return;
			}
			PPAudio::Get()->FillBuffer(pAudioFrame->extended_data[0], pAudioFrame->linesize[0]);
		}
	}
	
}
