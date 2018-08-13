#include "PPUI.h"
#include <cstdio>
#include "ConfigManager.h"

volatile u32 kDown;
volatile u32 kHeld;
volatile u32 kUp;

volatile u32 last_kDown;
volatile u32 last_kHeld;
volatile u32 last_kUp;

static circlePosition cPos;
static circlePosition cStick;
static touchPosition kTouch;
static touchPosition last_kTouch;
static touchPosition first_kTouchDown;
static touchPosition last_kTouchDown;
volatile u64 holdTime = 0;

static u32 sleepModeState = 0;

static std::vector<PopupCallback> mPopupList;
static std::string mTemplateInputString = "";

static const char* UI_INPUT_VALUE[] = { "1", "2", "3", 
										"4", "5", "6", 
										"7", "8", "9", 
										".", "0", ":" };

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

u32 PPUI::getSleepModeState()
{
	return sleepModeState;
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

	if(kDown & KEY_TOUCH)
	{
		first_kTouchDown = kTouch;
		last_kTouchDown = touchPosition();
	}
	if(last_kHeld & KEY_TOUCH && kUp & KEY_TOUCH)
	{
		last_kTouchDown = kTouch;
		first_kTouchDown = touchPosition();
		holdTime = 0;
	}
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

bool PPUI::TouchDown()
{
	return kDown & KEY_TOUCH;
}

bool PPUI::TouchMove()
{
	return kHeld & KEY_TOUCH;
}

bool PPUI::TouchUp()
{
	return last_kHeld & KEY_TOUCH && kUp & KEY_TOUCH;
}

///////////////////////////////////////////////////////////////////////////
// TEXT
///////////////////////////////////////////////////////////////////////////

int PPUI::DrawIdleTopScreen(PPSessionManager* sessionManager)
{
	PPGraphics::Get()->DrawRectangle(0, 0, 400, 240, rgb(26, 188, 156));
	LabelBox(0, 0, 400, 240, "PinBox", rgb(26, 188, 156), rgb(255, 255, 255));
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
		if (sessionManager->GetManagerState() == 2) return 0;

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
					ConfigManager::Get()->_cfg_ip = strdup(mTemplateInputString.c_str());
					ConfigManager::Get()->Save();
					mTemplateInputString = "";
				}
			);
		});
	}

	// Tab Button

	// Tab Content

	if (FlatColorButton(260, 90, 50, 30, "Start", rgb(41, 128, 185), rgb(52, 152, 219), rgb(255, 255, 255)))
	{
		if (sessionManager->GetManagerState() == 2) return 0;
		sessionManager->StartStreaming(sessionManager->getIPAddress());
	}

	// Sleep mode
	if (FlatColorButton(200, 90, 50, 30, "Sleep", rgb(39, 174, 96), rgb(46, 204, 113), rgb(255, 255, 255)))
	{
		if (sleepModeState == 1) sleepModeState = 0;
	}


	// Config mode
	if (FlatColorButton(10, 90, 120, 30, "> Stream Config", rgb(39, 174, 96), rgb(46, 204, 113), rgb(255, 255, 255)))
	{
		AddPopup([=]()
		{
			return DrawStreamConfigUI(sessionManager,
				[=](void* a, void* b)
			{
				// cancel
			},
				[=](void* a, void* b)
			{
				// ok
				// save config
				ConfigManager::Get()->Save();
				// send new setting to server
				sessionManager->UpdateStreamSetting();
			}
			);
		});
	}


	// Exit Button
	if (FlatColorButton(260, 200, 50, 30, "Exit", rgb(192, 57, 43), rgb(231, 76, 60), rgb(255, 255, 255)))
	{
		return -1;
	}


	ProfileLoad(sessionManager);
	DrawFPS(sessionManager);

	return 0;
}

int PPUI::DrawStreamConfigUI(PPSessionManager* sessionManager, ResultCallback cancel, ResultCallback ok)
{
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, rgb(236, 240, 241));
	LabelBox(0, 0, 320, 30, "Stream Config", rgb(26, 188, 156), rgb(255, 255, 255));


	
	ConfigManager::Get()->_cfg_video_quality = Slide(5, 40, 300, 30, ConfigManager::Get()->_cfg_video_quality, 10, 100, "Quality");
	ConfigManager::Get()->_cfg_video_scale = Slide(5, 70, 300, 30, ConfigManager::Get()->_cfg_video_scale, 10, 100, "Scale");
	ConfigManager::Get()->_cfg_skip_frame = Slide(5, 100, 300, 30, ConfigManager::Get()->_cfg_skip_frame, 0, 60, "Skip Frame");

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
}

int PPUI::DrawIdleBottomScreen(PPSessionManager* sessionManager)
{
	// touch screen to wake up
	if(TouchUpOnArea(0,0, 320, 240))
	{
		sleepModeState = 1;
	}
	// label
	LabelBox(0, 0, 320, 240, "Touch screen to wake up", rgb(0, 0, 0), rgb(125, 125, 125));

	ProfileLoad(sessionManager);
	DrawFPS(sessionManager);

	return 0;
}

void PPUI::DrawFPS(PPSessionManager* sessionManager)
{
	// render video FPS
	char videoFpsBuffer[100];
	snprintf(videoFpsBuffer, sizeof videoFpsBuffer, "FPS:%.1f|VPS:%.1f", sessionManager->GetFPS(), sessionManager->GetVideoFPS());
	LabelBoxLeft(5, 220, 100, 20, videoFpsBuffer, ppColor{ 0, 0, 0, 0 }, rgb(150, 150, 150), 0.4f);
}

void PPUI::ProfileLoad(PPSessionManager* sessionManager)
{
	char videoFpsBuffer[100];
	snprintf(videoFpsBuffer, sizeof videoFpsBuffer, "CPU:%.1f|GPU:%.1f|CMD:%.1f", C3D_GetProcessingTime()*6.0f, C3D_GetDrawingTime()*6.0f, C3D_GetCmdBufUsage()*100.0f);
	LabelBoxLeft(5, 200, 100, 20, videoFpsBuffer, ppColor{ 0, 0, 0, 0 }, rgb(150, 150, 150), 0.4f);
}

///////////////////////////////////////////////////////////////////////////
// SLIDE
///////////////////////////////////////////////////////////////////////////

float PPUI::Slide(float x, float y, float w, float h, float val, float min, float max, const char* label)
{
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, 0.5f, 0.5f);
	float labelY = (h - tSize.y) / 2.0f;
	float labelX = x + 5.f;
	float slideX = w / 100.f * 35.f;
	float marginY = 2;

	if (val < min) val = min;
	if (val > max) val = max;
	// draw label
	PPGraphics::Get()->DrawText(label, x + labelX, y + labelY, 0.5f, 0.5f, rgb(26, 26, 26), false);

	// draw bg
	float startX = x + slideX;
	float startY = y + marginY;
	w = w - slideX;
	h = h - 2 * marginY;
	PPGraphics::Get()->DrawRectangle(startX, startY, w, h, PPGraphics::Get()->PrimaryDarkColor);
	
	char valBuffer[50];
	snprintf(valBuffer, sizeof valBuffer, "%.1f", val, val);
	ppVector2 valSize = PPGraphics::Get()->GetTextSize(valBuffer, 0.5f, 0.5f);
	float valueX = (w - valSize.x) / 2.0f;
	float valueY = (h - valSize.y) / 2.0f;

	// draw value
	PPGraphics::Get()->DrawText(valBuffer, startX + valueX, startY + valueY, 0.5f, 0.5f, PPGraphics::Get()->AccentTextColor, false);
	float newValue = val;
	// draw plus and minus button
	if(RepeatButton(startX + 1, startY + 1, 30 - 2, h - 2, "<", rgb(236, 240, 241), rgb(189, 195, 199), rgb(44, 62, 80)))
	{
		// minus
		newValue -= 1;
	}

	if(RepeatButton(startX + w - 30, startY + 1, 30 - 1, h - 2, ">", rgb(236, 240, 241), rgb(189, 195, 199), rgb(44, 62, 80)))
	{
		// plus
		newValue += 1;
	}
	if (newValue < min) newValue = min;
	if (newValue > max) newValue = max;
	return newValue;
}

///////////////////////////////////////////////////////////////////////////
// CHECKBOX
///////////////////////////////////////////////////////////////////////////
bool PPUI::CheckBox(float x, float y, bool value, const char* label)
{

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
	float tScale = 0.5f;
	if (TouchDownOnArea(x, y, w, h))
	{
		PPGraphics::Get()->DrawRectangle(x, y, w, h, colActive);
		tScale = 0.6f;
	}
	else
	{
		PPGraphics::Get()->DrawRectangle(x, y, w, h, colNormal);
	}
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, tScale, tScale);
	PPGraphics::Get()->DrawText(label, x + (w - tSize.x) / 2.0f, y + (h - tSize.y) / 2.0f, tScale, tScale, txtCol, false);
	return TouchUpOnArea(x, y, w, h);
}

bool PPUI::RepeatButton(float x, float y, float w, float h, const char* label, ppColor colNormal, ppColor colActive, ppColor txtCol)
{
	bool isTouchDown = TouchDownOnArea(x, y, w, h);
	float tScale = 0.5f;
	u64 difTime = 0;
	if (isTouchDown)
	{
		PPGraphics::Get()->DrawRectangle(x, y, w, h, colActive);
		tScale = 0.6f;
		
		if (holdTime == 0)
		{
			holdTime = osGetTime();
		}else
		{
			difTime = osGetTime() - holdTime;
		}
	}
	else
	{
		PPGraphics::Get()->DrawRectangle(x, y, w, h, colNormal);
	}
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, tScale, tScale);
	float startX = (w - tSize.x) / 2.0f;
	float startY = (h - tSize.y) / 2.0f;
	PPGraphics::Get()->DrawText(label, x + startX, y + startY, tScale, tScale, txtCol, false);

	
	return isTouchDown && difTime > 500 || TouchUpOnArea(x, y, w, h);
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
void PPUI::LabelBox(float x, float y, float w, float h, const char* label, ppColor bgColor, ppColor txtColor, float scale)
{
	PPGraphics::Get()->DrawRectangle(x, y, w, h, bgColor);
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, scale, scale);
	float startX = (w - tSize.x) / 2.0f;
	float startY = (h - tSize.y) / 2.0f;
	PPGraphics::Get()->DrawText(label, x + startX, y + startY, scale, scale, txtColor, false);

#ifdef UI_DEBUG
	char buffer[100];
	snprintf(buffer, sizeof buffer, "%f  %f", tSize.x, tSize.y);
	PPGraphics::Get()->DrawText(buffer, x , y - 10, 0.8f * scale, 0.8f * scale, txtColor, false);
#endif
}

void PPUI::LabelBoxLeft(float x, float y, float w, float h, const char* label, ppColor bgColor, ppColor txtColor, float scale)
{
	PPGraphics::Get()->DrawRectangle(x, y, w, h, bgColor);
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, scale, scale);
	float startY = (h - tSize.y) / 2.0f;
	PPGraphics::Get()->DrawText(label, x, y + startY, scale, scale, txtColor, false);

#ifdef UI_DEBUG
	char buffer[100];
	snprintf(buffer, sizeof buffer, "%f  %f", tSize.x, tSize.y);
	PPGraphics::Get()->DrawText(buffer, x, y - 10, 0.8f * scale, 0.8f * scale, txtColor, false);
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
