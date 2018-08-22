#ifndef _PP_HUB_TITEM_H_
#define _PP_HUB_TITEM_H_
#include "PPMessage.h"

enum HubItemType
{
	HUB_SCREEN = 0x0,
	HUB_APP,
	HUB_MOVIE,
};

class HubItem
{
public:
	std::string				uuid;

	// app name
	std::string				name;

	// thumb image should be 64x64 png image
	u8*						thumbBuf;
	u32						thumbSize;

	HubItemType				type;


	// data
	std::string				thumbImage;
	std::string				exePath;
	std::string				processName;
};

#endif