#pragma once
#ifndef _PP_UI_H_
#define _PP_UI_H_
#include <3ds.h>
#include "PPGraphics.h"

//#define UI_DEBUG 1

class PPUI
{
public:
	static void UpdateInput();
	static bool TouchDownOnArea(float x, float y, float w, float h);
	static bool TouchUpOnArea(float x, float y, float w, float h);

	static void DrawNumberInputScreen();

	// BUTTON
	static bool FlatButton(float x, float y, float w, float h, const char* label);
	static bool FlatDarkButton(float x, float y, float w, float h, const char* label);
	static bool FlatColorButton(float x, float y, float w, float h, const char* label, ppColor colNormal, ppColor colActive, ppColor txtCol);

	static void InputField(float x, float y, float w, float h, const char* defaultValue, const char* placeHolder);
	static void LabelBox(float x, float y, float w, float h, const char* label, ppColor bgColor, ppColor txtColor);
};

#endif