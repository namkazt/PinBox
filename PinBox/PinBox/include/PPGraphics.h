#pragma once
#ifndef _PP_GRAPHICS_H_
#define _PP_GRAPHICS_H_

#include <3ds.h>
#include <3ds/gfx.h>
#include <citro3d.h>
//=========================================================================================
//=========================================================================================
#define CLEAR_COLOR 0x000000FF
// Used to transfer the final rendered display to the framebuffer
#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

// Used to convert textures to 3DS tiled format
// Note: vertical flip flag set so 0,0 is top left of texture
#define TEXTURE_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

#define FRAME_W 400
#define FRAME_H 240

//=================================================================================
// Init functions
//=================================================================================
void ppGraphicsInit();
void ppGraphicsRender();
void ppGraphicsExit();
void ppGraphicsDrawFrame(u8 *data, u32 size, u32 width, u32 height);


#endif