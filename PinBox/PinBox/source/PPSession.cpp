#include "PPSession.h"


PPSession::~PPSession()
{
	if (g_network != nullptr) delete g_network;
}


void PPSession::initSession()
{
	if (g_network != nullptr) return;
	g_network = new PPNetwork();
	printf("Init new session.\n");
}


void PPSession::InitMovieSession()
{
	initSession();
	//--------------------------------------
	// init specific for movie session
	g_sessionType = PPSESSION_MOVIE;
	printf("Init movie session.\n");
	// callback when request data is received
	g_network->SetOnReceivedRequest([=](void* buffer, u32 size, u32 tag){
		//------------------------------------------------------
		// process data by tag
		switch (tag)
		{
			case PPREQUEST_AUTHEN:
			{
				g_authenticated = true;
				break;
			}
			case PPREQUEST_HEADER:
			{
				break;
			}
			case PPREQUEST_BODY:
			{
				break;
			}
		}
		//------------------------------------------------------
		// free buffer after use
		linearFree(buffer);
	});
}

void PPSession::InitScreenCaptureSession()
{
	initSession();
	//--------------------------------------
	// init specific for movie session
	g_sessionType = PPSESSION_SCREEN_CAPTURE;
	printf("Init screen capture session.\n");
	// callback when request data is received
	g_network->SetOnReceivedRequest([=](void* buffer, u32 size, u32 tag) {
		//------------------------------------------------------
		// process data by tag
		switch (tag)
		{
		case PPREQUEST_AUTHEN:
		{
			g_authenticated = true;
			break;
		}
		case PPREQUEST_HEADER:
		{
			break;
		}
		case PPREQUEST_BODY:
		{
			break;
		}
		}
		//------------------------------------------------------
		// free buffer after use
		linearFree(buffer);
	});
}

void PPSession::InitInputCaptureSession()
{
	initSession();
	//--------------------------------------
	// init specific for movie session
	g_sessionType = PPSESSION_INPUT_CAPTURE;
	printf("Init input session.\n");
	// callback when request data is received
	g_network->SetOnReceivedRequest([=](void* buffer, u32 size, u32 tag) {
		//------------------------------------------------------
		// process data by tag
		switch (tag)
		{
		case PPREQUEST_AUTHEN:
		{
			g_authenticated = true;
			break;
		}
		case PPREQUEST_HEADER:
		{
			break;
		}
		case PPREQUEST_BODY:
		{
			break;
		}
		}
		//------------------------------------------------------
		// free buffer after use
		linearFree(buffer);
	});
}

void PPSession::StartSession()
{
	if (g_network == nullptr) return;
	g_network->Start("192.168.31.222","1234");
	//TODO: authentication message data
	g_network->SendMessage(nullptr, 12);
	g_network->SetRequestData(12, PPREQUEST_AUTHEN);
}

void PPSession::CloseSession()
{
	if (g_network == nullptr) return;
	g_network->Stop();
}
