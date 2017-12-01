#include "stdafx.h"
#include "PPClientSession.h"
#include "PPMessage.h"


void PPClientSession::InitSession(evpp::TCPConnPtr conn)
{
	g_connection = conn;

	g_continuousBuffer = (u8*)malloc(DEFAULT_CONTINUOUS_BUFFER_SIZE * sizeof(u8));
	g_bufferSize = DEFAULT_CONTINUOUS_BUFFER_SIZE;
	g_bufferWriteIndex = 0;
}

void PPClientSession::DisconnectFromServer()
{
	if (g_sessionType == PPSESSION_MOVIE)
	{

	}
	else if (g_sessionType == PPSESSION_SCREEN_CAPTURE)
	{
		if(g_ss_isStartStreaming)
		{
			g_ss_isStartStreaming = false;
			ss_stopFrameGraber();
		}
		g_ss_currentWaitedFrame = 0;
		g_ss_isReceivedLastFrame = true;
	}
	else if (g_sessionType == PPSESSION_INPUT_CAPTURE)
	{

	}
}

void PPClientSession::ProcessMessage(evpp::Buffer* msg)
{
	//--------------------------------------------------
	// if have new data
	if (msg) {
		u8* msgData = (u8*)msg->data();
		size_t msgSize = msg->size();
		// check with current old data
		if (msgSize + g_bufferWriteIndex > g_bufferSize)
		{
			g_bufferSize += DEFAULT_CONTINUOUS_BUFFER_STEP;
			u8* g_continuousBufferNew = (u8*)realloc((void*)g_continuousBuffer, g_bufferSize * sizeof(u8));
			if (!g_continuousBufferNew)
			{
				// fail to realloc memory
				//TODO: in this cause we need process message first and remove finished part
				// if can't be enough then app crash -> not enough memory
				g_bufferSize -= DEFAULT_CONTINUOUS_BUFFER_STEP;
				return;
			}
			else
			{
				// point old pointer to new point after reallocated
				g_continuousBuffer = g_continuousBufferNew;
			}
		}
		//------------------------------------------------
		// append into old data
		memcpy(g_continuousBuffer + g_bufferWriteIndex, msgData, msgSize);
		g_bufferWriteIndex += msgSize;
	}
	//------------------------------------------------
	// process message data
	if (this->IsAuthenticated())
	{
		// not enough data for header so we just leave it
		if (g_bufferWriteIndex < 9 && g_currentReadState == PPREQUEST_HEADER) return;
		// start process data
		this->ProcessIncommingMessage();
	}
	else
	{
		this->ProcessAuthentication();
		//--------------------------------------------
		// remove 9 bytes of authentication message
		CHOP_N(g_continuousBuffer, 9, g_bufferSize);
		g_bufferWriteIndex -= 9;
	}

	//------------------------------------------------
	// check if still need process message or not
	if (g_bufferWriteIndex >= g_waitForSize) ProcessMessage(nullptr);
}



void PPClientSession::ProcessIncommingMessage()
{
	//----------------------------------------------------
	if(g_currentReadState == PPREQUEST_HEADER)
	{
		//----------------------------------------------------
		// parse for header part
		PPMessage* autheMsg = new PPMessage();
		if (!autheMsg->ParseHeader(g_continuousBuffer))
		{
			std::cout << "[Parse Message Error] Vaidation code incorrect - are you sure current is header part ?" << std::endl;
			return;
		}
		//----------------------------------------------------
		// switch to read body part
		g_currentReadState = PPREQUEST_BODY;
		g_currentMsgCode = autheMsg->GetMessageCode();
		g_waitForSize = autheMsg->GetContentSize();
		//----------------------------------------------------
		// pre process header code if need
		preprocessMessageCode(g_currentMsgCode);
		//----------------------------------------------------
		// it content size of message is zero so we start 
		// wait for new message
		if (g_waitForSize <= 0) {
			g_currentReadState = PPREQUEST_HEADER;
			g_currentMsgCode = -1;
		}
		//--------------------------------------------
		// remove 9 bytes of header message
		CHOP_N(g_continuousBuffer, 9, g_bufferSize);
		g_bufferWriteIndex -= 9;
	}
	else if(g_currentReadState == PPREQUEST_BODY)
	{
		if(g_bufferWriteIndex >= g_waitForSize)
		{
			u8* bodyBuffer = (u8*)malloc(g_waitForSize);
			memcpy(bodyBuffer, g_continuousBuffer, g_waitForSize);
			//----------------------------------------------------
			// process body buffer
			processMessageBody(bodyBuffer, g_currentMsgCode);
			// free buffer data
			free(bodyBuffer);
			//--------------------------------------------
			// remove body data
			CHOP_N(g_continuousBuffer, g_waitForSize, g_bufferSize);
			g_bufferWriteIndex -= g_waitForSize;
			//----------------------------------------------------
			// switch to read next header part
			g_currentReadState = PPREQUEST_HEADER;
			g_currentMsgCode = -1;
			g_waitForSize = 0;
		}
	}
	
}

void PPClientSession::ProcessAuthentication()
{
	//----------------------------------------------------
	// parse for header part
	PPMessage* autheMsg = new PPMessage();
	if(!autheMsg->ParseHeader(g_continuousBuffer))
	{
		std::cout << "[Authentication Error] Vaidation code incorrect " << std::endl;
		sendMessageWithCode(MSG_CODE_RESULT_AUTHENTICATION_FAILED);
		return;
	}
	//----------------------------------------------------
	// validate authentication
	if(autheMsg->GetMessageCode() == MSG_CODE_REQUEST_AUTHENTICATION_MOVIE)
	{
		g_sessionType = PPSESSION_MOVIE;
	}else if(autheMsg->GetMessageCode() == MSG_CODE_REQUEST_AUTHENTICATION_SCREEN_CAPTURE)
	{
		g_sessionType = PPSESSION_SCREEN_CAPTURE;
	}
	else if (autheMsg->GetMessageCode() == MSG_CODE_REQUEST_AUTHENTICATION_INPUT)
	{
		g_sessionType = PPSESSION_INPUT_CAPTURE;
	}else
	{
		std::cout << "[Authentication Error] Invalid session type: " << autheMsg->GetMessageCode() << std::endl;
		sendMessageWithCode(MSG_CODE_RESULT_AUTHENTICATION_FAILED);
		return;
	}
	g_authenticated = true;
	// set waiting to a header msg
	g_currentReadState = PPREQUEST_HEADER;
	//----------------------------------------------------
	// send back to client to validate authentication successfully
	sendMessageWithCode(MSG_CODE_RESULT_AUTHENTICATION_SUCCESS);
}


void PPClientSession::sendMessageWithCode(u8 code)
{
	PPMessage* msg = new PPMessage();
	msg->BuildMessageHeader(code);
	void* buffer = msg->BuildMessageEmpty();
	g_connection->Send(buffer, 9);
	free(buffer);
}

void PPClientSession::sendMessageWithCodeAndData(u8 code, u8* buffer, size_t bufferSize)
{
	PPMessage* msg = new PPMessage();
	msg->BuildMessageHeader(code);
	void* result = msg->BuildMessage(buffer, bufferSize);
	g_connection->Send(result, msg->GetMessageSize());
	free(result);
}

void PPClientSession::preprocessMessageCode(u8 code)
{
	if(g_sessionType == PPSESSION_MOVIE)
	{
		
	}
	else if (g_sessionType == PPSESSION_SCREEN_CAPTURE)
	{
		switch (code)
		{
		case MSG_CODE_REQUEST_START_SCREEN_CAPTURE:
			// START
			g_ss_isStartStreaming = true;
			printf("[Screen capture] init frame graber.\n");
			ss_initFrameGraber();
			break;
		case MSG_CODE_REQUEST_STOP_SCREEN_CAPTURE:
			// STOP
			g_ss_isStartStreaming = false;
			printf("[Screen capture] stop framegraber.\n");
			g_ss_framgrabber.pause();
			g_ss_framgrabber.destroy();
			break;
		case MSG_CODE_REQUEST_SCREEN_RECEIVED_FRAME:
			printf("Client received frame.\n");
			g_ss_isReceivedLastFrame = true;
			break;
		default: break;
		}
	}
	else if (g_sessionType == PPSESSION_INPUT_CAPTURE)
	{

	}
}

void PPClientSession::processMessageBody(u8* buffer, u8 code)
{
	if (g_sessionType == PPSESSION_MOVIE)
	{

	}
	else if (g_sessionType == PPSESSION_SCREEN_CAPTURE)
	{
		switch (code)
		{
		case MSG_CODE_REQUEST_CHANGE_SETTING_SCREEN_CAPTURE: {

			// CHANGE SETTING
			std::cout << "GET SETTING:" << std::endl;
			int8_t waitForReceived = READ_U8(buffer, 0);
			g_ss_waitForClientReceived = !(waitForReceived == 0);

			break;
		}
		default: break;
		}
	}
	else if (g_sessionType == PPSESSION_INPUT_CAPTURE)
	{

	}
}

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------/////////////////////////////////////////////////////////////////////
// SCREEN CAPTURE										/////////////////////////////////////////////////////////////////////
//------------------------------------------------------/////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

void PPClientSession::ss_initFrameGraber()
{
	g_ss_currentWaitedFrame = 0;
	g_ss_framgrabber = SL::Screen_Capture::CreateScreeCapture([]()
	{
		auto mons = SL::Screen_Capture::GetMonitors();
		std::vector<SL::Screen_Capture::Monitor> selectedMonitor = std::vector<SL::Screen_Capture::Monitor>();
		//-------------------------- >> Index 0
		selectedMonitor.push_back(mons[1]);
		return selectedMonitor;
	}).onNewFrame([&](const SL::Screen_Capture::Image& img, const SL::Screen_Capture::Monitor& monitor)
	{
		if (!g_ss_isStartStreaming) return;
		if (g_ss_waitForClientReceived)
		{
			if (g_ss_isReceivedLastFrame)
			{
				g_ss_currentWaitedFrame = 0;
				g_ss_isReceivedLastFrame = false;
				ss_sendFrameToClient(img);
			}else
			{
				if(g_ss_currentWaitedFrame > g_ss_waitForFrame)
				{
					g_ss_currentWaitedFrame = 0;
					g_ss_isReceivedLastFrame = false;
					ss_sendFrameToClient(img);
				}
				g_ss_currentWaitedFrame++;
			}
		}
		else
		{
			ss_sendFrameToClient(img);
		}
	}).start_capturing();
	g_ss_framgrabber.setFrameChangeInterval(std::chrono::nanoseconds(10));//100 ms
	g_ss_framgrabber.resume();
}

void PPClientSession::ss_stopFrameGraber()
{
	g_ss_framgrabber.pause();
	g_ss_framgrabber.destroy();
}


void PPClientSession::ss_sendFrameToClient(const SL::Screen_Capture::Image& img)
{
	u32 newW = 400.0f * ((float)g_ss_outputScale / 100.0f);
	u32 newH = 240.0f * ((float)g_ss_outputScale / 100.0f);
	//===============================================================
	// get buffer data from image frame
	auto size = Width(img) * Height(img) * 3;
	u8* imgBuffer = (u8*)malloc(size);
	SL::Screen_Capture::ExtractAndConvertToRGB(img, (char*)imgBuffer);

	//===============================================================
	// webp encode
	try {
		WebPConfig config;
		if (!WebPConfigPreset(&config, WEBP_PRESET_PHOTO, g_ss_outputQuality)) {
			g_ss_isReceivedLastFrame = true;
			return;  // version error
		}
		//===============================================================
		// Add additional tuning:
		config.sns_strength = 90;
		config.filter_sharpness = 6;
		config.segments = 4;
		config.method = 0;
		config.alpha_compression = 0;
		config.alpha_quality = 0;
		auto config_error = WebPValidateConfig(&config);
		//===============================================================
		// Setup the input data, allocating a picture of width x height dimension
		WebPPicture pic;
		if (!WebPPictureInit(&pic)) {
			g_ss_isReceivedLastFrame = true;
			return;
		}
		pic.width = Width(img);
		pic.height = Height(img);
		if (!WebPPictureAlloc(&pic)) {
			g_ss_isReceivedLastFrame = true;
			return;
		}
		WebPPictureImportRGB(&pic, imgBuffer, Width(img)*3);
		WebPPictureRescale(&pic, newW, newH);
		WebPMemoryWriter writer;
		WebPMemoryWriterInit(&writer);
		pic.writer = WebPMemoryWrite;
		pic.custom_ptr = &writer;
		int ok = WebPEncode(&config, &pic);
		WebPPictureFree(&pic);   // Always free the memory associated with the input.
		//===============================================================
		// result send to client
		if (!ok) {
			//printf("Encoding error: %d\n", pic.error_code);
			g_ss_isReceivedLastFrame = true;
			WebPMemoryWriterClear(&writer);
			free(imgBuffer);
			return; 
		}
		//printf("Output size: %d\n", writer.size);
		sendMessageWithCodeAndData(MSG_CODE_REQUEST_NEW_SCREEN_FRAME, writer.mem, writer.size);
		WebPMemoryWriterClear(&writer);
		free(imgBuffer);
	}
	catch (...)
	{
		g_ss_isReceivedLastFrame = true;
		free(imgBuffer);
	}
}

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------/////////////////////////////////////////////////////////////////////
// MOVIE												/////////////////////////////////////////////////////////////////////
//------------------------------------------------------/////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------/////////////////////////////////////////////////////////////////////
// INPUT												/////////////////////////////////////////////////////////////////////
//------------------------------------------------------/////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------