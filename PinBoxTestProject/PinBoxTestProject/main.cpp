
#include <iostream>
#include "PPSessionManager.h"

void updateSessionManager(void* arg)
{
	PPSessionManager* sm = (PPSessionManager*)arg;
	if( sm != nullptr)
	{
		while(true)
		{
			sm->UpdateFrameTracker();
		}
	}
}


int main()
{

#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	std::cout << "==============================================================\n";
	std::cout << "= Test application for PinBox. Simulation client side (3ds)  =\n";
	std::cout << "==============================================================\n";
	PPSessionManager* sm = new PPSessionManager();
	sm->InitScreenCapture(1);

	std::thread g_thread = std::thread(updateSessionManager, sm);
	g_thread.detach();

	std::cout << "Press Q to exit.\nInput: ";
	char input;
	while(true)
	{
		std::cin >> input;
		if (input == 'q') break;
		if(input == 'a')
		{
			sm->StartStreaming("192.168.31.222", "1234");
		}
		if (input == 's')
		{
			sm->StopStreaming();
		}
		input = ' ';
		//-----------------------------------------
	}
	std::cout << "\nClosing session.\n";
	sm->Close();
	return 0;
}
