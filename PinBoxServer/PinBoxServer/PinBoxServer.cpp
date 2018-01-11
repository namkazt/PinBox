// PinBoxServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "PPServer.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>

int main(int argc, char *argv[])
{
	QApplication PinboxServer(argc, argv);
	//QPushButton pushButton("taweawaw");
	//pushButton.show();

	PPServer *server = new PPServer();
	server->InitServer();

	return PinboxServer.exec();
}

#include "winmain-inl.h"