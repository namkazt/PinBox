#pragma once
#ifndef _PP_GRAPHICS_H_
#define _PP_GRAPHICS_H_

#include <3ds.h>
#include <3ds/gfx.h>
#include <citro3d.h>
#include "Color.h"
#include <map>

//=========================================================================================
// Const
//=========================================================================================

#define CLEAR_COLOR 0x000000FF

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

#define TEXTURE_RGBA_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
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
}Vector2;

typedef struct
{
	float x;
	float y;
	float z;
}Vector3;

typedef struct
{
	Vector3 position;
	u32 color;
}VertexPosCol;

typedef struct
{
	Vector3 position;
	Vector2 textcoord;
}VertexPosTex;

typedef struct 
{
	C3D_Tex tex;
	u32 width;
	u32 height;
	bool initialized = false;
}Sprite;

//=========================================================================================
// Class
//=========================================================================================
class PPGraphics
{
public:
	Color PrimaryColor = Color{ 76, 175, 80, 255 };
	Color PrimaryDarkColor = Color{ 0, 150, 136, 255 };
	Color AccentColor = Color{ 230, 126, 34, 255 };
	Color AccentDarkColor = Color{ 211, 84, 0, 255 };
	Color PrimaryTextColor = Color{ 38, 50, 56, 255 };
	Color AccentTextColor = Color{ 255, 255, 255, 255 };

	Color TransBackgroundDark = Color{3, 3, 3, 180};

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

	// top screen sprite
	Sprite*							mTopScreenSprite;

	// Temporary memory pool
	void							*memoryPoolAddr = NULL;
	u32								memoryPoolIndex = 0;
	u32								memoryPoolSize = 0;

	gfxScreen_t						mCurrentDrawScreen = GFX_TOP;
	int								mRendering = 0;

	// Texture cache
	std::map<std::string, Sprite*>	mTexCached;

	void setupForPosCollEnv(void* vertices);
	void setupForPosTexlEnv(void* vertices, u32 color, int texID);

	int getTextUnit(GPU_TEXUNIT unit);
	

	void *allocMemoryPoolAligned(u32 size, u32 alignment);
	void resetMemoryPool() { memoryPoolIndex = 0; }

public:
	~PPGraphics();
	static PPGraphics* Get();

	void GraphicsInit();
	void GraphicExit();

	// cache functions
	Sprite* AddCacheImageAsset(const char* name, std::string key);
	Sprite* AddCacheImage(const char* path, std::string key);
	Sprite* AddCacheImage(u8 *buf, u32 size, std::string key);
	Sprite* GetCacheImage(std::string key);

	// draw functions
	void BeginRender();
	void RenderOn(gfxScreen_t screen);
	void EndRender();

	void UpdateTopScreenSprite(u8* data, u32 size);
	void DrawTopScreenSprite();

	// Draw image
	void DrawImage(Sprite* sprite, int x, int y);
	void DrawImage(std::string key, int x, int y) { DrawImage(GetCacheImage(key), x, y); }
	void DrawImage(Sprite* sprite, int x, int y, int w, int h);
	void DrawImage(std::string key, int x, int y, int w, int h) { DrawImage(GetCacheImage(key), x, y, w, h); }
	void DrawImage(Sprite* sprite, int x, int y, int w, int h, int degrees);
	void DrawImage(std::string key, int x, int y, int w, int h, int degrees) { DrawImage(GetCacheImage(key), x, y, w, h, degrees); }
	void DrawImage(Sprite* sprite, int x, int y, int w, int h, int degrees, Vector2 anchor);
	void DrawImage(std::string key, int x, int y, int w, int h, int degrees, Vector2 anchor) { DrawImage(GetCacheImage(key), x, y, w, h, degrees, anchor); }

	// draw rectangle
	void DrawRectangle(float x, float y, float w, float h, Color color, float rounding = 0.0f);

	// mask
	void StartMasked(float x, float y, float w, float h, gfxScreen_t screen) const;
	void StopMasked() const;

	// draw text
	void DrawText(const char* text, float x, float y, float scaleX, float scaleY, Color color, bool baseline);
	void DrawTextAutoWrap(const char* text, float x, float y, float w, float scaleX, float scaleY, Color color, bool baseline);
	Vector2 GetTextSize(const char* text, float scaleX, float scaleY);
	Vector3 GetTextSizeAutoWrap(const char* text, float scaleX, float scaleY, float w);
};


#endif