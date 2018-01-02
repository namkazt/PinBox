#pragma once
#ifndef _PP_UI_H_
#define _PP_UI_H_
#include <3ds.h>

class PPUI
{
public:
	static void UpdateInput();
	static bool TouchOnArea(float x, float y, float w, float h);

	static void DrawNumberInputScreen();

	static bool FlatButton(float x, float y, float w, float h, const char* label);

};

#endif