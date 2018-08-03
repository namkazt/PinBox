#pragma once
#ifndef _PP_MESSAGE_H_
#define _PP_MESSAGE_H_

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;

#define WRITE_CHAR_PTR(BUFFER, DATA, SIZE) memcpy(BUFFER, DATA, SIZE); BUFFER += SIZE;
#define WRITE_U8(BUFFER, DATA) *(BUFFER++) = DATA;
#define WRITE_U16(BUFFER, DATA) *(BUFFER++) = DATA; *(BUFFER++) = DATA >> 8;
#define WRITE_U32(BUFFER, DATA) *(BUFFER++) = DATA; *(BUFFER++) = DATA >> 8; *(BUFFER++) = DATA >> 16; *(BUFFER++) = DATA >> 24;
#define WRITE_U64(BUFFER, DATA) *(BUFFER++) = DATA; *(BUFFER++) = DATA >> 8; *(BUFFER++) = DATA >> 16; *(BUFFER++) = DATA >> 24;*(BUFFER++) = DATA >> 32;*(BUFFER++) = DATA >> 40;*(BUFFER++) = DATA >> 48;*(BUFFER++) = DATA >> 56;
#define READ_U8(BUFFER, INDEX) BUFFER[INDEX];
#define READ_U16(BUFFER, INDEX) BUFFER[INDEX] | BUFFER[INDEX + 1] << 8;
#define READ_U32(BUFFER, INDEX) BUFFER[INDEX] | BUFFER[INDEX + 1] << 8 | BUFFER[INDEX + 2] << 16 | BUFFER[INDEX + 3] << 24;
#define READ_U32(BUFFER, INDEX) BUFFER[INDEX] | BUFFER[INDEX + 1] << 8 | BUFFER[INDEX + 2] << 16 | BUFFER[INDEX + 3] << 24 | BUFFER[INDEX + 3] << 32 | BUFFER[INDEX + 3] << 40 | BUFFER[INDEX + 3] << 48 | BUFFER[INDEX + 3] << 56;
#define IS_INVALID_CODE(BUFFER, INDEX) BUFFER[INDEX] != 'P' || BUFFER[INDEX+1] != 'P' || BUFFER[INDEX+2] != 'B' || BUFFER[INDEX+3] != 'X'

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
	u8*									g_content = nullptr;
public:
	~PPMessage();
	u32									GetMessageSize() const { return g_contentSize + 9; }
	u8*									GetMessageContent() { return g_content; }
	u32									GetContentSize() { return g_contentSize; }
	u8									GetMessageCode() { return g_code; }
	//-----------------------------------------------
	// NOTE: after using this message, returned data must be free whenever it sent.
	//-----------------------------------------------
	u8*									BuildMessage(u8* contentBuffer, u32 contentSize);
	u8*									BuildMessageEmpty();
	void								BuildMessageHeader(u8 code);

	bool								ParseHeader(u8* buffer);
};

#endif