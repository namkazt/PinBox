#include "PPUI.h"
#include <cstdio>


// LOG
void PPLog::InitLog()
{
	mLogMutex = new Mutex();
}

void PPLog::Write(const char* log)
{
	mLogMutex->Lock();
	if(mLogContainer.size() >= mLogMaxSize)
	{
		mLogContainer.erase(mLogContainer.begin());
	}
	mLogContainer.push_back(log);
	mLogMutex->Unlock();
}

static PPLog* mLog;

static u32 kDown;
static u32 kHeld;
static u32 kUp;

static u32 last_kDown;
static u32 last_kHeld;
static u32 last_kUp;

static circlePosition cPos;
static circlePosition cStick;
static touchPosition kTouch;
static touchPosition last_kTouch;

static std::vector<PopupCallback> mPopupList;
static std::string mTemplateInputString = "";

static const char* UI_INPUT_VALUE[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", ".", "9", ":" };

u32 PPUI::getKeyDown()
{
	return kDown;
}

u32 PPUI::getKeyHold()
{
	return kHeld;
}

u32 PPUI::getKeyUp()
{
	return kUp;
}

circlePosition PPUI::getLeftCircle()
{
	return cPos;
}

circlePosition PPUI::getRightCircle()
{
	return cStick;
}

void PPUI::UpdateInput()
{
	//----------------------------------------
	// store old input
	last_kDown = kDown;
	last_kHeld = kHeld;
	last_kUp = kUp;
	last_kTouch = kTouch;
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
		if (last_kTouch.px >= (u16)x && last_kTouch.px <= (u16)(x + w) && last_kTouch.py >= (u16)y && last_kTouch.py <= (u16)(y + h))
		{
			return true;
		}
	}
	return false;
}


int PPUI::DrawNumberInputScreen(const char* label, ResultCallback cancel, ResultCallback ok)
{
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, rgb(236, 240, 241));
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 80, rgb(26, 188, 156));

	// Screen title
	LabelBox(0, 5, 320, 30, label, rgb(26, 188, 156), rgb(255, 255, 255));

	// Input display
	LabelBox(20, 40, 280, 30, mTemplateInputString.c_str(), rgb(236, 240, 241), rgb(44, 62, 80));

	// Number pad
	for(int c = 0; c < 3; c++)
	{
		for (int r = 0; r < 4; r++)
		{
			if(FlatButton(10 + c * 35, 90 + r * 35, 30, 30, UI_INPUT_VALUE[c + r * 3]))
			{
				char v = *UI_INPUT_VALUE[c + r * 3];
				mTemplateInputString.push_back(v);
			}
		}
	}

	// Delete button
	if (FlatDarkButton(120, 90, 40, 30, "DEL"))
	{
		if (mTemplateInputString.size() > 0)
		{
			mTemplateInputString.erase(mTemplateInputString.end() - 1);
		}
	}

	// Clear button
	if (FlatDarkButton(120, 125, 40, 30, "CLR"))
	{
		mTemplateInputString = "";
	}

	// Cancel button
	if (FlatColorButton(200, 200, 50, 30, "Cancel", rgb(192, 57, 43), rgb(231, 76, 60), rgb(255, 255, 255)))
	{
		ClosePopup();
		if (cancel != nullptr) cancel(nullptr, nullptr);
	}

	// OK button
	if (FlatColorButton(260, 200, 50, 30, "OK", rgb(41, 128, 185), rgb(52, 152, 219), rgb(255, 255, 255)))
	{
		ClosePopup();
		if (ok != nullptr) ok(nullptr, nullptr);
	}

	return 0;
}


int PPUI::DrawBottomScreenUI(PPSessionManager* sessionManager)
{
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, rgb(236, 240, 241));
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 80, rgb(26, 188, 156));

	// Screen title
	switch(sessionManager->GetManagerState())
	{
		case -1: {
			LabelBox(0, 5, 320, 30, "Status: No Wifi Connection", rgb(26, 188, 156), rgb(255, 255, 255));
			break;
		}
		case 0: {
			LabelBox(0, 5, 320, 30, "Status: Ready to Connect", rgb(26, 188, 156), rgb(255, 255, 255));
			break;
		}
		case 1: {
			LabelBox(0, 5, 320, 30, "Status: Connecting...", rgb(26, 188, 156), rgb(255, 255, 255));
			break;
		}
		case 2: {
			LabelBox(0, 5, 320, 30, "Status: Connected", rgb(26, 188, 156), rgb(255, 255, 255));
			break;
		}
	}

	// IP Port
	LabelBox(20, 40, 230, 30, sessionManager->getIPAddress(), rgb(236, 240, 241), rgb(44, 62, 80));

	// Edit Button
	if (FlatColorButton(260, 40, 50, 30, "Edit", rgb(192, 57, 43), rgb(231, 76, 60), rgb(255, 255, 255)))
	{
		mTemplateInputString = std::string(sessionManager->getIPAddress());
		AddPopup([=]()
		{
			return DrawNumberInputScreen("Enter your IP and port", 
				[=](void* a, void* b)
				{
					// cancel
					mTemplateInputString = "";
				},
				[=](void* a, void* b)
				{
					// ok
					sessionManager->setIPAddress(mTemplateInputString.c_str());
					mTemplateInputString = "";
				}
			);
		});
	}

	// Tab Button

	// Tab Content
	if (FlatColorButton(260, 90, 50, 30, "Start", rgb(41, 128, 185), rgb(52, 152, 219), rgb(255, 255, 255)))
	{
		// parse IP and port
		char* string = strdup(sessionManager->getIPAddress());
		char* token;
		char* ip, *port;
		int i = 0;
		while((token = strsep(&string, ":")) != nullptr)
		{
			i++;
			if (i == 1) ip = strdup(token);
			if (i == 2) {
				port = strdup(token);
				break;
			}
		}
		sessionManager->StartStreaming(ip, port);
	}


	// Exit Button
	if (FlatColorButton(260, 200, 50, 30, "Exit", rgb(192, 57, 43), rgb(231, 76, 60), rgb(255, 255, 255)))
	{
		return -1;
	}
	return 0;
}



///////////////////////////////////////////////////////////////////////////
// BUTTON
///////////////////////////////////////////////////////////////////////////

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


///////////////////////////////////////////////////////////////////////////
// TEXT
///////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////
// POPUP
///////////////////////////////////////////////////////////////////////////

bool PPUI::HasPopup()
{
	return mPopupList.size() > 0;
}

PopupCallback PPUI::GetPopup()
{
	return mPopupList[mPopupList.size() - 1];
}

void PPUI::ClosePopup()
{
	mPopupList.erase(mPopupList.end() - 1);
}

void PPUI::AddPopup(PopupCallback callback)
{
	mPopupList.push_back(callback);
}

///////////////////////////////////////////////////////////////////////////
// LOG
///////////////////////////////////////////////////////////////////////////

int PPUI::LogWindow(float x, float y, float w, float h)
{

}
