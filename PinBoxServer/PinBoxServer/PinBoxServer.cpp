// PinBoxServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "PPServer.h"
#include "AudioStreamSession.h"



int main()
{
	//AudioStreamSession* ass = new AudioStreamSession();
	//ass->SetOnRecordData([=](u8* output, u32 size, u32 frames)
	//{
	//});
	//ass->StartAudioStream();

	PPServer *server = new PPServer();
	server->InitServer();

	/*char a;
	std::cin >> a;
	if (a == 'f') {
		ass->StopStreaming();
	}
*/
	return 0;
}

#include "winmain-inl.h"