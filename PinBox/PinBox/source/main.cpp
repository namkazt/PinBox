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
		return;
	}

	
	////-------------------------------------------
	//// init webp decode config
	//u32 nW = 512, nH = 256;
	//WebPDecoderConfig config;
	//WebPInitDecoderConfig(&config);
	//config.options.no_fancy_upsampling = 1;
	//config.options.use_scaling = 1;
	//config.options.scaled_width = nW;
	//config.options.scaled_height = nH;
	//if (!WebPGetFeatures(dog_webp, dog_webp_size, &config.input) == VP8_STATUS_OK)
	//	return;
	//config.output.colorspace = MODE_BGR;
	//if (!WebPDecode(dog_webp, dog_webp_size, &config) == VP8_STATUS_OK)
	//	return;
	////-------------------------------------------
	//// draw
	//PPGraphics::Get()->UpdateTopScreenSprite(config.output.private_memory, nW * nH * 3, nW, nH);
	////-------------------------------------------
	//// free data
	//WebPFreeDecBuffer(&config.output);
	
	/*
	while (aptMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		u32 kUp = hidKeysUp();
		if (kDown & KEY_START) break; // break in order to return to hbmenu


		PPGraphics::Get()->BeginRender();

		// draw on top screen
		PPGraphics::Get()->RenderOn(GFX_TOP);
		PPGraphics::Get()->DrawTopScreenSprite();

		// draw on bottom screen
		PPGraphics::Get()->RenderOn(GFX_BOTTOM);
		PPGraphics::Get()->DrawRectangle(10, 10, 10, 10, ppColor{ 255, 150, 0, 255 });

		if(kHeld & KEY_A)
		{
			PPGraphics::Get()->DrawText("Pressed A: YES", 10, 50, 0.5f, 0.5f, ppColor{ 0, 255, 0, 255 }, false);
		}else
		{
			PPGraphics::Get()->DrawText("Pressed A: NO", 10, 50, 0.5f, 0.5f, ppColor{ 0, 255, 0, 255 }, false);
		}
		

		PPGraphics::Get()->EndRender();
		gspWaitForVBlank();
	}
	*/
	
	
	//---------------------------------------------
	// Wifi Check
	//---------------------------------------------
	/*u32 wifiStatus = 0;
	while (aptMainLoop()) {
		ACU_GetWifiStatus(&wifiStatus);
		if (wifiStatus) break;

		hidScanInput();
		u32 kHeld = hidKeysHeld();
		if (kHeld & KEY_START)
		{
			break;
		}
			
		gfxFlushBuffers();
		gspWaitForVBlank();
		gfxSwapBuffers();
	}*/
	
	//---------------------------------------------
	// wifiStatus = 0 : not connected to internet
	// wifiStatus = 1 : Old 3DS internet
	// wifiStatus = 2 : New 3DS internet
	//---------------------------------------------

	osSetSpeedupEnable(1);
	//---------------------------------------------
	// Init session manager
	//---------------------------------------------
	PPSessionManager* sm = new PPSessionManager();
	sm->InitInputStream();
	sm->InitScreenCapture(3);
	
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
		PPUI::UpdateInput();

		//sm->UpdateInputStream(PPUI::kDown, PPUI::kHeld, PPUI::kUp, PPUI::cPos.dx, PPUI::cPos.dy, PPUI::cStick.dx, PPUI::cStick.dy);
		


		//if (kHeld & KEY_START && kHeld & KEY_SELECT) break; // break in order to return to hbmenu
		//if (kHeld & KEY_L &&kHeld & KEY_A && !isStart)
		//{
		//	isStart = true;
		//	printf("COMMAND: start stream \n");
		//	gfxFlushBuffers();
		//	sm->StartStreaming("192.168.31.183", "1234");
		//}
		//if (kHeld & KEY_L && kHeld & KEY_B && isStart)
		//{
		//	isStart = false;
		//	printf("COMMAND: start stream \n");
		//	gfxFlushBuffers();
		//	sm->StopStreaming();
		//}

		PPGraphics::Get()->BeginRender();
		PPGraphics::Get()->RenderOn(GFX_TOP);
		sm->UpdateFrameTracker();
		PPGraphics::Get()->DrawTopScreenSprite();
			
		// draw on bottom screen
		PPGraphics::Get()->RenderOn(GFX_BOTTOM);
		PPUI::DrawNumberInputScreen();

		PPGraphics::Get()->EndRender();

		gspWaitForVBlank();
	}
	//---------------------------------------------
	// End
	//---------------------------------------------
	sm->StopStreaming();
	sm->Close();

		
	
	



	PPGraphics::Get()->GraphicExit();
	irrstExit();
	socExit();
	free(SOC_buffer);
	aptExit();
	acExit();

	return 0;
}