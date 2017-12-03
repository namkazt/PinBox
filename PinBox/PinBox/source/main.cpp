//===============================================================================
// enable this for debug log console on top
//===============================================================================
#define __CONSOLE_DEBUGING__
//===============================================================================

#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <arpa/inet.h>

#include "PPGraphics.h"
#include "PPSessionManager.h"

#include "dog_webp.h"



#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x800000

void initDbgConsole()
{
#ifdef __CONSOLE_DEBUGING__
	consoleInit(GFX_BOTTOM, NULL);
	printf("Console log initialized\n");
#endif
}

static u32 *SOC_buffer = NULL;

int main()
{
	//---------------------------------------------
	// Init svc
	//---------------------------------------------
	aptInit();
	
	//---------------------------------------------
	// Init SOCKET
	//---------------------------------------------
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
	u32 ret = socInit(SOC_buffer, SOC_BUFFERSIZE);
	if (ret != 0)
	{
		printf("Init SOC services failed.\n");
		return;
	}

	//---------------------------------------------
	// Init Graphics
	//---------------------------------------------
	ppGraphicsInit();
	initDbgConsole();


	// init webp decode config
	/*WebPDecoderConfig config;
	WebPInitDecoderConfig(&config);
	config.options.no_fancy_upsampling = 1;
	config.options.use_scaling = 1;
	config.options.scaled_width = 400;
	config.options.scaled_height = 240;
	if (!WebPGetFeatures(dog_webp, dog_webp_size, &config.input) == VP8_STATUS_OK)
		return;
	config.output.colorspace = MODE_BGR;
	if (!WebPDecode(dog_webp, dog_webp_size, &config) == VP8_STATUS_OK)
		return;

	ppGraphicsDrawFrame(config.output.private_memory, 400 * 240 * 3, 400, 240);
	
	WebPFreeDecBuffer(&config.output);*/

	
	//---------------------------------------------
	// Init session manager
	//---------------------------------------------

	PPSessionManager* sm = new PPSessionManager();
	sm->InitScreenCapture(2);

	bool isStart = false;
	//---------------------------------------------
	// Main loop
	//---------------------------------------------
	while (aptMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break; // break in order to return to hbmenu

		if(kDown & KEY_A && !isStart)
		{
			isStart = true;
			printf("COMMAND: start stream\n");
			sm->StartStreaming("192.168.31.222", "1234");
		}
		if (kDown & KEY_B && isStart)
		{
			isStart = false;
			printf("COMMAND: stop session\n");
			sm->StopStreaming();
		}


		sm->UpdateFrameTracker();
		ppGraphicsRender();
	}

	
	//---------------------------------------------
	// End
	//---------------------------------------------
	sm->StopStreaming();
	ppGraphicsExit();

	socExit();
	free(SOC_buffer);
	aptExit();

	return 0;
}