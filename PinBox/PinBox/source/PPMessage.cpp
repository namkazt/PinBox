#include "PPMessage.h"

PPMessage::~PPMessage()
{
	if (g_content != nullptr) 
		free(g_content);
}

u8* PPMessage::BuildMessage(u8* contentBuffer, u32 contentSize)
{
	g_contentSize = contentSize;
	//-----------------------------------------------
	// alloc msg buffer block
	u8* msgBuffer = (u8*)malloc(sizeof(u8) * (contentSize + 9));
	//-----------------------------------------------
	// build header
	u8* pointer = msgBuffer;
	// 1, validate code
	WRITE_CHAR_PTR(pointer, g_validateCode, 4);
	// 2, message code
	WRITE_U8(pointer, g_code);
	// 3, content size
	WRITE_U32(pointer, g_contentSize);
	//-----------------------------------------------
	// build content data
	if (g_contentSize > 0) {
		memcpy(msgBuffer + 9, contentBuffer, contentSize);
	}
	//-----------------------------------------------
	return msgBuffer;
}


u8* PPMessage::BuildMessageEmpty()
{
	return BuildMessage(nullptr, 0);
}

void PPMessage::BuildMessageHeader(u8 code)
{
	g_code = code;
}


bool PPMessage::ParseHeader(u8* buffer)
{
	if (IS_INVALID_CODE(buffer, 0))
	{
		printf("Parse header failed. Validate code is incorrect: %c%c%c%c \n", buffer[0], buffer[1], buffer[2], buffer[3]);
		return false;
	}
	//-----------------------------------------------------------
	size_t readIndex = 4;
	g_code = READ_U8(buffer, readIndex); readIndex += 1;
	g_contentSize = READ_U32(buffer, readIndex); readIndex += 4;
	return true;
}

void PPMessage::ClearHeader()
{
	g_code = 0;
	g_contentSize = 0;
}
