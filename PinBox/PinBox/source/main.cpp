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
#define SOC_BUFFERSIZE  0x100000

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
	acInit();
	aptInit();
	irrstInit();
	
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
	//initDbgConsole();


	//---------------------------------------------
	// Wifi Check
	//---------------------------------------------
	u32 wifiStatus = 0;
	while (aptMainLoop()) { /* Wait for WiFi; break when WiFiStatus is truthy */
		ACU_GetWifiStatus(&wifiStatus);
		if (wifiStatus) break;

		hidScanInput();
		printf("Waiting for WiFi connection...\n");

		u32 kHeld = hidKeysHeld();
		if (kHeld & KEY_START)
		{
			break;
		}
			
		gfxFlushBuffers();
		gspWaitForVBlank();
		gfxSwapBuffers();
	}
	
	//---------------------------------------------
	// wifiStatus = 0 : not connected to internet
	// wifiStatus = 1 : Old 3DS internet
	// wifiStatus = 2 : New 3DS internet
	//---------------------------------------------

	if (wifiStatus) {
		osSetSpeedupEnable(1);
		//---------------------------------------------
		// Init session manager
		//---------------------------------------------
		PPSessionManager* sm = new PPSessionManager();
		sm->InitScreenCapture(3);
		sm->InitInputStream();
		bool isStart = false;
		//---------------------------------------------
		// Main loop
		//---------------------------------------------
		while (aptMainLoop())
		{
			//Scan all the inputs. This should be done once for each frame
			hidScanInput();
			irrstScanInput();
			//---------------------------------------------
			// Update Input
			//---------------------------------------------
			u32 kDown = hidKeysDown();
			u32 kHeld = hidKeysHeld();
			u32 kUp = hidKeysUp();
			circlePosition pos;
			hidCircleRead(&pos);

			circlePosition cStick;
			irrstCstickRead(&cStick);

			sm->UpdateInputStream(kDown, kHeld, kUp, pos.dx, pos.dy, cStick.dx, cStick.dy);


			if (kHeld & KEY_START && kHeld & KEY_SELECT) break; // break in order to return to hbmenu



			if (kHeld & KEY_L && kHeld & KEY_A && !isStart)
			{
				isStart = true;
				printf("COMMAND: start stream\n");
				sm->StartStreaming("192.168.31.183", "1234");
			}
			if (kHeld & KEY_L && kHeld & KEY_B && isStart)
			{
				isStart = false;
				printf("COMMAND: stop session\n");
				sm->StopStreaming();
			}

			//---------------------------------------------
			// Update graphics
			//---------------------------------------------
			sm->UpdateFrameTracker();
			ppGraphicsRender();


			gfxFlushBuffers();
			gspWaitForVBlank();
			gfxSwapBuffers();
		}
		//---------------------------------------------
		// End
		//---------------------------------------------
		sm->StopStreaming();
		sm->Close();

	}

	ppGraphicsExit();

	irrstExit();
	socExit();
	free(SOC_buffer);
	aptExit();
	acExit();


	return 0;
}