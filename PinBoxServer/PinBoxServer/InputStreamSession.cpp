#include "stdafx.h"
#include "InputStreamSession.h"
#include <fakeinput.hpp>
#include <libconfig.h++>
#include <iostream>

InputStreamSession::InputStreamSession()
{
	initMapKey();
	LoadInputConfig();
	initVIGEM();
}


InputStreamSession::~InputStreamSession()
{
}

void InputStreamSession::LoadInputConfig()
{
	//-----------------------------------------------
	// Init default mapping
	//-----------------------------------------------
	m_defaultProfile = new KeyMappingProfile();
	m_defaultProfile->name = "Default";
	//m_defaultProfile->type = "keyboard";
	m_defaultProfile->type = "x360";
	m_defaultProfile->mappings[0] = FakeInput::Key_Z;
	m_defaultProfile->mappings[1] = FakeInput::Key_X;
	m_defaultProfile->mappings[10] = FakeInput::Key_A;
	m_defaultProfile->mappings[11] = FakeInput::Key_S;
	m_defaultProfile->mappings[9] = FakeInput::Key_Q;
	m_defaultProfile->mappings[8] = FakeInput::Key_W;
	m_defaultProfile->mappings[6] = FakeInput::Key_Right;
	m_defaultProfile->mappings[7] = FakeInput::Key_Down;
	m_defaultProfile->mappings[5] = FakeInput::Key_Left;
	m_defaultProfile->mappings[4] = FakeInput::Key_Up;
	m_defaultProfile->mappings[3] = FakeInput::Key_N;
	m_defaultProfile->mappings[2] = FakeInput::Key_M;
	m_defaultProfile->circlePadAsMouse = false;
	// circle pad act as dpad
	m_defaultProfile->mappings[30] = FakeInput::Key_Up;
	m_defaultProfile->mappings[31] = FakeInput::Key_Down;
	m_defaultProfile->mappings[29] = FakeInput::Key_Left;
	m_defaultProfile->mappings[28] = FakeInput::Key_Right;
	// for new3ds only
	m_defaultProfile->mappings[14] = FakeInput::Key_E;
	m_defaultProfile->mappings[15] = FakeInput::Key_D;
	m_keyMappingProfiles[m_defaultProfile->name] = m_defaultProfile;

	// x360
	m_defaultProfile->controller[0] = XUSB_GAMEPAD_A;
	m_defaultProfile->controller[1] = XUSB_GAMEPAD_B;
	m_defaultProfile->controller[10] = XUSB_GAMEPAD_X;
	m_defaultProfile->controller[11] = XUSB_GAMEPAD_Y;
	m_defaultProfile->controller[9] = XUSB_GAMEPAD_LEFT_SHOULDER;
	m_defaultProfile->controller[8] = XUSB_GAMEPAD_RIGHT_SHOULDER;
	m_defaultProfile->controller[6] = XUSB_GAMEPAD_DPAD_UP;
	m_defaultProfile->controller[7] = XUSB_GAMEPAD_DPAD_DOWN;
	m_defaultProfile->controller[5] = XUSB_GAMEPAD_DPAD_LEFT;
	m_defaultProfile->controller[4] = XUSB_GAMEPAD_DPAD_RIGHT;
	m_defaultProfile->controller[3] = XUSB_GAMEPAD_START;
	m_defaultProfile->controller[2] = XUSB_GAMEPAD_BACK;

	//-----------------------------------------------
	// load config
	//-----------------------------------------------
	libconfig::Config inputConfigFile;
	try
	{
		inputConfigFile.readFile("input.cfg");
	}catch(const libconfig::FileIOException& fioex)
	{
		std::cout << "[Error] Input config file was not found." << std::endl << std::flush;
	}
	catch (const libconfig::ParseException& pex)
	{
		std::cout << "[Error] Input config file corrupted." << std::endl << std::flush;
	}

	// load input profiles
	const libconfig::Setting& root = inputConfigFile.getRoot();
	const libconfig::Setting& inputProfiles = root["input_profiles"];
	int profilesCount = inputProfiles.getLength();
	for (int i = 0; i < profilesCount; ++i)
	{
		KeyMappingProfile* profile = new KeyMappingProfile();

		std::string name, type;

		if(!inputProfiles[i].lookupValue("name", name) && 
			inputProfiles[i].lookupValue("type", type))
		{
			continue;
		}

		profile->name = name;
		profile->type = type;

		if(type == "keyboard")
		{
			std::string btn_A, btn_B, btn_X, btn_Y;
			std::string btn_DPAD_UP, btn_DPAD_DOWN, btn_DPAD_LEFT, btn_DPAD_RIGHT;
			std::string btn_L, btn_R, btn_ZL, btn_ZR;
			std::string btn_START, btn_SELECT;
			bool circle_as_mouse, zl_zr_as_mouse_btn;

			if (!inputProfiles[i].lookupValue("btn_A", btn_A) &&
				inputProfiles[i].lookupValue("btn_B", btn_B) &&
				inputProfiles[i].lookupValue("btn_X", btn_X) &&
				inputProfiles[i].lookupValue("btn_Y", btn_Y) &&
				inputProfiles[i].lookupValue("btn_DPAD_UP", btn_DPAD_UP) &&
				inputProfiles[i].lookupValue("btn_DPAD_DOWN", btn_DPAD_DOWN) &&
				inputProfiles[i].lookupValue("btn_DPAD_LEFT", btn_DPAD_LEFT) &&
				inputProfiles[i].lookupValue("btn_DPAD_RIGHT", btn_DPAD_RIGHT) &&
				inputProfiles[i].lookupValue("btn_L", btn_L) &&
				inputProfiles[i].lookupValue("btn_R", btn_R) &&
				inputProfiles[i].lookupValue("btn_START", btn_START) &&
				inputProfiles[i].lookupValue("btn_SELECT", btn_SELECT))
			{
			continue;
			}

			profile->mappings[0] = m_keyMapping[btn_A];
			profile->mappings[1] = m_keyMapping[btn_B];
			profile->mappings[10] = m_keyMapping[btn_X];
			profile->mappings[11] = m_keyMapping[btn_Y];
			profile->mappings[9] = m_keyMapping[btn_L];
			profile->mappings[8] = m_keyMapping[btn_R];
			profile->mappings[14] = m_keyMapping[btn_ZL];
			profile->mappings[15] = m_keyMapping[btn_ZR];
			profile->mappings[6] = m_keyMapping[btn_DPAD_RIGHT];
			profile->mappings[7] = m_keyMapping[btn_DPAD_DOWN];
			profile->mappings[5] = m_keyMapping[btn_DPAD_LEFT];
			profile->mappings[4] = m_keyMapping[btn_DPAD_UP];
			profile->mappings[3] = m_keyMapping[btn_START];
			profile->mappings[2] = m_keyMapping[btn_SELECT];

			// optional
			// load for zl and zr
			if (inputProfiles[i].lookupValue("circle_pad_as_mouse", circle_as_mouse))
			{
				inputProfiles[i].lookupValue("zl_zr_as_mouse_button", zl_zr_as_mouse_btn);
				profile->circlePadAsMouse = circle_as_mouse;
				profile->ZLRAsMouseButton = zl_zr_as_mouse_btn;
			}
			// optional
			// load for zl and zr
			if (!zl_zr_as_mouse_btn) {
				if (inputProfiles[i].lookupValue("btn_ZL", btn_ZL)) profile->mappings[14] = m_keyMapping[btn_ZL];
				if (inputProfiles[i].lookupValue("btn_ZR", btn_ZR)) profile->mappings[15] = m_keyMapping[btn_ZR];
			}

			m_keyMappingProfiles[profile->name] = profile;
		}else if(type == "x360")
		{
			profile->controller[0] = XUSB_GAMEPAD_A;
			profile->controller[1] = XUSB_GAMEPAD_B;
			profile->controller[10] = XUSB_GAMEPAD_X;
			profile->controller[11] = XUSB_GAMEPAD_Y;
			profile->controller[9] = XUSB_GAMEPAD_LEFT_SHOULDER;
			profile->controller[8] = XUSB_GAMEPAD_RIGHT_SHOULDER;
			profile->controller[6] = XUSB_GAMEPAD_DPAD_UP;
			profile->controller[7] = XUSB_GAMEPAD_DPAD_DOWN;
			profile->controller[5] = XUSB_GAMEPAD_DPAD_LEFT;
			profile->controller[4] = XUSB_GAMEPAD_DPAD_RIGHT;
			profile->controller[3] = XUSB_GAMEPAD_START;
			profile->controller[2] = XUSB_GAMEPAD_BACK;
		}
		
	}
}

void InputStreamSession::UpdateInput(uint32_t down, uint32_t up, short cx, short cy, short ctx, short cty)
{
	m_OldDown = down;
	m_OldUp = up;
	m_OldCX = cx;
	m_OldCY = cy;
	m_OldCTX = ctx;
	m_OldCTY = cty;
	/*std::cout << "Input Down:" << down << 
		" - Up:" << up << 
		" - CX:" << cx <<
		" - CY:" << cy <<
		" - CTX:" << ctx << 
		" - CTY:" << cty << std::endl << std::flush;*/
	ProcessInput();
}
void InputStreamSession::ProcessInput()
{
	// validate profile
	if (m_currentProfile.empty()) m_currentProfile = "Default";
	if (m_keyMappingProfiles.find(m_currentProfile) == m_keyMappingProfiles.end()) m_currentProfile = "Default";

	KeyMappingProfile *profile = m_keyMappingProfiles[m_currentProfile];

	if(profile->type == "keyboard")
	{
		// Key Input
		for (uint8_t i = 0; i < 32; ++i)
		{
			if (m_OldDown & BIT(i))
			{
				if (profile->mappings.find(i) != profile->mappings.end())
				{
					FakeInput::Keyboard::pressKey(profile->mappings[i]);
				}
			}else
			{
				if (profile->mappings.find(i) != profile->mappings.end())
				{
					FakeInput::Keyboard::releaseKey(profile->mappings[i]);
				}
			}
		}

		if (profile->circlePadAsMouse)
		{
			// Circle Pad
			short _cx = (c_cpadMax - m_OldCX);
			short _cy = (c_cpadMax - m_OldCY);
			FakeInput::Mouse::move(_cx, _cy);
		}
	}else if(profile->type == "x360")
	{
		m_x360Report.wButtons = 0;
		m_x360Report.bLeftTrigger = 0;
		m_x360Report.bRightTrigger = 0;
		// Key Input
		for (uint8_t i = 0; i < 32; ++i)
		{
			if (m_OldDown & BIT(i))
			{
				if (profile->controller.find(i) != profile->controller.end())
				{
					m_x360Report.wButtons |= profile->controller[i];
				}
			}
		}

		// ZL
		if (m_OldDown & BIT(14))
		{
			m_x360Report.bLeftTrigger = 250;
		}
		if(m_OldUp & BIT(14))
		{
			m_x360Report.bLeftTrigger = 0;
		}

		// ZR
		if (m_OldDown & BIT(15))
		{
			m_x360Report.bRightTrigger = 250;
		}
		if (m_OldUp & BIT(15))
		{
			m_x360Report.bRightTrigger = 0;
		}

		const short MAX_STICK_RANGE = 30000;

		// Left stick
		if (m_OldCX > 0 && m_OldCX < c_cpadDeadZone) m_OldCX = 0;
		if (m_OldCY > 0 && m_OldCY < c_cpadDeadZone) m_OldCY = 0;
		if (m_OldCX < 0 && m_OldCX > -c_cpadDeadZone) m_OldCX = 0;
		if (m_OldCY < 0 && m_OldCY > -c_cpadDeadZone) m_OldCY = 0;

		float pCx = (float)m_OldCX / (float)c_cpadMax;
		float pCy = (float)m_OldCY / (float)c_cpadMax;
		
		m_x360Report.sThumbLX = (short)(pCx * MAX_STICK_RANGE);
		if (m_x360Report.sThumbLX > MAX_STICK_RANGE) m_x360Report.sThumbLX = MAX_STICK_RANGE;
		if (m_x360Report.sThumbLX < -MAX_STICK_RANGE) m_x360Report.sThumbLX = -MAX_STICK_RANGE;
		m_x360Report.sThumbLY = (short)(pCy * MAX_STICK_RANGE);
		if (m_x360Report.sThumbLY > MAX_STICK_RANGE) m_x360Report.sThumbLY = MAX_STICK_RANGE;
		if (m_x360Report.sThumbLY < -MAX_STICK_RANGE) m_x360Report.sThumbLY = -MAX_STICK_RANGE;

		// Right stick
		if (m_OldCTX > 0 && m_OldCTX < c_cpadDeadZone) m_OldCTX = 0;
		if (m_OldCTY > 0 && m_OldCTY < c_cpadDeadZone) m_OldCTY = 0;
		if (m_OldCTX < 0 && m_OldCTX > -c_cpadDeadZone) m_OldCTX = 0;
		if (m_OldCTY < 0 && m_OldCTY > -c_cpadDeadZone) m_OldCTY = 0;

		float pCtx = ((float)m_OldCTX / (float)c_cpadMax) + 0.3f;
		float pCty = ((float)m_OldCTY / (float)c_cpadMax) + 0.3f;
		
		int newCTX = (int)(pCtx * MAX_STICK_RANGE);
		int newCTY = (int)(pCty * MAX_STICK_RANGE);
		if (newCTX > MAX_STICK_RANGE) newCTX = MAX_STICK_RANGE;
		if (newCTX < -MAX_STICK_RANGE) newCTX = -MAX_STICK_RANGE;
		if (newCTY > MAX_STICK_RANGE) newCTY = MAX_STICK_RANGE;
		if (newCTY < -MAX_STICK_RANGE) newCTY = -MAX_STICK_RANGE;

		m_x360Report.sThumbRX = newCTX;
		m_x360Report.sThumbRY = newCTY;

		// update
		if(!VIGEM_SUCCESS(vigem_target_x360_update(m_vDriver, m_x360Controller, m_x360Report)))
		{
			std::cout << "[Error] Error when submit report update to X360." << std::endl << std::flush;
		}

		//std::cout << "[DEBUG] LStick: " << m_x360Report.sThumbLX << " : " << m_x360Report.sThumbLY << " RStick: " << m_x360Report.sThumbRX << " : " << m_x360Report.sThumbRX  << std::endl << std::flush;

		// reset
		//m_OldDown = 0;
		m_OldUp = 0;
	}
	
}

void InputStreamSession::ChangeInputProfile(std::string profileName)
{

}

void InputStreamSession::initMapKey()
{
	m_keyMapping["A"] = FakeInput::Key_A;
	m_keyMapping["B"] = FakeInput::Key_B;
	m_keyMapping["C"] = FakeInput::Key_C;
	m_keyMapping["D"] = FakeInput::Key_D;
	m_keyMapping["E"] = FakeInput::Key_E;
	m_keyMapping["F"] = FakeInput::Key_F;
	m_keyMapping["G"] = FakeInput::Key_G;
	m_keyMapping["H"] = FakeInput::Key_H;
	m_keyMapping["I"] = FakeInput::Key_I;
	m_keyMapping["J"] = FakeInput::Key_J;
	m_keyMapping["K"] = FakeInput::Key_K;
	m_keyMapping["L"] = FakeInput::Key_L;
	m_keyMapping["M"] = FakeInput::Key_M;
	m_keyMapping["N"] = FakeInput::Key_N;
	m_keyMapping["O"] = FakeInput::Key_O;
	m_keyMapping["P"] = FakeInput::Key_P;
	m_keyMapping["Q"] = FakeInput::Key_Q;
	m_keyMapping["R"] = FakeInput::Key_R;
	m_keyMapping["S"] = FakeInput::Key_S;
	m_keyMapping["T"] = FakeInput::Key_T;
	m_keyMapping["U"] = FakeInput::Key_U;
	m_keyMapping["V"] = FakeInput::Key_V;
	m_keyMapping["W"] = FakeInput::Key_W;
	m_keyMapping["X"] = FakeInput::Key_X;
	m_keyMapping["Y"] = FakeInput::Key_Y;
	m_keyMapping["Z"] = FakeInput::Key_Z;
	m_keyMapping["0"] = FakeInput::Key_0;
	m_keyMapping["1"] = FakeInput::Key_1;
	m_keyMapping["2"] = FakeInput::Key_2;
	m_keyMapping["3"] = FakeInput::Key_3;
	m_keyMapping["4"] = FakeInput::Key_4;
	m_keyMapping["5"] = FakeInput::Key_5;
	m_keyMapping["6"] = FakeInput::Key_6;
	m_keyMapping["7"] = FakeInput::Key_7;
	m_keyMapping["8"] = FakeInput::Key_8;
	m_keyMapping["9"] = FakeInput::Key_9;
	m_keyMapping["F1"] = FakeInput::Key_F1;
	m_keyMapping["F2"] = FakeInput::Key_F2;
	m_keyMapping["F3"] = FakeInput::Key_F3;
	m_keyMapping["F4"] = FakeInput::Key_F4;
	m_keyMapping["F5"] = FakeInput::Key_F5;
	m_keyMapping["F6"] = FakeInput::Key_F6;
	m_keyMapping["F7"] = FakeInput::Key_F7;
	m_keyMapping["F8"] = FakeInput::Key_F8;
	m_keyMapping["F9"] = FakeInput::Key_F9;
	m_keyMapping["F10"] = FakeInput::Key_F10;
	m_keyMapping["F11"] = FakeInput::Key_F11;
	m_keyMapping["F12"] = FakeInput::Key_F12;
	m_keyMapping["Escape"] = FakeInput::Key_Escape;
	m_keyMapping["Space"] = FakeInput::Key_Space;
	m_keyMapping["Return"] = FakeInput::Key_Return;
	m_keyMapping["Backspace"] = FakeInput::Key_Backspace;
	m_keyMapping["Tab"] = FakeInput::Key_Tab;
	m_keyMapping["Shift_L"] = FakeInput::Key_Shift_L;
	m_keyMapping["Shift_R"] = FakeInput::Key_Shift_R;
	m_keyMapping["Control_L"] = FakeInput::Key_Control_L;
	m_keyMapping["Control_R"] = FakeInput::Key_Control_R;
	m_keyMapping["Alt_L"] = FakeInput::Key_Alt_L;
	m_keyMapping["Alt_R"] = FakeInput::Key_Alt_R;
	m_keyMapping["CapsLock"] = FakeInput::Key_CapsLock;
	m_keyMapping["NumLock"] = FakeInput::Key_NumLock;
	m_keyMapping["ScrollLock"] = FakeInput::Key_ScrollLock;
	m_keyMapping["PrintScreen"] = FakeInput::Key_PrintScreen;
	m_keyMapping["Insert"] = FakeInput::Key_Insert;
	m_keyMapping["Delete"] = FakeInput::Key_Delete;
	m_keyMapping["PageUP"] = FakeInput::Key_PageUP;
	m_keyMapping["PageDown"] = FakeInput::Key_PageDown;
	m_keyMapping["Home"] = FakeInput::Key_Home;
	m_keyMapping["End"] = FakeInput::Key_End;
	m_keyMapping["Left"] = FakeInput::Key_Left;
	m_keyMapping["Right"] = FakeInput::Key_Right;
	m_keyMapping["Up"] = FakeInput::Key_Up;
	m_keyMapping["Down"] = FakeInput::Key_Down;
	m_keyMapping["Numpad0"] = FakeInput::Key_Numpad0;
	m_keyMapping["Numpad1"] = FakeInput::Key_Numpad1;
	m_keyMapping["Numpad2"] = FakeInput::Key_Numpad2;
	m_keyMapping["Numpad3"] = FakeInput::Key_Numpad3;
	m_keyMapping["Numpad4"] = FakeInput::Key_Numpad4;
	m_keyMapping["Numpad5"] = FakeInput::Key_Numpad5;
	m_keyMapping["Numpad6"] = FakeInput::Key_Numpad6;
	m_keyMapping["Numpad7"] = FakeInput::Key_Numpad7;
	m_keyMapping["Numpad8"] = FakeInput::Key_Numpad8;
	m_keyMapping["Numpad9"] = FakeInput::Key_Numpad9;
	m_keyMapping["NumpadAdd"] = FakeInput::Key_NumpadAdd;
	m_keyMapping["NumpadSubtract"] = FakeInput::Key_NumpadSubtract;
	m_keyMapping["NumpadMultiply"] = FakeInput::Key_NumpadMultiply;
	m_keyMapping["NumpadDivide"] = FakeInput::Key_NumpadDivide;
	m_keyMapping["NumpadDecimal"] = FakeInput::Key_NumpadDecimal;
	m_keyMapping["NumpadEnter"] = FakeInput::Key_NumpadEnter;
}


void InputStreamSession::initVIGEM()
{
	m_Xbox360Enable = false;
	//
	// Allocate driver connection object
	// 
	m_vDriver = vigem_alloc();

	//
	// Attempt to connect to bus driver
	// 
	if(!VIGEM_SUCCESS(vigem_connect(m_vDriver)))
	{
		std::cout << "[Error] Can't connect to Virtual Controller." << std::endl << std::flush;
		std::cout << "-> Please ask on Discord for more information: https://discordapp.com/channels/340110838947905538/340110838947905538." << std::endl << std::flush;
		return;
	}
	std::cout << "[X360] Connected successfully." << std::endl << std::flush;
	//
	// Allocate target device object of type Xbox 360 Controller
	// 
	m_x360Controller = vigem_target_x360_alloc();

	//
	// Add new Xbox 360 device to bus.
	// 
	// This call blocks until the device reached working state 
	// or an error occurred.
	// 
	if (!VIGEM_SUCCESS(vigem_target_add(m_vDriver, m_x360Controller)))
	{
		std::cout << "[Error] Couldn't add virtual X360 device." << std::endl << std::flush;
		return;
	}
	std::cout << "[X360] Added virtual x360 device successfully." << std::endl << std::flush;

	XUSB_REPORT_INIT(&m_x360Report);
	m_Xbox360Enable = true;
}