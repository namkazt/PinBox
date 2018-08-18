//===============================================================================
// enable this for debug log console on top
//===============================================================================
//define __CONSOLE_DEBUGING__
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
#include "PPAudio.h"
#include "constant.h"

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
	APT_SetAppCpuTimeLimit(80);
	//---------------------------------------------
	// Init Graphics
	//---------------------------------------------
	PPGraphics::Get()->GraphicsInit();
	PPAudio::Get()->AudioInit();
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
		osSetSpeedupEnable(1);

		sm->NewSession();
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
			int r = 0;
			if(sm->GetManagerState() == 2 && PPUI::getSleepModeState() == 0)
			{
				r = PPUI::DrawIdleBottomScreen(sm);
			}else
			{
				if (PPUI::HasPopup())
				{
					PopupCallback popupFunc = PPUI::GetPopup();
					r = popupFunc();
				}
				else
				{
					// if there is no popup then render main UI
					r = PPUI::DrawBottomScreenUI(sm);
				}
			}
			
			//---------------------------------------------
			PPGraphics::Get()->EndRender();

			sm->CollectFPSData();

			if (r == -1) break;
		}
		//---------------------------------------------
		// End
		//---------------------------------------------
		sm->StopStreaming();
	}
		
	
	PPGraphics::Get()->GraphicExit();
	PPAudio::Get()->AudioExit();
	ConfigManager::Get()->Destroy();
	irrstExit();
	socExit();
	free(SOC_buffer);
	aptExit();
	acExit();

	return 0;
}