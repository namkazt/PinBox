#pragma once
#ifndef _PP_GRAPHICS_H_
#define _PP_GRAPHICS_H_

#include <3ds.h>
#include <3ds/gfx.h>
#include <citro3d.h>
#include <list>
#include <vector>

//=========================================================================================
// Const
//=========================================================================================

#define CLEAR_COLOR 0x000000FF
// Used to transfer the final rendered display to the framebuffer
#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

// Used to convert textures to 3DS tiled format
// Note: vertical flip flag set so 0,0 is top left of texture
#define TEXTURE_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

#define TEX_GRAPHIC 0
#define TEX_VIDEO_STREAM_LEFT 1
#define TEX_VIDEO_STREAM_RIGHT 2
#define TEX_FONT_GRAPHIC 3


//=========================================================================================
// Type
//=========================================================================================
typedef struct
{
	float x;
	float y;
}ppVector2;

typedef struct
{
	float x;
	float y;
	float z;
}ppVector3;

typedef struct
{
	u8 r;
	u8 g;
	u8 b;
	u8 a;
	u32 toU32() const { return (((a) & 0xFF) << 24) | (((b) & 0xFF) << 16) | (((g) & 0xFF) << 8) | (((r) & 0xFF) << 0); }
}ppColor;

typedef struct
{
	ppVector3 position;
	u32 color;
}ppVertexPosCol;

typedef struct
{
	ppVector3 position;
	ppVector2 textcoord;
}ppVertexPosTex;


//=========================================================================================
// Class
//=========================================================================================
class PPGraphics
{
private:
	C3D_RenderTarget*				mRenderTargetTop = nullptr;
	C3D_Mtx							mProjectionTop;
	C3D_RenderTarget*				mRenderTargetBtm = nullptr;
	C3D_Mtx							mProjectionBtm;

	DVLB_s*							mVShaderDVLB;
	shaderProgram_s					mShaderProgram;
	int								mULocProjection;

	// system font
	C3D_Tex*						mGlyphSheets;

	// Temporary memory pool
	void							*memoryPoolAddr = NULL;
	u32								memoryPoolIndex = 0;
	u32								memoryPoolSize = 0;

	gfxScreen_t						mCurrentDrawScreen = GFX_TOP;


	void setupForPosCollEnv(void* vertices);
	void setupForPosTexlEnv(void* vertices, u32 color, int texID);

	int getTextUnit(GPU_TEXUNIT unit);
	

	void *allocMemoryPoolAligned(u32 size, u32 alignment);
	void resetMemoryPool() { memoryPoolIndex = 0; }
	u32 getMemoryFreeSpace() const { return memoryPoolSize - memoryPoolIndex; };

public:
	static PPGraphics* Get();

	void GraphicsInit();
	void GraphicExit();

	// draw functions
	void BeginRender(gfxScreen_t screen);
	void EndRender();

	void DrawRectangle(float x, float y, float w, float h, ppColor color);
	void DrawText(const char* text, float x, float y, float scaleX, float scaleY, ppColor color, bool baseline);
};


#endif