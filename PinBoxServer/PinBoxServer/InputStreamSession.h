#pragma once
#ifndef _PP_INPUT_STREAM_SESSION_H_
#define _PP_INPUT_STREAM_SESSION_H_

#define BIT(n) (1U<<(n))
#include <key.hpp>
#include <map>
#include "AudioStreamSession.h"
#include "ViGEmClient.h"

/// Key values.
enum
{
	KEY_A = BIT(0),       ///< A
	KEY_B = BIT(1),       ///< B
	KEY_SELECT = BIT(2),       ///< Select
	KEY_START = BIT(3),       ///< Start
	KEY_DRIGHT = BIT(4),       ///< D-Pad Right
	KEY_DLEFT = BIT(5),       ///< D-Pad Left
	KEY_DUP = BIT(6),       ///< D-Pad Up
	KEY_DDOWN = BIT(7),       ///< D-Pad Down
	KEY_R = BIT(8),       ///< R
	KEY_L = BIT(9),       ///< L
	KEY_X = BIT(10),      ///< X
	KEY_Y = BIT(11),      ///< Y
	KEY_ZL = BIT(14),      ///< ZL (New 3DS only)
	KEY_ZR = BIT(15),      ///< ZR (New 3DS only)
	KEY_TOUCH = BIT(20),      ///< Touch (Not actually provided by HID)
	KEY_CSTICK_RIGHT = BIT(24), ///< C-Stick Right (New 3DS only)
	KEY_CSTICK_LEFT = BIT(25), ///< C-Stick Left (New 3DS only)
	KEY_CSTICK_UP = BIT(26), ///< C-Stick Up (New 3DS only)
	KEY_CSTICK_DOWN = BIT(27), ///< C-Stick Down (New 3DS only)
	KEY_CPAD_RIGHT = BIT(28),   ///< Circle Pad Right
	KEY_CPAD_LEFT = BIT(29),   ///< Circle Pad Left
	KEY_CPAD_UP = BIT(30),   ///< Circle Pad Up
	KEY_CPAD_DOWN = BIT(31),   ///< Circle Pad Down

	// Generic catch-all directions
	KEY_UP = KEY_DUP | KEY_CPAD_UP,    ///< D-Pad Up or Circle Pad Up
	KEY_DOWN = KEY_DDOWN | KEY_CPAD_DOWN,  ///< D-Pad Down or Circle Pad Down
	KEY_LEFT = KEY_DLEFT | KEY_CPAD_LEFT,  ///< D-Pad Left or Circle Pad Left
	KEY_RIGHT = KEY_DRIGHT | KEY_CPAD_RIGHT, ///< D-Pad Right or Circle Pad Right
};
struct KeyMappingProfile
{
	std::string								name = "";
	std::map<uint8_t, FakeInput::Key>		mappings;
	std::map<uint8_t, _XUSB_BUTTON>			controller;
	bool									circlePadAsMouse = false;
	bool									ZLRAsMouseButton = false;
	std::string								type = "";
};

class InputStreamSession
{
private:
	const int									c_cpadDeadZone = 15;
	const int									c_cpadMin = 0;
	const int									c_cpadMax = 156;

	u32											m_OldDown;
	u32											m_OldUp;
	short										m_OldCX;
	short										m_OldCY;
	short										m_OldCTX;
	short										m_OldCTY;
	bool										m_receivedInput = false;

	std::string									m_currentProfile = "Default";
	KeyMappingProfile*							m_defaultProfile;
	std::map<std::string, KeyMappingProfile*>	m_keyMappingProfiles;
	std::map<std::string, FakeInput::Key>		m_keyMapping;
	void initMapKey();


	void initVIGEM();
	bool m_Xbox360Enable = false;
	PVIGEM_CLIENT m_vDriver;
	// xbox 360 controller
	PVIGEM_TARGET m_x360Controller;
	XUSB_REPORT m_x360Report;

public:
	InputStreamSession();
	~InputStreamSession();

	void LoadInputConfig();

	void UpdateInput(u32 down, u32 up, short cx, short cy, short ctx, short cty);
	void ProcessInput();
	void ChangeInputProfile(std::string	profileName);
};

#endif
