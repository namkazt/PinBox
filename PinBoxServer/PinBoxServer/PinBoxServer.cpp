// PinBoxServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "PPServer.h"
#include "AudioStreamSession.h"

static u_char* recordedAudio;
static uint32_t recordedIndex = 0;
static uint32_t totalFrames = 0;


int main()
{

	AudioStreamSession* ass = new AudioStreamSession();
	ass->SetOnRecordData([=](u8* buffer, u32 size, u32 frames)
	{
		int a = 0;
	});
	ass->StartAudioStream();

	/*FILE* fd = fopen("record.raw", "wb");
	fwrite(recordedAudio, sizeof(u_char), recordedIndex * 2, fd);
	fclose(fd);*/


	//PPServer *server = new PPServer();
	//server->InitServer();

	char a;
	std::cin >> a;
	if (a == 'f') {
		ass->StopStreaming();
	}

	return 0;
}

#include "winmain-inl.h"