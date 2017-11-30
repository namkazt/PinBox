//===============================================================================
// enable this for debug log console on top
//===============================================================================
#define __CONSOLE_DEBUGING__
//===============================================================================

#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "PPGraphics.h"
#include "PPSession.h"

void initDbgConsole()
{
#ifdef __CONSOLE_DEBUGING__
	consoleInit(GFX_BOTTOM, NULL);
	printf("Console log initialized\n");
#endif
}

int main()
{
	//---------------------------------------------
	// Init svc
	//---------------------------------------------
	ppGraphicsInit();
	
	initDbgConsole();


	//---------------------------------------------
	// Init session manager
	//---------------------------------------------

	PPSession* session = new PPSession();
	session->InitScreenCaptureSession();
	session->StartSession();

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


		ppGraphicsRender();
	}

	//---------------------------------------------
	// End
	//---------------------------------------------
	ppGraphicsExit();


	return 0;
}