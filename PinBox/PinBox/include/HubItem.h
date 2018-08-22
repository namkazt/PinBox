#ifndef _PP_HUB_TITEM_H_
#define _PP_HUB_TITEM_H_
#include <3ds.h>

class HubItem
{
public:
	const char*				uuid;

	// app name
	const char*				name;

	// thumb image should be 48x48 jpeg image
	u8*						thumbBuf;
	u32						thumbSize;
};

#endif