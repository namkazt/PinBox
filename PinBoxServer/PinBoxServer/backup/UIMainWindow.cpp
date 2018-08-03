#include "stdafx.h"
#include "UIMainWindow.h"



UIMainWindow::UIMainWindow(QWidget* parent) : QMainWindow(parent), ui(new UIMainWindow)
{

}

UIMainWindow::~UIMainWindow()
{
	delete ui;
}
