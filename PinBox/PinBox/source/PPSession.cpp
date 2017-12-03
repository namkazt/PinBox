#include "PPSession.h"
#include "PPGraphics.h"


PPSession::~PPSession()
{
	if (g_network != nullptr) delete g_network;
}


void PPSession::initSession()
{
	if (g_network != nullptr) return;
	g_network = new PPNetwork();
	g_pendingFrame = std::queue<QueueFrame*>();
	printf("Init new session.\n");
	//------------------------------------------------------
	// callback when request data is received
	g_network->SetOnReceivedRequest([=](u8* buffer, u32 size, u32 tag) {
		//------------------------------------------------------
		// verify authentication
		//------------------------------------------------------
		if (!g_authenticated)
		{
			if (tag == PPREQUEST_AUTHEN)
			{
				// check for authentication
				PPMessage *authenMsg = new PPMessage();
				if (authenMsg->ParseHeader(buffer))
				{
					if (authenMsg->GetMessageCode() == MSG_CODE_RESULT_AUTHENTICATION_SUCCESS)
					{
						printf("Authentication sucessfully.\n");
						g_authenticated = true;
					}
					else
					{
						printf("Authentication failed.\n");
						return;
					}
				}
				else
				{
					printf("Authentication failed.\n");
					return;
				}
				delete authenMsg;
			}
			else
			{
				printf("Client was not authentication.\n");
				return;
			}
		}
		//------------------------------------------------------
		// process data by tag
		switch (tag)
		{
		case PPREQUEST_HEADER:
		{
			if (!g_tmpMessage) g_tmpMessage = new PPMessage();
			if (g_tmpMessage->ParseHeader(buffer))
			{
				//----------------------------------------------------
				// request body part of this message
				g_network->SetRequestData(g_tmpMessage->GetContentSize(), PPREQUEST_BODY);
			}
			else
			{
				delete g_tmpMessage;
				g_tmpMessage = nullptr;
				printf("Parse message header fail. remove message.\n");
			}
			break;
		}
		case PPREQUEST_BODY:
		{
			//------------------------------------------------------
			// if tmp message is null that mean this is useless data then we avoid it
			if (!g_tmpMessage) return;
			// verify buffer size with message estimate size
			if (size == g_tmpMessage->GetContentSize())
			{
				// process message
				switch (g_sessionType)
				{
				case PPSESSION_MOVIE:
				{
					processMovieSession(buffer, size);
					break;
				}
				case PPSESSION_SCREEN_CAPTURE:
				{
					processScreenCaptureSession(buffer, size);
					break;
				}
				case PPSESSION_INPUT_CAPTURE:
				{
					processInputSession(buffer, size);
					break;
				}
				}

				//----------------------------------------------------------
				// Request for next message
				g_network->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
			}
			//------------------------------------------------------
			// remove message after use
			delete g_tmpMessage;
			g_tmpMessage = nullptr;
			break;
		}
		default: break;
		}
		//------------------------------------------------------
		// free buffer after use
		linearFree(buffer);
	});
}


void PPSession::InitMovieSession()
{
	initSession();
	//--------------------------------------
	// init specific for movie session
	g_sessionType = PPSESSION_MOVIE;
	printf("Init movie session.\n");
}

void PPSession::InitScreenCaptureSession()
{
	initSession();
	//--------------------------------------
	// init specific for movie session
	g_sessionType = PPSESSION_SCREEN_CAPTURE;
	printf("Init screen capture session.\n");
}

void PPSession::InitInputCaptureSession()
{
	initSession();
	//--------------------------------------
	// init specific for movie session
	g_sessionType = PPSESSION_INPUT_CAPTURE;
	printf("Init input session.\n");
}


void PPSession::StartSession(const char* ip, const char* port)
{
	if (g_network == nullptr) return;
	g_mutexFrame = new Mutex();
	g_network->SetOnConnectionSuccessed([=](u8* data, u32 size)
	{
		printf("Note: this called is not on main thread.\n");
		//NOTE: this not called on main thread !
		//--------------------------------------------------
		int8_t code = 0;
		if (g_sessionType == PPSESSION_MOVIE) code = MSG_CODE_REQUEST_AUTHENTICATION_MOVIE;
		else if (g_sessionType == PPSESSION_SCREEN_CAPTURE) code = MSG_CODE_REQUEST_AUTHENTICATION_SCREEN_CAPTURE;
		else if (g_sessionType == PPSESSION_INPUT_CAPTURE) code = MSG_CODE_REQUEST_AUTHENTICATION_INPUT;
		if (code == 0)
		{
			printf("Invalid session type.\n");
			return;
		}
		//--------------------------------------------------
		// screen capture session authen
		PPMessage *authenMsg = new PPMessage();
		authenMsg->BuildMessageHeader(code);
		u8* msgBuffer = authenMsg->BuildMessageEmpty();
		//--------------------------------------------------
		// send authentication message
		g_network->SendMessageData(msgBuffer, authenMsg->GetMessageSize());
		//--------------------------------------------------
		// set request to get result message
		g_network->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_AUTHEN);
		delete authenMsg;
	});
	g_network->SetOnConnectionClosed([=](u8* data, u32 size)
	{
		//NOTE: this not called on main thread !
		printf("Connection was interupted.\n");
	});
	g_network->Start(ip, port);
}

void PPSession::CloseSession()
{
	delete g_mutexFrame;
	if (g_network == nullptr) return;
	g_network->Stop();
	g_authenticated = false;
	SS_Reset();
}


void PPSession::processMovieSession(u8* buffer, size_t size)
{
	//NOTE: this not called on main thread !
	//------------------------------------------------------
	// process message data	by message type
	//------------------------------------------------------
	switch (g_tmpMessage->GetMessageCode())
	{
	default: break;
	}
}

void PPSession::processScreenCaptureSession(u8* buffer, size_t size)
{
	//NOTE: this not called on main thread !
	//------------------------------------------------------
	// process message data	by message type
	//------------------------------------------------------
	switch (g_tmpMessage->GetMessageCode())
	{
	case MSG_CODE_REQUEST_NEW_SCREEN_FRAME:
	{
		try {
			// If using waiting for new frame then we need:
			//TODO: send request that client received frame and want new one
			if (SS_setting_waitToReceivedFrame)
			{
				//--------------------------------------------------
				// send request that client received frame
				PPMessage *msgObj = new PPMessage();
				msgObj->BuildMessageHeader(MSG_CODE_REQUEST_SCREEN_RECEIVED_FRAME);
				u8* msgBuffer = msgObj->BuildMessageEmpty();
				//--------------------------------------------------
				// send authentication message
				g_network->SendMessageData(msgBuffer, msgObj->GetMessageSize());
				delete msgObj;
			}

			//-------------------------------------------
			// init webp decode config
			u32 nW = 512, nH = 256;
			WebPDecoderConfig config;
			WebPInitDecoderConfig(&config);
			config.options.no_fancy_upsampling = 1;
			config.options.use_scaling = 1;
			config.options.scaled_width = nW;
			config.options.scaled_height = nH;
			if (!WebPGetFeatures(buffer, size, &config.input) == VP8_STATUS_OK)
				return;
			config.output.colorspace = MODE_BGR;
			if (!WebPDecode(buffer, size, &config) == VP8_STATUS_OK)
				return;
			//-------------------------------------------
			g_mutexFrame->Lock();
			//-------------------------------------------
			// mutex lock
			QueueFrame *qF = new QueueFrame();
			qF->size = nW * nH * 3;
			qF->start = (u8*)linearAlloc(qF->size);
			qF->width = nW;
			qF->height = nH;
			memcpy(qF->start, config.output.private_memory, qF->size);
			g_pendingFrame.push(qF);
			//-------------------------------------------
			g_mutexFrame->Unlock();
			//-------------------------------------------
			WebPFreeDecBuffer(&config.output);
		}
		catch (...)
		{
			printf("Error when process frame.\n");
		}
		break;
	}

	//======================================================================
	// AUDIO DECODE
	//======================================================================
	/*
	case MSG_CODE_REQUEST_NEW_AUDIO_FRAME:
	{
		int error;
		//check if opus file is init or not
		if (SS_opusFile == nullptr) {
			// check if it is opus stream
			OggOpusFile* opusTest = op_test_memory(buffer, size, &error);
			op_free(opusTest);
			if (error != 0) {
				printf("Stream is not OPUS format.\n");
				return;
			}
			// create new opus file from first block of memory
			SS_opusFile = op_open_memory(buffer, size, &error);

			// init audio
			//TODO: this should be call on outside
			ndspInit();

			// reset audio channel when start stream
			ndspChnReset(AUDIO_CHANNEL);
			ndspChnWaveBufClear(AUDIO_CHANNEL);
			ndspSetOutputMode(NDSP_OUTPUT_STEREO);
			ndspChnSetInterp(AUDIO_CHANNEL, NDSP_INTERP_LINEAR);
			ndspChnSetRate(AUDIO_CHANNEL, 48000);
			ndspChnSetFormat(AUDIO_CHANNEL, NDSP_FORMAT_STEREO_PCM16);

			float mix[12];
			memset(mix, 0, sizeof(mix));
			mix[0] = 1.0;
			mix[1] = 1.0;
			ndspChnSetMix(0, mix);


			ndspWaveBuf waveBuf[2];
			memset(waveBuf, 0, sizeof(waveBuf));
			waveBuf[0].data_vaddr = &audioBuffer[0];
			waveBuf[0].nsamples = SAMPLESPERBUF;
			waveBuf[1].data_vaddr = &audioBuffer[SAMPLESPERBUF];
			waveBuf[1].nsamples = SAMPLESPERBUF;

			ndspChnWaveBufAdd(AUDIO_CHANNEL, &waveBuf[0]);
			ndspChnWaveBufAdd(AUDIO_CHANNEL, &waveBuf[1]);
		}
		else
		{
			int16_t* bufferOut;
			u32 samplesToRead = 32 * 1024; // 32Kb buffer

										   // read more buffer
			u32 bufferToRead = 120 * 48 * 2;
			int samplesRead = op_read_stereo(SS_opusFile, bufferOut, samplesToRead > bufferToRead ? bufferToRead : samplesToRead);

			if (samplesRead == 0)
			{
				//finish read this
			}
			else if (samplesRead < 0)
			{
				// error ?
				return;
			}
		}

		break;
	}
	*/
	default: break;
	}
}


void PPSession::processInputSession(u8* buffer, size_t size)
{
	//NOTE: this not called on main thread !
	//------------------------------------------------------
	// process message data	by message type
	//------------------------------------------------------
	switch (g_tmpMessage->GetMessageCode())
	{
	default: break;
	}
}


//-----------------------------------------------------
// screen capture
//-----------------------------------------------------
void PPSession::SS_StartStream()
{
	PPMessage *authenMsg = new PPMessage();
	authenMsg->BuildMessageHeader(MSG_CODE_REQUEST_START_SCREEN_CAPTURE);
	u8* msgBuffer = authenMsg->BuildMessageEmpty();
	g_network->SendMessageData(msgBuffer, authenMsg->GetMessageSize());
	g_network->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
	SS_v_isStartStreaming = true;
	delete authenMsg;
}

void PPSession::SS_StopStream()
{
	//--------------------------------------
	// clear pending frame
	g_mutexFrame->Lock();
	while (g_pendingFrame.size() > 0)
	{
		QueueFrame* frame = g_pendingFrame.front();
		g_pendingFrame.pop();
		linearFree(frame->start);
		delete frame;
	}
	std::queue<QueueFrame*>().swap(g_pendingFrame);
	g_mutexFrame->Unlock();
	//--------------------------------------
	PPMessage *authenMsg = new PPMessage();
	authenMsg->BuildMessageHeader(MSG_CODE_REQUEST_STOP_SCREEN_CAPTURE);
	u8* msgBuffer = authenMsg->BuildMessageEmpty();
	g_network->SendMessageData(msgBuffer, authenMsg->GetMessageSize());
	g_network->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
	SS_v_isStartStreaming = false;
	delete authenMsg;
}

void PPSession::SS_ChangeSetting()
{
	PPMessage *authenMsg = new PPMessage();
	authenMsg->BuildMessageHeader(MSG_CODE_REQUEST_CHANGE_SETTING_SCREEN_CAPTURE);

	//-----------------------------------------------
	// alloc msg content block
	size_t contentSize = 13;
	u8* contentBuffer = (u8*)linearAlloc(sizeof(u8) * contentSize);
	u8* pointer = contentBuffer;
	//----------------------------------------------
	// setting: wait for received frame
	u8 _setting_waitToReceivedFrame = SS_setting_waitToReceivedFrame ? 1 : 0;
	WRITE_U8(pointer, _setting_waitToReceivedFrame);
	// setting: smooth frame number ( only activate if waitForReceivedFrame = true)
	WRITE_U32(pointer, SS_setting_smoothStepFrames);
	// setting: frame quality [0 ... 100]
	WRITE_U32(pointer, SS_setting_sourceQuality);
	// setting: frame scale [0 ... 100]
	WRITE_U32(pointer, SS_setting_sourceScale);
	//-----------------------------------------------
	// build message
	u8* msgBuffer = authenMsg->BuildMessage(contentBuffer, contentSize);
	g_network->SendMessageData(msgBuffer, authenMsg->GetMessageSize());
	g_network->SetRequestData(MSG_COMMAND_SIZE, PPREQUEST_HEADER);
	linearFree(contentBuffer);
	delete authenMsg;
}

void PPSession::SS_Reset()
{
	SS_v_isStartStreaming = false;
}

QueueFrame*	PPSession::SS_PopPendingQueue(){

	if (g_pendingFrame.size() == 0) return nullptr;
	g_mutexFrame->Lock();
	QueueFrame* frame = (QueueFrame*)g_pendingFrame.front();
	g_pendingFrame.pop();
	g_mutexFrame->Unlock();
	return frame;
}