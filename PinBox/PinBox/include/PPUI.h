#pragma once
#ifndef _PP_UI_H_
#define _PP_UI_H_
#include <3ds.h>
#include <string>
#include <citro3d.h>
#include "PPGraphics.h"
#include "PPSessionManager.h"
#include "easing.h"
#include "Color.h"

//#define UI_DEBUG 1
#define RET_CLOSE_APP -1000

#define SCROLL_THRESHOLD 5
#define SCROLL_BAR_MIN 0.2f
#define SCROLL_SPEED_MODIFIED 0.2f
#define SCROLL_SPEED_STEP 0.18f

enum Direction
{
	D_NONE = 0,
	D_HORIZONTAL = 1,
	D_VERTICAL = 2,
	D_BOTH = 3
};

typedef struct DialogBoxOverride {
	bool isActivate = false;
	Color TitleBgColor;
	Color TitleTextColor;
	const char* Title = nullptr;
	const char* Body = nullptr;
};

typedef struct WH {
	int width;
	int height;
};

typedef std::function<void(float x, float y, float w, float h)> TabContentDraw;
typedef std::function<int()> PopupCallback;
typedef std::function<WH()> WHCallback;
typedef std::function<void(void* arg1, void* arg2)> ResultCallback;

class PPUI
{

public:
	static u32 getKeyDown();
	static u32 getKeyHold();
	static u32 getKeyUp();
	static circlePosition getLeftCircle();
	static circlePosition getRightCircle();

	static u32 getSleepModeState();

	static void UpdateInput();
	static bool TouchDownOnArea(float x, float y, float w, float h);
	static bool TouchUpOnArea(float x, float y, float w, float h);
	static bool TouchDown();
	static bool TouchMove();
	static bool TouchUp();


	// RESOURCES
	static void InitResource();
	static void CleanupResource();

	// SCREEN
	static int DrawIdleTopScreen(PPSessionManager *sessionManager);


	static int DrawBtmServerSelectScreen(PPSessionManager *sessionManager);
	static int DrawBtmAddNewServerProfileScreen(PPSessionManager *sessionManager, ResultCallback cancel, ResultCallback ok);
	static int DrawBtmPairedScreen(PPSessionManager *sessionManager);

	static int DrawStreamConfigUI(PPSessionManager *sessionManager, ResultCallback cancel, ResultCallback ok);
	static int DrawIdleBottomScreen(PPSessionManager *sessionManager);

	static void InfoBox(PPSessionManager *sessionManager);

	// TAB
	static int DrawTabs(const char* tabs[], u32 tabCount, int activeTab, float x, float y, float w, float h);

	// DIALOG
	static void OverrideDialogTypeWarning();
	static void OverrideDialogTypeInfo();
	static void OverrideDialogTypeSuccess();
	static void OverrideDialogTypeCritical();

	static void OverrideDialogContent(const char* title, const char* body);

	static int DrawDialogKeyboard( ResultCallback cancelCallback, ResultCallback okCallback);
	static int DrawDialogNumberInput( ResultCallback cancelCallback, ResultCallback okCallback);
	static int DrawDialogLoading(const char* title, const char* body, PopupCallback callback);
	static int DrawDialogMessage(PPSessionManager *sessionManager, const char* title, const char* body);
	static int DrawDialogMessage(PPSessionManager *sessionManager, const char* title, const char* body, PopupCallback closeCallback);
	static int DrawDialogMessage(PPSessionManager *sessionManager, const char* title, const char* body, PopupCallback cancelCallback, PopupCallback okCallback);

	static int DrawDialogBox(PPSessionManager *sessionManager);


	// SCROLL BOX
	static Vector2 ScrollBox(float x, float y, float w, float h, Direction dir, Vector2 cursor, WHCallback contentDraw);

	// SLIDE
	static float Slide(float x, float y, float w, float h, float val, float min, float max, float step, const char* label);
	
	// CHECKBOX
	static bool ToggleBox(float x, float y, float w, float h, bool value, const char* label);

	// SELECT BOX
	static bool SelectBox(float x, float y, float w, float h, Color color, float rounding);

	// BUTTON
	static bool FlatButton(float x, float y, float w, float h, const char* label, float rounding = 0.0f);
	static bool FlatDarkButton(float x, float y, float w, float h, const char* label, float rounding = 0.0f);
	static bool FlatColorButton(float x, float y, float w, float h, const char* label, Color colNormal, Color colActive, Color txtCol, float rounding = 0.0f);

	static bool RepeatButton(float x, float y, float w, float h, const char* label, Color colNormal, Color colActive, Color txtCol);

	// TEXT
	static int LabelBox(float x, float y, float w, float h, const char* label, Color bgColor, Color txtColor, float scale = 0.5f, float rounding = 0.f);
	static int LabelBoxAutoWrap(float x, float y, float w, float h, const char* label, Color bgColor, Color txtColor, float scale = 0.5f, float rounding = 0.f);
	static int LabelBoxLeft(float x, float y, float w, float h, const char* label, Color bgColor, Color txtColor, float scale = 0.5f, float rounding = 0.f);

	// POPUP
	static bool HasPopup();
	static PopupCallback GetPopup();
	static void ClosePopup();
	static void AddPopup(PopupCallback callback);

};

#endif