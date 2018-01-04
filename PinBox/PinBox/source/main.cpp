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
#include "PPUI.h"

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

//#define USE_CITRA 1

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
	// Init Graphics
	//---------------------------------------------
	PPGraphics::Get()->GraphicsInit();
	//initDbgConsole();

	//---------------------------------------------
	// Init SOCKET
	//---------------------------------------------
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
	u32 ret = socInit(SOC_buffer, SOC_BUFFERSIZE);
	if (ret != 0)
	{
		return 0;
	}
	
	//---------------------------------------------
	// Init session manager
	//---------------------------------------------
	PPSessionManager* sm = new PPSessionManager();
	
	//---------------------------------------------
	// Wifi Check
	//---------------------------------------------
	u32 wifiStatus = 0;
#ifndef USE_CITRA
	while (aptMainLoop()) {
		ACU_GetWifiStatus(&wifiStatus);
		if (wifiStatus) break;

		//---------------------------------------------
		// Update Input
		//---------------------------------------------
		hidScanInput();
		irrstScanInput();
		PPUI::UpdateInput();

		//---------------------------------------------
		// Draw UI
		//---------------------------------------------
		PPGraphics::Get()->BeginRender();
		PPGraphics::Get()->RenderOn(GFX_BOTTOM);
		int ret = PPUI::DrawBottomScreenUI(sm);
		PPGraphics::Get()->EndRender();

		if (ret == -1) break;
	}
#else
	wifiStatus = 1;
#endif
	//---------------------------------------------
	// wifiStatus = 0 : not connected to internet
	// wifiStatus = 1 : Old 3DS internet
	// wifiStatus = 2 : New 3DS internet
	//---------------------------------------------
	if (wifiStatus) {
		osSetSpeedupEnable(1);
		
		sm->InitInputStream();
		sm->InitScreenCapture(3);

		//---------------------------------------------
		// Main loop
		//---------------------------------------------
		while (aptMainLoop())
		{
			gspWaitForVBlank();
			hidScanInput();
			irrstScanInput();
			//---------------------------------------------
			// Update Input
			//---------------------------------------------
			PPUI::UpdateInput();
			sm->UpdateInputStream(PPUI::getKeyDown(), PPUI::getKeyHold(), PPUI::getKeyUp(), 
				PPUI::getLeftCircle().dx, PPUI::getLeftCircle().dy, 
				PPUI::getRightCircle().dx, PPUI::getRightCircle().dy);
			//---------------------------------------------
			// Update Frame
			//---------------------------------------------
			sm->UpdateFrameTracker();

			//---------------------------------------------
			// Debug
			//---------------------------------------------
			//if (PPUI::getKeyHold() & KEY_START && PPUI::getKeyHold() & KEY_SELECT) break; // break in order to return to hbmenu
			//if (PPUI::getKeyHold() & KEY_L && PPUI::getKeyHold() & KEY_A)
			//{
			//	printf("COMMAND: start stream \n");
			//	gfxFlushBuffers();
			//	sm->StartStreaming("192.168.31.183", "1234");
			//}

			PPGraphics::Get()->BeginRender();
			//---------------------------------------------
			// Draw top UI
			//---------------------------------------------
			PPGraphics::Get()->RenderOn(GFX_TOP);
			if(sm->GetManagerState() == 2)
			{
				PPGraphics::Get()->DrawTopScreenSprite();
			}else
			{
				PPUI::DrawIdleTopScreen(sm);
			}
			
			//---------------------------------------------
			// Draw bottom UI
			//---------------------------------------------
			PPGraphics::Get()->RenderOn(GFX_BOTTOM);
			int ret = 0;
			if(PPUI::HasPopup())
			{
				PopupCallback popupFunc = PPUI::GetPopup();
				ret = popupFunc();
			}else
			{
				// if there is no popup then render main UI
				ret = PPUI::DrawBottomScreenUI(sm);
			}
			//---------------------------------------------
			PPGraphics::Get()->EndRender();
			if (ret == -1) break;

		}
		//---------------------------------------------
		// End
		//---------------------------------------------
		sm->StopStreaming();
		sm->Close();
	}
		
	
	PPGraphics::Get()->GraphicExit();
	irrstExit();
	socExit();
	free(SOC_buffer);
	aptExit();
	acExit();

	return 0;
}