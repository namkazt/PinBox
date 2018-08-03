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

#include "ConfigManager.h"
#include "PPGraphics.h"
#include "PPSessionManager.h"

#include "PPUI.h"

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

//#define CONSOLE_DEBUG 1
//#define USE_CITRA 1

void initDbgConsole()
{
#ifdef CONSOLE_DEBUG
	consoleInit(GFX_TOP, NULL);
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
	initDbgConsole();

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
	// Init config
	//---------------------------------------------
	ConfigManager::Get()->InitConfig();
	
	//---------------------------------------------
	// Init session manager
	//---------------------------------------------
	PPSessionManager* sm = new PPSessionManager();
	snprintf(sm->mIPAddress, sizeof sm->mIPAddress, "%s", ConfigManager::Get()->_cfg_ip);
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
		sm->InitInputStream();

		if(wifiStatus == 1)
		{
			// New 3DS
			osSetSpeedupEnable(true);
			sm->InitScreenCapture(1);
		}else
		{
			// Old 3ds
			sm->InitScreenCapture(1);
		}

		sm->StartFPSCounter();
		//---------------------------------------------
		// Main loop
		//---------------------------------------------
		while (aptMainLoop())
		{
			hidScanInput();
			irrstScanInput();
			//---------------------------------------------
			// Update Input
			//---------------------------------------------
			PPUI::UpdateInput();
			sm->UpdateInputStream(PPUI::getKeyHold(), PPUI::getKeyUp(), 
				PPUI::getLeftCircle().dx, PPUI::getLeftCircle().dy, 
				PPUI::getRightCircle().dx, PPUI::getRightCircle().dy);

			//---------------------------------------------
			// Update Frame
			//---------------------------------------------
			sm->UpdateVideoFrame();

			PPGraphics::Get()->BeginRender();
			//---------------------------------------------
			// Draw top UI
			//---------------------------------------------
#ifndef CONSOLE_DEBUG
			PPGraphics::Get()->RenderOn(GFX_TOP);
			if(sm->GetManagerState() == 2)
			{
				PPGraphics::Get()->DrawTopScreenSprite();
			}else
			{
				PPUI::DrawIdleTopScreen(sm);
			}
#endif
			//---------------------------------------------
			// Draw bottom UI
			//---------------------------------------------
			PPGraphics::Get()->RenderOn(GFX_BOTTOM);
			int ret = 0;
			// TODO: should be option : auto set idle mode when start streaming
			if(sm->GetManagerState() == 2 && PPUI::getSleepModeState() == 0)
			{
				ret = PPUI::DrawIdleBottomScreen(sm);
			}else
			{
				if (PPUI::HasPopup())
				{
					PopupCallback popupFunc = PPUI::GetPopup();
					ret = popupFunc();
				}
				else
				{
					// if there is no popup then render main UI
					ret = PPUI::DrawBottomScreenUI(sm);
				}
			}
			
			//---------------------------------------------
			PPGraphics::Get()->EndRender();

			sm->CollectFPSData();


			if (ret == -1) break;
		}
		//---------------------------------------------
		// End
		//---------------------------------------------
		sm->StopStreaming();
		sm->Close();
	}
		
	
	PPGraphics::Get()->GraphicExit();
	ConfigManager::Get()->Destroy();
	irrstExit();
	socExit();
	free(SOC_buffer);
	aptExit();
	acExit();

	return 0;
}