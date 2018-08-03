#include "PPDecoder.h"

static u8* pRGBBuffer = nullptr;

PPDecoder::PPDecoder()
{
}

PPDecoder::~PPDecoder()
{
}

void PPDecoder::initDecoder()
{
	av_register_all();

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
	// decode state
	
	initY2RImageConverter();
}

void PPDecoder::releaseDecoder()
{
	av_parser_close(pVideoParser);
	avcodec_free_context(&pVideoContext);
	av_frame_free(&pVideoFrame);
	av_packet_free(&pVideoPacket);
	//---------------------------------------------------
	bool is_busy = 0;
	Y2RU_StopConversion();
	Y2RU_IsBusyConversion(&is_busy);
	y2rExit();
}

u8* PPDecoder::appendVideoBuffer(u8* buffer, u32 size)
{
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
		svcSleepThread(100000ull);
		res = Y2RU_StopConversion();
		if (res != 0) printf("Error on Y2RU_StopConversion\n");
		res = Y2RU_IsBusyConversion(&is_busy);
		if (res != 0) printf("Error on Y2RU_IsBusyConversion\n");
		tries += 1;
	} while (is_busy && tries < 100);
	pDecodeState->y2rParams.input_format = INPUT_YUV420_INDIV_8;
	pDecodeState->y2rParams.output_format = OUTPUT_RGB_24;
	pDecodeState->y2rParams.rotation = ROTATION_NONE;
	pDecodeState->y2rParams.block_alignment = BLOCK_LINE;
	//pDecodeState->y2rParams.block_alignment = BLOCK_8_BY_8;
	pDecodeState->y2rParams.input_line_width = 400;
	pDecodeState->y2rParams.input_lines = 240;
	if (pDecodeState->y2rParams.input_lines % 8) {
		pDecodeState->y2rParams.input_lines += 8 - (pDecodeState->y2rParams.input_lines % 8);
	}
	pDecodeState->y2rParams.standard_coefficient = COEFFICIENT_ITU_R_BT_709;
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
	size_t rgb_size = img_size * pixSize;
	s16 gap = (iFrameWidth - img_w) * 8 * pixSize;

	res = Y2RU_SetReceiving(pRGBBuffer, rgb_size, img_w * 8 * pixSize, gap);
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
	else if (ret < 0) return nullptr;

	//----------------------------------------------
	// init process
	//----------------------------------------------
	iFrameWidth = pVideoFrame->width;
	iFrameHeight =pVideoFrame->height;
	if (pRGBBuffer == nullptr) pRGBBuffer = (u8*)linearMemAlign(3 * iFrameWidth * iFrameHeight, 0x80);

	//TODO: this part have problem ...
	convertColor();
	//----------------------------------------------
	// convert frame
	//----------------------------------------------
	/*u8* Y = (u8*)pVideoFrame->data[0];
	u8* U = (u8*)pVideoFrame->data[1];
	u8* V = (u8*)pVideoFrame->data[2];
	yuv420_rgb24_std(iFrameWidth, iFrameHeight, Y, U, V, pVideoFrame->linesize[0], pVideoFrame->linesize[1], pRGBBuffer, iFrameWidth * 3, YCBCR_JPEG);
*/

	return pRGBBuffer;
}

