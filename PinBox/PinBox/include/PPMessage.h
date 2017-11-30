#pragma once
#ifndef _PP_MESSAGE_H_
#define _PP_MESSAGE_H_

#include <3ds.h>


class PPMessage
{
private:
	//-----------------------------------------------
	// message header - 9 bytes
	// [4b] validate code : "PPBX"
	// [1b] message code : 0 to 255 - define message type
	// [4b] message content size 
	//-----------------------------------------------
	const char							g_validateCode[4] = { 'P','P','B','X' };
	u8									g_code = 0;
	u32									g_contentSize = 0;

	//-----------------------------------------------
	// message content
	//-----------------------------------------------
	void*								g_content = nullptr;
public:
	~PPMessage();
	u32			GetMessageSize() const { return g_contentSize + 9; }
	void*		GetMessageContent() { return g_content; }
	//-----------------------------------------------
	// NOTE: after using this message, returned data must be free whenever it sent.
	//-----------------------------------------------
	void*		BuildMessage(void* contentBuffer, u32 contentSize);
	
};

#endif