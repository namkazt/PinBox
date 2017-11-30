#include "PPMessage.h"
#include <cstring>

PPMessage::~PPMessage()
{
	if (g_content != nullptr) 
		linearFree(g_content);
}

void* PPMessage::BuildMessage(void* contentBuffer, u32 contentSize)
{
	g_contentSize = contentSize;
	//-----------------------------------------------
	// alloc msg buffer block
	u8* msgBuffer = (u8*)linearAlloc(sizeof(u8) * (contentSize + 9));
	u32 pointer = 0;
	//-----------------------------------------------
	// build header
	// 1, validate code
	memcpy(msgBuffer, &g_validateCode, 4);
	pointer += 4;
	// 2, message code
	memcpy(msgBuffer + pointer, &g_code, 1);
	pointer += 1;
	// 3, content size
	memcpy(msgBuffer + pointer, &g_contentSize, 4);
	pointer += 4;
	//-----------------------------------------------
	// build content data
	memcpy(msgBuffer + pointer, contentBuffer, contentSize);
	//-----------------------------------------------
	return msgBuffer;
}
