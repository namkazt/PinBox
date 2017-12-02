#include "PPSession.h"
#include <iostream>
#include "opencv/cv.h"

int main()
{
	if (htonl(47) == 47)
		printf("System using Big Endian\n");
	else
		printf("System using Little Endian\n");

	std::cout << "==============================================================\n";
	std::cout << "= Test application for PinBox. Simulation client side (3ds)  =\n";
	std::cout << "==============================================================\n";
	PPSession* session = new PPSession();
	session->InitScreenCaptureSession();

	std::cout << "Press Q to exit.\nInput: ";
	char input;
	while(true)
	{
		std::cin >> input;
		if (input == 'q') break;

		if (input == 'x')
			session->StartSession("192.168.31.222", "1234");
		if(input == 'a')
		{
			session->SS_ChangeSetting();
			session->SS_StartStream();
		}
			
		if (input == 's')
			session->SS_StopStream();
		input = ' ';
	}
	std::cout << "\nClosing session.\n";
	session->CloseSession();
	return 0;
}