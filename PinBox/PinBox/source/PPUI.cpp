#include "PPUI.h"
#include "PPGraphics.h"
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

bool PPUI::TouchOnArea(float x, float y, float w, float h)
{
	if ((last_kDown & KEY_TOUCH || last_kHeld & KEY_TOUCH) && kUp & KEY_TOUCH)
	{
		if (kTouch.px >= x && kTouch.px <= (x + w) && kTouch.py >= y && kTouch.py <= (y + h))
		{
			return true;
		}
	}
	return false;
}

void PPUI::DrawNumberInputScreen()
{
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, rgb(236, 240, 241));
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 30, rgb(26, 188, 156));

	FlatButton(0, 0, 50, 30, "test4");
}

bool PPUI::FlatButton(float x, float y, float w, float h, const char* label)
{
	bool isTouched = TouchOnArea(x, y, w, h);
	float tScale = 0.5f;
	if(isTouched)
	{
		PPGraphics::Get()->DrawRectangle(x, y, w, h, rgb(46, 204, 113));
		tScale = 0.6f;
	}else
	{
		PPGraphics::Get()->DrawRectangle(x, y, w, h, rgb(26, 188, 156));
	}
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, tScale, tScale);
	float startX = (w - tSize.x) / 2.0f;
	float startY = (h - tSize.y) / 2.0f;
	PPGraphics::Get()->DrawText(label, x + startX, y + startY, tScale, tScale, rgb(236, 240, 241), false);
	
	return isTouched;
}

