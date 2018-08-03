#include "stdafx.h"

#include "ScreenCaptureSession.h"

#include "PPServer.h"
#include "ServerConfig.h"
#include <locale>


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
	m_audioGrabber = new AudioStreamSession();
	m_audioGrabber->StartAudioStream();
	m_audioGrabber->Pause();
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

			g_threadMutex->lock();
			int totalSize = 0;
			if (mLastFrameData == nullptr) {
				mLastFrameData = new FrameData();
				mLastFrameData->Width = Width(img);
				mLastFrameData->Height = Height(img);
				mLastFrameData->StrideWidth = mLastFrameData->Width * img.Pixelstride + img.RowPadding;
				totalSize = mLastFrameData->StrideWidth * mLastFrameData->Height;
				mLastFrameData->DataAddr = (uint8_t*)malloc(totalSize);
			}else
			{
				totalSize = mLastFrameData->StrideWidth * mLastFrameData->Height;
			}
			mLastFrameData->Drew = false;
			memmove(mLastFrameData->DataAddr, img.Data, totalSize);
			g_threadMutex->unlock();
		}
	)->start_capturing();
	int timeDelay = 1000.0f / (float)mFrameRate;
	m_frameGrabber->setFrameChangeInterval(std::chrono::milliseconds(timeDelay));
	m_frameGrabber->pause();
	//-----------------------------------------------------
	// decoder
	mLastFrameData = new FrameData();
	mLastFrameData->Width = monitor.Width;
	mLastFrameData->Height = monitor.Height;
	mLastFrameData->StrideWidth = mLastFrameData->Width * 4 + 40;
	int totalSize = mLastFrameData->StrideWidth * mLastFrameData->Height;
	mLastFrameData->Drew = false;
	mLastFrameData->DataAddr = (uint8_t*)malloc(totalSize);
	initEncoder(mLastFrameData);
	//-----------------------------------------------------
	// start loopback record thread
	g_threadMutex = new std::mutex();
	g_thread = std::thread(onProcessUpdateThread, this);
	g_thread.detach();
}

void ScreenCaptureSession::onProcessUpdateThread(void* context)
{
	ScreenCaptureSession* self = (ScreenCaptureSession*)context;
	int timeDelay = 1000.0f / (float)self->mFrameRate;
	auto timer = new SL::Screen_Capture::Timer<int, std::milli>( std::chrono::duration<int, std::milli>(0));

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
		//TODO: we update server here ?
		self->serverUpdate();

		//======================================================
		timer->start();
		int tsDif = av_compare_ts(self->mVideoStream.NextFrameIdx, self->mVideoStream.CodecContext->time_base, self->mAudioStream.NextFrameIdx, self->mAudioStream.CodecContext->time_base);

		//======================================================
		// encode video frame
		//======================================================
		self->g_threadMutex->lock();
		if(self->mLastFrameData != nullptr)
		{
			self->writeVideoFrame(self->mLastFrameData);

			////TODO: what if it is the first frame ( init something ? )
			//if(self->mIsFirstFrame)
			//{
			//	self->mIsFirstFrame = false;
			//}
			//if (tsDif <= 0 || !self->mLastFrameData->Drew) {
			//	self->mLastFrameData->Drew = true;
			//	self->writeVideoFrame(self->mLastFrameData);
			//}
		}
		self->g_threadMutex->unlock();

		//======================================================
		// encode audio frame
		//======================================================
		//if (tsDif >= 0 ) {
		//	//self->writeAudioFrame();
		//	int audioFrameLeft = self->m_audioGrabber->GetAudioFrames();
		//	while (audioFrameLeft > 1024) {
		//		self->writeAudioFrame();
		//		audioFrameLeft = self->m_audioGrabber->GetAudioFrames();
		//	}
		//}
		timer->wait();


		//======================================================
		// test
		//======================================================
		/*if (self->m_frameIndex - self->mLastSentFrame > (self->mFrameRate * 10))
		{
			avcodec_send_frame(self->mVideoStream.CodecContext, NULL);
			avcodec_send_frame(self->mAudioStream.CodecContext, NULL);
			isStopEncode = true;
			break;
		}*/
	}

	//======================================================
	// stop
	//======================================================
	// av_write_trailer(self->mFormatContext); //NOTE: no need
	// avio_closep(&self->mFormatContext->pb); //NOTE: no need

	self->closeStream(&self->mVideoStream);
	self->closeStream(&self->mAudioStream);
	avformat_free_context(self->mFormatContext);
}

static int64_t bufferReadIndex = 0;
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

	if (m_clientSession->g_ss_isReceived) {
		g_packetMutex->lock();
		if(m_packet != nullptr)
		{
			m_clientSession->PreparePacketAndSend(0x01, m_packet->refPacket);
			// clear data
			av_free_packet(m_packet->refPacket);
			if (m_packet->next != NULL) {
				m_packet->next->prev = NULL;
				delete m_packet;
				m_packet = m_packet->next;
			}else
			{
				delete m_packet;
				m_packet = NULL;
				m_lastPacket = NULL;
			}
		}
		g_packetMutex->unlock();
	}
}

void ScreenCaptureSession::initEncoder(FrameData* frameData)
{
	int ret;
	g_packetMutex = new std::mutex();
	//----------------------------------------------------------------
	// init codecs and streams
	//----------------------------------------------------------------
	if (!mInitializedCodec) {
		// register all codecs
		av_register_all();

		// init streams format
		mVideoStream = { 0 };
		mAudioStream = { 0 };
		avformat_alloc_output_context2(&mFormatContext, NULL, "mp4", NULL);
		if (!mFormatContext)
		{
			std::cout << "Could not allocate the mFormatContext" << std::endl;
			return;
		}
		mOutputFormat = mFormatContext->oformat;
		if (mOutputFormat->video_codec != AV_CODEC_ID_NONE)
		{
			initVideoStream(frameData->Width, frameData->Height);
		}
		if (mOutputFormat->audio_codec != AV_CODEC_ID_NONE)
		{
			initAudioStream();
		}
		// print encode format
		av_dump_format(mFormatContext, 0, "", 1);

		//-------------------------------------------------------
		// save to file | only for write local file to test
		/*int ret = avio_open(&mFormatContext->pb, "test_record.mp4", AVIO_FLAG_WRITE);
		if (ret < 0) {
			return;
		}*/
		//-------------------------------------------------------

		ret = avio_open_dyn_buf(&mFormatContext->pb);

		// is it require to write header ?
		ret = avformat_write_header(mFormatContext, NULL);

		mInitializedCodec = true;
	}
}

void ScreenCaptureSession::initVideoStream(int srcW, int srcH)
{
	mVideoCodec = avcodec_find_encoder(mOutputFormat->video_codec);
	if (!mVideoCodec)
	{
		std::cout << "Codec not support: " << avcodec_get_name(mOutputFormat->video_codec) << std::endl;
		return;
	}
	mVideoStream.Stream = avformat_new_stream(mFormatContext, mVideoCodec);
	if(!mVideoStream.Stream)
	{
		std::cout << "Codec not allocate stream" << std::endl;
		return;
	}
	mVideoStream.Stream->id = mFormatContext->nb_streams - 1;
	mVideoStream.CodecContext = avcodec_alloc_context3(mVideoCodec);
	if (!mVideoStream.CodecContext)
	{
		std::cout << "could not allocate video codec context" << std::endl;
		return;
	}

	//----------------------------------------------------
	// config for context
	mVideoStream.CodecContext->codec_id = mOutputFormat->video_codec;
	mVideoStream.CodecContext->bit_rate = 720000;
	mVideoStream.CodecContext->width = 400;
	mVideoStream.CodecContext->height = 240;
	mVideoStream.CodecContext->time_base = AVRational{ 1, mFrameRate };
	mVideoStream.CodecContext->framerate = AVRational{ mFrameRate, 1 };
	mVideoStream.Stream->time_base = mVideoStream.CodecContext->time_base;
	mVideoStream.CodecContext->gop_size = 10;
	mVideoStream.CodecContext->max_b_frames = 1;
	mVideoStream.CodecContext->pix_fmt = AV_PIX_FMT_YUV420P;

	mVideoStream.SWSContext = sws_getContext(srcW, srcH, AV_PIX_FMT_BGRA,
		mVideoStream.CodecContext->width, mVideoStream.CodecContext->height, AV_PIX_FMT_YUV420P, 0, 0, 0, 0);
	if (!mVideoStream.SWSContext)
	{
		std::cout << "could not create SWSContext" << std::endl;
		return;
	}

	if (mOutputFormat->flags & AVFMT_GLOBALHEADER)
		mVideoStream.CodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


	//----------------------------------------------------
	// Open video
	int ret = avcodec_open2(mVideoStream.CodecContext, mVideoCodec, NULL);
	if (ret < 0)
	{
		std::cout << "could not open codec" << std::endl;
		return;
	}

	//----------------------------------------------------
	// alloc re-useable frame
	mVideoStream.Frame = av_frame_alloc();
	if (!mVideoStream.Frame)
	{
		std::cout << "could not alloc frame" << std::endl;
		return;
	}
	mVideoStream.Frame->format = mVideoStream.CodecContext->pix_fmt;
	mVideoStream.Frame->width = mVideoStream.CodecContext->width;
	mVideoStream.Frame->height = mVideoStream.CodecContext->height;
	ret = av_frame_get_buffer(mVideoStream.Frame, 32); //NOTE: why size is 32 ?
	if (ret < 0)
	{
		std::cout << "Could not allocate the video frame data" << std::endl;
		return;
	}

	// copy the stream parameters to the muxer
	ret = avcodec_parameters_from_context(mVideoStream.Stream->codecpar, mVideoStream.CodecContext);
	if (ret < 0)
	{
		std::cout << "Could not copy stream parameters" << std::endl;
		return;
	}
	mVideoStream.Packet = av_packet_alloc();
	av_init_packet(mVideoStream.Packet);
}

void ScreenCaptureSession::initAudioStream()
{
	mAudioCodec = avcodec_find_encoder(mOutputFormat->audio_codec);
	if (!mVideoCodec)
	{
		std::cout << "Codec not support: " << avcodec_get_name(mOutputFormat->audio_codec) << std::endl;
		return;
	}
	mAudioStream.Stream = avformat_new_stream(mFormatContext, NULL);
	if (!mAudioStream.Stream)
	{
		std::cout << "Codec not allocate audio stream" << std::endl;
		return;
	}
	mAudioStream.Stream->id = mFormatContext->nb_streams - 1;
	mAudioStream.CodecContext = avcodec_alloc_context3(mAudioCodec);
	if (!mAudioStream.CodecContext)
	{
		std::cout << "could not allocate audio codec context" << std::endl;
		return;
	}


	//------------------------------------------------------
	// config audio stream
	mAudioStream.CodecContext->sample_fmt = mAudioCodec->sample_fmts ? mAudioCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP; // NOTE: loopback is use int-16 wave
	mAudioStream.CodecContext->bit_rate = 96000;
	mAudioStream.CodecContext->sample_rate = 48000;
	if(mAudioCodec->supported_samplerates)
	{
		mAudioStream.CodecContext->sample_rate = mAudioCodec->supported_samplerates[0];
		for (int i = 0; mAudioCodec->supported_samplerates[i]; i++) {
			if (mAudioCodec->supported_samplerates[i] == 48000)
				mAudioStream.CodecContext->sample_rate = 48000;
		}
	}
	mAudioStream.CodecContext->channels = av_get_channel_layout_nb_channels(mAudioStream.CodecContext->channel_layout);
	mAudioStream.CodecContext->channel_layout = AV_CH_LAYOUT_STEREO;
	if(mAudioCodec->channel_layouts)
	{
		mAudioStream.CodecContext->channel_layout = mAudioCodec->channel_layouts[0];
		for (int i = 0; mAudioCodec->channel_layouts[i]; i++) {
			if (mAudioCodec->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
				mAudioStream.CodecContext->channel_layout = AV_CH_LAYOUT_STEREO;
		}
	}
	mAudioStream.CodecContext->channels = av_get_channel_layout_nb_channels(mAudioStream.CodecContext->channel_layout);
	mAudioStream.Stream->time_base = AVRational{ 1, mAudioStream.CodecContext->sample_rate };

	if (mOutputFormat->flags & AVFMT_GLOBALHEADER)
		mAudioStream.CodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	//----------------------------------------------------
	// Open audio
	int nb_samples;
	int ret = avcodec_open2(mAudioStream.CodecContext, mAudioCodec, NULL);

	if (mAudioStream.CodecContext->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
		nb_samples = 10000;
	else
		nb_samples = mAudioStream.CodecContext->frame_size;

	mAudioStream.Frame = av_frame_alloc();
	mAudioStream.Frame->format = mAudioStream.CodecContext->sample_fmt;
	mAudioStream.Frame->channel_layout = mAudioStream.CodecContext->channel_layout;
	mAudioStream.Frame->sample_rate = mAudioStream.CodecContext->sample_rate;
	mAudioStream.Frame->nb_samples = nb_samples;
	if (nb_samples) {
		ret = av_frame_get_buffer(mAudioStream.Frame, 0);
		if (ret < 0) {
			std::cout << "Codec not allocate audio buffer" << std::endl;
			return;
		}
	}

	mAudioStream.TmpFrame = av_frame_alloc();
	mAudioStream.TmpFrame->format = AV_SAMPLE_FMT_S16;
	mAudioStream.TmpFrame->channel_layout = mAudioStream.CodecContext->channel_layout;
	mAudioStream.TmpFrame->sample_rate = mAudioStream.CodecContext->sample_rate;
	mAudioStream.TmpFrame->nb_samples = nb_samples;
	if (nb_samples) {
		ret = av_frame_get_buffer(mAudioStream.TmpFrame, 0);
		if (ret < 0) {
			std::cout << "Codec not allocate audio buffer" << std::endl;
			return;
		}
	}

	// copy the stream parameters to the muxer
	ret = avcodec_parameters_from_context(mAudioStream.Stream->codecpar, mAudioStream.CodecContext);
	if (ret < 0)
	{
		std::cout << "Could not copy stream parameters" << std::endl;
		return;
	}

	// create resampler context
	mAudioStream.SWRContext = swr_alloc();
	if (!mAudioStream.SWRContext) {
		std::cout << "Could not allocate resampler context" << std::endl;
		return;
	}

	/* set options */
	av_opt_set_int(mAudioStream.SWRContext, "in_channel_count", mAudioStream.CodecContext->channels, 0);
	av_opt_set_int(mAudioStream.SWRContext, "in_sample_rate", mAudioStream.CodecContext->sample_rate, 0);
	av_opt_set_sample_fmt(mAudioStream.SWRContext, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	av_opt_set_int(mAudioStream.SWRContext, "out_channel_count", mAudioStream.CodecContext->channels, 0);
	av_opt_set_int(mAudioStream.SWRContext, "out_sample_rate", mAudioStream.CodecContext->sample_rate, 0);
	av_opt_set_sample_fmt(mAudioStream.SWRContext, "out_sample_fmt", mAudioStream.CodecContext->sample_fmt, 0);

	if((ret = swr_init(mAudioStream.SWRContext)) < 0)
	{
		std::cout << "Failed to initialize the resampling context" << std::endl;
		return;
	}

	mAudioStream.Packet = av_packet_alloc();
	av_init_packet(mAudioStream.Packet);
}

AVFrame* ScreenCaptureSession::getVideoFrame( FrameData* frameData)
{
	AVCodecContext* c = mVideoStream.CodecContext;

	int ret = av_frame_make_writable(mVideoStream.Frame);
	if (ret < 0)
	{
		std::cout << "Could not make frame writeble" << std::endl;
		return NULL;
	}

	//===============================================================
	// increment for frame index
	if (m_frameIndex > UINT32_MAX - 100) m_frameIndex = 0;	// reset frame counter
	m_frameIndex++;

	const unsigned char *inData[1] = { frameData->DataAddr };
	int      inLinesize[1] = { frameData->StrideWidth };

	sws_scale(mVideoStream.SWSContext, inData, inLinesize, 0, frameData->Height, mVideoStream.Frame->data, mVideoStream.Frame->linesize);
	mVideoStream.Frame->pts = mVideoStream.NextFrameIdx++;
	return mVideoStream.Frame;
}

void ScreenCaptureSession::writeVideoFrame( FrameData* frameData)
{
	AVFrame* frame = getVideoFrame(frameData);
	//===============================================================
	int ret = avcodec_send_frame(mVideoStream.CodecContext, frame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR(EINVAL) || ret == AVERROR_EOF || ret == AVERROR(ENOMEM))
	{
		/*
		* AVERROR(EAGAIN):   input is not accepted in the current state - user
		*					  must read output with avcodec_receive_packet() (once
		*                         all output is read, the packet should be resent, and
		*                         the call will not fail with EAGAIN).
		*/
		std::cout << "Error sending a video frame for encoding: AVERROR(EAGAIN)" << std::endl;
		return;
	}
	else if (ret < 0)
	{
		std::cout << "Error sending a video frame for encoding" << std::endl;
		return;
	}
	// init new packet
	AVPacket* packet = av_packet_alloc();
	av_init_packet(packet);

	ret = avcodec_receive_packet(mVideoStream.CodecContext, packet);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	{
		std::cout << "Something must to be return here: VIDEO" << std::endl;
		av_free_packet(packet);
		return;
	}
	else if (ret < 0)
	{
		std::cout << "Error when encoding video" << std::endl;
		av_free_packet(packet);
		return;
	}
	std::cout << "Write video packet: " << mVideoStream.Packet->pts << " - Size: " << mVideoStream.Packet->size << std::endl << std::flush;

	// old code
	/*av_packet_rescale_ts(mVideoStream.Packet, mVideoStream.CodecContext->time_base, mVideoStream.Stream->time_base);
	mVideoStream.Packet->stream_index = mVideoStream.Stream->index;
	ret = av_interleaved_write_frame(mFormatContext, mVideoStream.Packet);*/


	// store this packet to send to client
	g_packetMutex->lock();
	if (m_packet == nullptr) {
		m_packet = new PacketLinkList();
		m_packet->refPacket = packet;
		m_lastPacket = m_packet;
	}
	else
	{
		auto linkPacket = new PacketLinkList();
		linkPacket->refPacket = packet;
		linkPacket->prev = m_lastPacket;
		m_lastPacket->next = linkPacket;
		m_lastPacket = linkPacket;
	}
	g_packetMutex->unlock();
}

AVFrame* ScreenCaptureSession::getAudioFrame()
{
	m_audioGrabber->BeginReadAudioBuffer();
	u32 size = m_audioGrabber->GetAudioBufferSize();
	int frames = m_audioGrabber->GetAudioFrames();
	if (frames > 0 && frames < mAudioStream.TmpFrame->nb_samples) {
		m_audioGrabber->FinishReadAudioBuffer(0, 0);
		return NULL;
	}
	if(size == 0)
	{
		//std::cout << "audio stream is very silent right now" << std::endl;
		//m_audioGrabber->FinishReadAudioBuffer(0, 0);
		//return NULL;
	}
	if(frames > mAudioStream.TmpFrame->nb_samples || frames == 0)
	{
		frames = mAudioStream.TmpFrame->nb_samples;
	}
	if(frames * 4 < size)
	{
		size = frames * 4;
	}
	av_frame_make_writable(mAudioStream.TmpFrame);
	uint16_t* samples = (uint16_t*)mAudioStream.TmpFrame->data[0];
	memmove(samples, m_audioGrabber->GetAudioBuffer(), size); //NOTE: need to be *2 ?

	mAudioStream.TmpFrame->pts = mAudioStream.NextFrameIdx;
	mAudioStream.NextFrameIdx += frames;

	// convert samples from native format to destination codec using resampler
	AVCodecContext* c = mAudioStream.CodecContext;
	int ret = av_frame_make_writable(mAudioStream.Frame);
	if (ret < 0)
	{
		std::cout << "Could not make frame writeble" << std::endl;
		m_audioGrabber->FinishReadAudioBuffer(0, 0);
		return NULL;
	}
	int64_t delay = swr_get_delay(mAudioStream.SWRContext, c->sample_rate);
	int dst_nb_samples = av_rescale_rnd(delay + frames, c->sample_rate, c->sample_rate, AV_ROUND_UP);
	ret = swr_convert(mAudioStream.SWRContext, mAudioStream.Frame->data, dst_nb_samples, (const uint8_t**)mAudioStream.TmpFrame->data, frames);

	//mAudioStream.TmpFrame = mAudioStream.Frame;
	mAudioStream.TmpFrame->pts = av_rescale_q(mAudioStream.SamplesCount, AVRational{ 1, c->sample_rate }, c->time_base);
	mAudioStream.Frame->pts = mAudioStream.TmpFrame->pts;
	mAudioStream.SamplesCount += dst_nb_samples;

	int framesLeft = m_audioGrabber->FinishReadAudioBuffer(size, frames);
	//std::cout << "Delay : " << size << std::endl;
	return mAudioStream.Frame;
}

void ScreenCaptureSession::writeAudioFrame()
{
	AVFrame* frame = getAudioFrame();
	if (!frame) return;
	//===============================================================
	int ret = avcodec_send_frame(mAudioStream.CodecContext, frame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR(EINVAL) || ret == AVERROR_EOF || ret == AVERROR(ENOMEM))
	{
		/*
		* AVERROR(EAGAIN):   input is not accepted in the current state - user
		*					  must read output with avcodec_receive_packet() (once
		*                         all output is read, the packet should be resent, and
		*                         the call will not fail with EAGAIN).
		*/
		std::cout << "Error sending an audio frame for encoding: AVERROR(EAGAIN)" << std::endl;
		return;
	}
	else if (ret < 0)
	{
		std::cout << "Error sending an audio frame for encoding" << std::endl;
		return;
	}

	ret = avcodec_receive_packet(mAudioStream.CodecContext, mAudioStream.Packet);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	{
		return;
	}
	else if (ret < 0)
	{
		std::cout << "Error when encoding audio" << std::endl;
		return;
	}


	std::cout << "Write audio packet: " << mAudioStream.Packet->pts << " - Size: " << mAudioStream.Packet->size << std::endl << std::flush;


	av_packet_rescale_ts(mAudioStream.Packet, mAudioStream.CodecContext->time_base, mAudioStream.Stream->time_base);
	mAudioStream.Packet->stream_index = mAudioStream.Stream->index;
	ret = av_interleaved_write_frame(mFormatContext, mAudioStream.Packet);
	av_packet_unref(mAudioStream.Packet);
}


void ScreenCaptureSession::closeStream(OutputStream* stream)
{
	avcodec_free_context(&stream->CodecContext);
	av_frame_free(&stream->Frame);
	av_frame_free(&stream->TmpFrame);
	sws_freeContext(stream->SWSContext);
	swr_free(&stream->SWRContext);
}

void ScreenCaptureSession::startStream()
{
	m_isStartStreaming = true;
	m_frameIndex = 0;
	m_frameGrabber->resume();
	m_audioGrabber->Resume();
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
