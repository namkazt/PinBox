#pragma once
#include <QtWidgets/QMainWindow>

class UIMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit UIMainWindow(QWidget *parent = 0);
	~UIMainWindow();
private:
	UIMainWindow *ui;
};

