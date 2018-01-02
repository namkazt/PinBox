#include "PPUI.h"
#include <cstdio>

static u32 kDown;
static u32 kHeld;
static u32 kUp;

static u32 last_kDown;
static u32 last_kHeld;
static u32 last_kUp;

static circlePosition cPos;
static circlePosition cStick;
static touchPosition kTouch;

static const char* UI_INPUT_VALUE[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", ".", "9", ":" };

void PPUI::UpdateInput()
{
	//----------------------------------------
	// store old input
	last_kDown = kDown;
	last_kHeld = kHeld;
	last_kUp = kUp;
	//----------------------------------------
	// scan new input
	kDown = hidKeysDown();
	kHeld = hidKeysHeld();
	kUp = hidKeysUp();
	cPos = circlePosition();
	hidCircleRead(&cPos);
	cStick = circlePosition();
	irrstCstickRead(&cStick);
	kTouch = touchPosition();
	hidTouchRead(&kTouch);
}

bool PPUI::TouchDownOnArea(float x, float y, float w, float h)
{
	if (kDown & KEY_TOUCH || kHeld & KEY_TOUCH)
	{
		if (kTouch.px >= (u16)x && kTouch.px <= (u16)(x + w) && kTouch.py >= (u16)y && kTouch.py <= (u16)(y + h))
		{
			return true;
		}
	}
	return false;
}

bool PPUI::TouchUpOnArea(float x, float y, float w, float h)
{
	if ((last_kDown & KEY_TOUCH || last_kHeld & KEY_TOUCH) && kUp & KEY_TOUCH)
	{
		if (kTouch.px >= (u16)x && kTouch.px <= (u16)(x + w) && kTouch.py >= (u16)y && kTouch.py <= (u16)(y + h))
		{
			return true;
		}
	}
	return false;
}

void PPUI::DrawNumberInputScreen()
{
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, rgb(236, 240, 241));
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 80, rgb(26, 188, 156));

	// Screen title
	LabelBox(0, 5, 320, 30, "Input For xxx", rgb(26, 188, 156), rgb(255, 255, 255));

	// Input display
	LabelBox(20, 40, 280, 30, "192.168.31.183:1234", rgb(236, 240, 241), rgb(44, 62, 80));

	// Number pad
	for(int c = 0; c < 3; c++)
	{
		for (int r = 0; r < 4; r++)
		{
			if(FlatButton(10 + c * 35, 90 + r * 35, 30, 30, UI_INPUT_VALUE[c + r * 3]))
			{
				
			}
		}
	}

	// Delete button
	if (FlatDarkButton(120, 90, 40, 30, "DEL"))
	{

	}

	// Clear button
	if (FlatDarkButton(120, 125, 40, 30, "CLR"))
	{

	}

	// Cancel button
	if (FlatColorButton(200, 200, 50, 30, "Cancel", rgb(192, 57, 43), rgb(231, 76, 60), rgb(255, 255, 255)))
	{

	}

	// OK button
	if (FlatColorButton(260, 200, 50, 30, "OK", rgb(41, 128, 185), rgb(52, 152, 219), rgb(255, 255, 255)))
	{

	}
}

bool PPUI::FlatButton(float x, float y, float w, float h, const char* label)
{
	return FlatColorButton(x, y, w, h, label, rgb(26, 188, 156), rgb(46, 204, 113), rgb(236, 240, 241));
}

bool PPUI::FlatDarkButton(float x, float y, float w, float h, const char* label)
{
	return FlatColorButton(x, y, w, h, label, rgb(22, 160, 133), rgb(39, 174, 96), rgb(236, 240, 241));
}

bool PPUI::FlatColorButton(float x, float y, float w, float h, const char* label, ppColor colNormal, ppColor colActive, ppColor txtCol)
{
	bool isTouchDown = TouchDownOnArea(x, y, w, h);
	float tScale = 0.5f;
	if (isTouchDown)
	{
		PPGraphics::Get()->DrawRectangle(x, y, w, h, colActive);
		tScale = 0.6f;
	}
	else
	{
		PPGraphics::Get()->DrawRectangle(x, y, w, h, colNormal);
	}
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, tScale, tScale);
	float startX = (w - tSize.x) / 2.0f;
	float startY = (h - tSize.y) / 2.0f;
	PPGraphics::Get()->DrawText(label, x + startX, y + startY, tScale, tScale, txtCol, false);
	return TouchUpOnArea(x, y, w, h);
}


/**
 * \brief Draw label box
 * \param x 
 * \param y 
 * \param w 
 * \param h 
 * \param defaultValue 
 * \param placeHolder 
 */
void PPUI::LabelBox(float x, float y, float w, float h, const char* label, ppColor bgColor, ppColor txtColor)
{
	PPGraphics::Get()->DrawRectangle(x, y, w, h, bgColor);
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, 0.5f, 0.5f);
	float startX = (w - tSize.x) / 2.0f;
	float startY = (h - tSize.y) / 2.0f;
	PPGraphics::Get()->DrawText(label, x + startX, y + startY, 0.5f, 0.5f, txtColor, false);

#ifdef UI_DEBUG
	char buffer[100];
	snprintf(buffer, sizeof buffer, "%f  %f", tSize.x, tSize.y);
	PPGraphics::Get()->DrawText(buffer, x , y - 10, 0.3f, 0.3f, txtColor, false);
#endif
}


/**
 * \brief Draw text box and show Input screen when click in
 * \param x 
 * \param y 
 * \param w 
 * \param h 
 * \param defaultValue 
 * \param placeHolder 
 */
void PPUI::InputField(float x, float y, float w, float h, const char* defaultValue, const char* placeHolder)
{

}

