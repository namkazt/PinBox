#include "PPGraphics.h"
#include "vshader_shbin.h"
#include <cstdio>
#include <cstdlib>
#include <3ds/gpu/enums.h>
#include <3ds/gfx.h>
#include <c3d/base.h>
#include <c3d/renderqueue.h>
#include <3ds/gpu/gx.h>
#include <3ds/allocator/linear.h>
#include <3ds/gpu/shaderProgram.h>
#include <citro3d.h>
#include <3ds/font.h>
#include <3ds/util/utf.h>
#include "lodepng.h"

#define IM_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR)/sizeof(*_ARR)))
//---------------------------------------------------------------------
// Graphics Instance
//---------------------------------------------------------------------
static PPGraphics* mInstance = nullptr;

static Vector2 mCircleVertex12[12];

#define TEX_MIN_SIZE 32
//Grabbed from: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
unsigned int next_pow2(unsigned int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v >= TEX_MIN_SIZE ? v : TEX_MIN_SIZE;
}

PPGraphics* PPGraphics::Get()
{
	if(mInstance == nullptr)
	{
		mInstance = new PPGraphics();
	}
	return mInstance;
}

void PPGraphics::GraphicsInit()
{
	// shared data
	int arrSize = IM_ARRAYSIZE(mCircleVertex12);
	for(int i = 0; i < arrSize; ++i)
	{
		const float a = ((float)i * 2 * M_PI) / (float)IM_ARRAYSIZE(mCircleVertex12);
		mCircleVertex12[i] = Vector2{ cosf(a), sinf(a) };
	}

	// init
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	//---------------------------------------------------------------------
	// Setup render target
	//---------------------------------------------------------------------
	mRenderTargetTop = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetClear(mRenderTargetTop, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetSetOutput(mRenderTargetTop, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
	
	mRenderTargetBtm = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetClear(mRenderTargetBtm, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetSetOutput(mRenderTargetBtm, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	//---------------------------------------------------------------------
	// Setup top screen sprite ( for video render )
	//---------------------------------------------------------------------
	mTopScreenSprite = new Sprite();

	//---------------------------------------------------------------------
	// Init memory pool
	// @Code borrow from libsf2d
	//---------------------------------------------------------------------
	memoryPoolAddr = linearAlloc(0x80000);
	memoryPoolSize = 0x80000;
	//---------------------------------------------------------------------
	// Load the vertex shader, create a shader program and bind it
	//---------------------------------------------------------------------
	mVShaderDVLB = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
	shaderProgramInit(&mShaderProgram);
	shaderProgramSetVsh(&mShaderProgram, &mVShaderDVLB->DVLE[0]);
	C3D_BindProgram(&mShaderProgram);

	mULocProjection = shaderInstanceGetUniformLocation(mShaderProgram.vertexShader, "projection");

	Mtx_OrthoTilt(&mProjectionTop, 0.0, 400, 240, 0.0, 0.0, 1.0, true);
	Mtx_OrthoTilt(&mProjectionBtm, 0.0, 320, 240, 0.0, 0.0, 1.0, true);

	C3D_CullFace(GPU_CULL_NONE);
	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
	

	//---------------------------------------------------------------------
	// Init top screen sprite
	//---------------------------------------------------------------------
	mTopScreenSprite->initialized = true;
	C3D_TexInit(&mTopScreenSprite->tex, 512, 256, GPU_RGB8);
	C3D_TexSetFilter(&mTopScreenSprite->tex, GPU_LINEAR, GPU_NEAREST);
	C3D_TexSetWrap(&mTopScreenSprite->tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);
	//---------------------------------------------------------------------
	// System font init
	//---------------------------------------------------------------------
	Result res = fontEnsureMapped();

	int i;
	TGLP_s* glyphInfo = fontGetGlyphInfo();
	mGlyphSheets = malloc(sizeof(C3D_Tex)*glyphInfo->nSheets);
	for (i = 0; i < glyphInfo->nSheets; i++)
	{
		C3D_Tex* tex = &mGlyphSheets[i];
		tex->data = fontGetGlyphSheetTex(i);
		tex->fmt = glyphInfo->sheetFmt;
		tex->size = glyphInfo->sheetSize;
		tex->width = glyphInfo->sheetWidth;
		tex->height = glyphInfo->sheetHeight;
		tex->param = GPU_TEXTURE_MAG_FILTER(GPU_LINEAR) | GPU_TEXTURE_MIN_FILTER(GPU_LINEAR)
			| GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_EDGE) | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_EDGE);
		tex->border = 0;
		tex->lodParam = 0;
	}
}

void PPGraphics::GraphicExit()
{
	// clean up cache
	for(auto it = mTexCached.begin(); it != mTexCached.end(); ++it) {
		if(it->second->initialized) C3D_TexDelete(&it->second->tex);
		delete it->second;
	}
	mTexCached.clear();
	// delete top
	if(mTopScreenSprite->initialized)
		C3D_TexDelete(&mTopScreenSprite->tex);
	delete mTopScreenSprite;
	linearFree(memoryPoolAddr);
	free(mGlyphSheets);
	shaderProgramFree(&mShaderProgram);
	DVLB_Free(mVShaderDVLB);
	C3D_Fini();
	gfxExit();
}

Sprite* PPGraphics::AddCacheImageAsset(const char* name, std::string key)
{
	auto iter = mTexCached.find(key);
	if (iter != mTexCached.end()) {
		return iter->second;
	}

	// get image path
	char path[100];
	sprintf(path, "romfs:/assets/%s", name);
	return AddCacheImage(path, key);
}

Sprite* PPGraphics::AddCacheImage(const char* path, std::string key)
{
	auto iter = mTexCached.find(key);
	if (iter != mTexCached.end()) {
		return iter->second;
	}

	unsigned char* image;
	unsigned width, height;

	int ret = lodepng_decode32_file(&image, &width, &height, path);
	if (ret != 0)
	{
		printf("Failed when decode asset image: %s with key: %s\n", path, key.c_str());
		return nullptr;
	}
	
	u8 *gpusrc = linearAlloc(width*height * 4);
	u8* src = image; u8 *dst = gpusrc;
	// lodepng outputs big endian rgba so we need to convert
	for (int i = 0; i<width*height; i++) {
		int r = *src++;
		int g = *src++;
		int b = *src++;
		int a = *src++;
		*dst++ = a;
		*dst++ = b;
		*dst++ = g;
		*dst++ = r;
	}

	Sprite* sprite = new Sprite();
	// init texture
	bool success = C3D_TexInit(&sprite->tex, next_pow2(width), next_pow2(height), GPU_RGBA8);
	if (!success)
	{
		free(sprite);
		return nullptr;
	}
	sprite->width = width;
	sprite->height = height;
	C3D_TexSetWrap(&sprite->tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);

	printf("Added sprite: %s from path: %s\n", key.c_str(), path);

	GSPGPU_FlushDataCache(gpusrc, width*height * 4);
	C3D_SyncDisplayTransfer((u32*)gpusrc, GX_BUFFER_DIM(width, height), (u32*)sprite->tex.data, GX_BUFFER_DIM(sprite->tex.width, sprite->tex.height), TEXTURE_RGBA_TRANSFER_FLAGS);

	free(image);
	linearFree(gpusrc);

	mTexCached[key] = sprite;

	return sprite;
}

Sprite* PPGraphics::AddCacheImage(u8* buf, u32 size, std::string key)
{
	auto iter = mTexCached.find(key);
	if (iter != mTexCached.end()) {
		return iter->second;
	}
	
	// load data
	unsigned char* image;
	unsigned width, height;
	int ret = lodepng_decode32(&image, &width, &height, buf, size);
	if(ret != 0)
	{
		printf("Failed when decode png image with key: %s\n", key.c_str());
		return nullptr;
	}
	
	u8 *gpusrc = linearAlloc(width*height * 4);
	u8* src = image; u8 *dst = gpusrc;
	// lodepng outputs big endian rgba so we need to convert
	for (int i = 0; i<width*height; i++) {
		int r = *src++;
		int g = *src++;
		int b = *src++;
		int a = *src++;
		*dst++ = a;
		*dst++ = b;
		*dst++ = g;
		*dst++ = r;
	}

	Sprite* sprite = new Sprite();
	// init texture
	bool success = C3D_TexInit(&sprite->tex, next_pow2(width), next_pow2(height), GPU_RGBA8);
	if(!success)
	{
		delete sprite;
		return nullptr;
	}
	sprite->width = width;
	sprite->height = height;
	C3D_TexSetWrap(&sprite->tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);

	printf("added key: %s - size: %d\n", key.c_str(), size);

	GSPGPU_FlushDataCache(gpusrc, width*height * 4);
	C3D_SyncDisplayTransfer((u32*)gpusrc, GX_BUFFER_DIM(width, height), (u32*)sprite->tex.data, GX_BUFFER_DIM(sprite->tex.width, sprite->tex.height), TEXTURE_RGBA_TRANSFER_FLAGS);

	free(image);
	linearFree(gpusrc);

	mTexCached[key] = sprite;

	return sprite;
}

Sprite* PPGraphics::GetCacheImage(std::string key)
{
	auto iter = mTexCached.find(key);
	if (iter != mTexCached.end()) {
		return iter->second;
	}
	printf("Image cache not found: %s\n", key.c_str());
	return nullptr;
}

PPGraphics::~PPGraphics()
{

}

void PPGraphics::BeginRender()
{
	resetMemoryPool();
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
}

void PPGraphics::RenderOn(gfxScreen_t screen)
{
	mCurrentDrawScreen = screen;
	if (screen == GFX_TOP)
	{
		C3D_FrameDrawOn(mRenderTargetTop);
		// set uniform projection
		C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, mULocProjection, &mProjectionTop);
		// draw a transparent and small dot to avoid soft lock
		// @ref : https://github.com/fincs/citro3d/issues/35
		DrawRectangle(0, 0, 1, 1, Color{ 0,0,0,255 });
	}
	else {
		C3D_FrameDrawOn(mRenderTargetBtm);
		// set uniform projection
		C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, mULocProjection, &mProjectionBtm);
		// draw a transparent and small dot to avoid soft lock
		// @ref : https://github.com/fincs/citro3d/issues/35
		DrawRectangle(0, 0, 1, 1, Color{ 0,0,0,255 });
	}
}

void PPGraphics::EndRender()
{
	C3D_FrameEnd(0);
}

void PPGraphics::UpdateTopScreenSprite(u8* data, u32 size)
{
	if (!mTopScreenSprite->initialized) return;
	memcpy(mTopScreenSprite->tex.data, data, size);
	GSPGPU_FlushDataCache(mTopScreenSprite->tex.data, size);
}

void PPGraphics::DrawTopScreenSprite()
{
	if (!mTopScreenSprite->initialized) return;

	float x = 0, y = 0, w = 400.0f, h = 240.0f;
	float ow = 512.0f, oh = 256.0f;
	float u = 1;
	float v = 1;

	VertexPosTex *vertices = (VertexPosTex*)allocMemoryPoolAligned(sizeof(VertexPosTex) * 4, 8);
	
	// set position
	vertices[0].position = (Vector3) { 0, 0, 0.5f };
	vertices[1].position = (Vector3) { 512.0f, 0, 0.5f };
	vertices[2].position = (Vector3) { 0, 256.0f, 0.5f };
	vertices[3].position = (Vector3) { 512.0f, 256.0f, 0.5f };
	vertices[0].textcoord = (Vector2) { 0.0f, 1.0f };
	vertices[1].textcoord = (Vector2) { 1.0f, 1.0f};
	vertices[2].textcoord = (Vector2) { 0.0f, 0.0f };
	vertices[3].textcoord = (Vector2) { 1.0f, 0.0f};
	
	// setup env
	C3D_TexBind(getTextUnit(GPU_TEXUNIT0), &mTopScreenSprite->tex);
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2);

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vertices, sizeof(VertexPosTex), 2, 0x10);

	C3D_DrawArrays(GPU_TRIANGLE_STRIP, 0, 4);
}

void PPGraphics::setupForPosCollEnv(void* vertices)
{
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);
	AttrInfo_AddLoader(attrInfo, 1, GPU_UNSIGNED_BYTE, 4);

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vertices, sizeof(VertexPosCol), 2, 0x10);
}

void PPGraphics::setupForPosTexlEnv(void* vertices, u32 color, int texID)
{
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, 0, 0);
	C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_CONSTANT, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
	C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
	C3D_TexEnvColor(env, color);

	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2);

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vertices, sizeof(VertexPosTex), 2, 0x10);
}

int PPGraphics::getTextUnit(GPU_TEXUNIT unit)
{
	switch (unit) {
	case GPU_TEXUNIT0: return 0;
	case GPU_TEXUNIT1: return 1;
	case GPU_TEXUNIT2: return 2;
	default:           return -1;
	}
}

void* PPGraphics::allocMemoryPoolAligned(u32 size, u32 alignment)
{
	u32 new_index = (memoryPoolIndex + alignment - 1) & ~(alignment - 1);
	if ((new_index + size) < memoryPoolSize) {
		void *addr = (void *)((u32)memoryPoolAddr + new_index);
		memoryPoolIndex = new_index + size;
		return addr;
	}
	return NULL;
}

void PPGraphics::DrawImage(Sprite* sprite, int x, int y)
{
	DrawImage(sprite, x, y, sprite->width, sprite->height);
}

void PPGraphics::DrawImage(Sprite* sprite, int x, int y, int w, int h)
{
	DrawImage(sprite, x, y, w, h, 0);
}

void PPGraphics::DrawImage(Sprite* sprite, int x, int y, int w, int h, int degrees)
{
	DrawImage(sprite, x, y, w, h, degrees, Vector2{ 0.5f, 0.5f });
}

void PPGraphics::DrawImage(Sprite* sprite, int x, int y, int w, int h, int degrees, Vector2 anchor)
{
	if (!sprite) return;
	C3D_TexFlush(&sprite->tex);

	// bind texture
	C3D_TexBind(getTextUnit(GPU_TEXUNIT0), &sprite->tex);
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);


	VertexPosTex *vertices = (VertexPosTex*)allocMemoryPoolAligned(sizeof(VertexPosTex) * 4, 8);
	if (!vertices) return;

	// set position
	vertices[0].position = (Vector3) { x, y, 0.5f };
	vertices[1].position = (Vector3) { w + x, y, 0.5f };
	vertices[2].position = (Vector3) { x, h + y, 0.5f };
	vertices[3].position = (Vector3) { w + x, h + y, 0.5f };

	float u = sprite->width / (float)sprite->tex.width;
	float v = sprite->width / (float)sprite->tex.height;

	// set uv
	vertices[0].textcoord = (Vector2) { 0.0f, 0.0f };
	vertices[1].textcoord = (Vector2) { u, 0.0f };
	vertices[2].textcoord = (Vector2) { 0.0f, v };
	vertices[3].textcoord = (Vector2) { u, v };

	// rotate if need
	if (degrees != 0)
	{
		float rad = (float)degrees * M_PI / 180.f;
		const float c = cosf(rad);
		const float s = sinf(rad);
		float cx = x + w * anchor.x;
		float cy = y + h * anchor.y;
		int i;
		for (i = 0; i < 4; ++i) { // Rotate and translate
			float _x = cx - vertices[i].position.x;
			float _y = cy - vertices[i].position.y;
			vertices[i].position.x = _x*c - _y*s + cx;
			vertices[i].position.y = _x*s + _y*c + cy;
		}
	}

	// setup attributes
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2);

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vertices, sizeof(VertexPosTex), 2, 0x10);

	C3D_DrawArrays(GPU_TRIANGLE_STRIP, 0, 4);
}

void PPGraphics::DrawRectangle(float x, float y, float w, float h, Color color, float rounding)
{
	u32 col = color.toU32();

	if(rounding > 0.0f)
	{
		VertexPosCol* vertices = (VertexPosCol*)allocMemoryPoolAligned(sizeof(VertexPosCol) * 66, 8);
		if (!vertices) return;
		int vIndex = -1;
		Vector3 outters[8];
		int outterIdx = -1;

		Vector3 tl = Vector3{ x + rounding, y + rounding, 0.5f };
		Vector3 tr = Vector3{ x + w - rounding, y + rounding, 0.5f };
		Vector3 bl = Vector3{ x + rounding, y + h - rounding, 0.5f };
		Vector3 br = Vector3{ x + w - rounding, y + h - rounding, 0.5f };

		// main rect
		vertices[++vIndex] = VertexPosCol { tl , col };			// top-left
		vertices[++vIndex] = VertexPosCol { tr , col };			// top-right
		vertices[++vIndex] = VertexPosCol { bl , col };			// bottom-left

		vertices[++vIndex] = VertexPosCol { tr , col };			// top-right
		vertices[++vIndex] = VertexPosCol { bl , col };			// bottom-left
		vertices[++vIndex] = VertexPosCol { br , col };			// bottom-right

		// arc path func
		// tl : 6, 9     tr : 9, 12     br : 0, 3    bl : 3, 6
		const std::function<void(Vector3 center, int min, int max)> arcVertices = [&](Vector3 center, int min, int max)
		{
			Vector3 result[4];
			int i = -1;
			for (int a = min; a <= max; ++a)
			{
				const Vector2& v = mCircleVertex12[a % IM_ARRAYSIZE(mCircleVertex12)];
				result[++i] = Vector3{ center.x + v.x * rounding, center.y + v.y * rounding, 0.5f };
			}
			// add vertices
			vertices[++vIndex] = VertexPosCol{ center , col };
			vertices[++vIndex] = VertexPosCol{ result[0] , col };
			vertices[++vIndex] = VertexPosCol{ result[1] , col };

			vertices[++vIndex] = VertexPosCol{ center , col };
			vertices[++vIndex] = VertexPosCol{ result[1] , col };
			vertices[++vIndex] = VertexPosCol{ result[2] , col };

			vertices[++vIndex] = VertexPosCol{ center , col };
			vertices[++vIndex] = VertexPosCol{ result[2] , col };
			vertices[++vIndex] = VertexPosCol{ result[3] , col };

			outters[++outterIdx] = result[0];
			outters[++outterIdx] = result[3];
		};

		arcVertices(tl, 6, 9);
		arcVertices(tr, 9, 12);
		arcVertices(br, 0, 3);
		arcVertices(bl, 3, 6);

		// outer rect
		vertices[++vIndex] = VertexPosCol{ tl , col };			
		vertices[++vIndex] = VertexPosCol{ tr , col };			
		vertices[++vIndex] = VertexPosCol{ outters[1] , col };		
		vertices[++vIndex] = VertexPosCol{ tr , col };		
		vertices[++vIndex] = VertexPosCol{ outters[1] , col };			
		vertices[++vIndex] = VertexPosCol{ outters[2] , col };		

		vertices[++vIndex] = VertexPosCol{ tr , col };
		vertices[++vIndex] = VertexPosCol{ br , col };
		vertices[++vIndex] = VertexPosCol{ outters[3] , col };
		vertices[++vIndex] = VertexPosCol{ outters[3] , col };
		vertices[++vIndex] = VertexPosCol{ br , col };
		vertices[++vIndex] = VertexPosCol{ outters[4] , col };

		vertices[++vIndex] = VertexPosCol{ bl , col };
		vertices[++vIndex] = VertexPosCol{ br , col };
		vertices[++vIndex] = VertexPosCol{ outters[5] , col };
		vertices[++vIndex] = VertexPosCol{ bl , col };
		vertices[++vIndex] = VertexPosCol{ outters[5] , col };
		vertices[++vIndex] = VertexPosCol{ outters[6] , col };

		vertices[++vIndex] = VertexPosCol{ tl , col };
		vertices[++vIndex] = VertexPosCol{ bl , col };
		vertices[++vIndex] = VertexPosCol{ outters[0] , col };
		vertices[++vIndex] = VertexPosCol{ bl , col };
		vertices[++vIndex] = VertexPosCol{ outters[0] , col };
		vertices[++vIndex] = VertexPosCol{ outters[7] , col };

		setupForPosCollEnv(vertices);

		C3D_DrawArrays(GPU_TRIANGLES, 0, 66);
	}else
	{
		VertexPosCol* vertices = (VertexPosCol*)allocMemoryPoolAligned(sizeof(VertexPosCol) * 4, 8);
		if (!vertices) return;
		u32 vIndex = 0;

		vertices[vIndex] = VertexPosCol{ Vector3{ x, y, 0.5f } , col };
		vertices[++vIndex] = VertexPosCol{ Vector3{ x + w, y, 0.5f } , col };
		vertices[++vIndex] = VertexPosCol{ Vector3{ x, y + h, 0.5f } , col };
		vertices[++vIndex] = VertexPosCol{ Vector3{ x + w, y + h, 0.5f } , col };

		setupForPosCollEnv(vertices);

		C3D_DrawArrays(GPU_TRIANGLE_STRIP, 0, 4);
	}
}

void PPGraphics::StartMasked(float x, float y, float w, float h, gfxScreen_t screen) const
{
	if(screen == GFX_TOP) C3D_SetScissor(GPU_SCISSOR_NORMAL, 240 - (y + h), 400 - (x + w), 240 - y, 400 - x);
	else C3D_SetScissor(GPU_SCISSOR_NORMAL, 240 - (y + h), 320 - (x + w), 240 - y, 320 - x);
	
}

void PPGraphics::StopMasked() const
{
	C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);
}

void PPGraphics::DrawText(const char* text, float x, float y, float scaleX, float scaleY, Color color, bool baseline)
{
	ssize_t  units;
	u32 code;

	const u8* p = (const u8*)text;
	float firstX = x;
	u32 flags = GLYPH_POS_CALC_VTXCOORD | (baseline ? GLYPH_POS_AT_BASELINE : 0);
	int lastSheet = -1;
	do
	{
		if(!*p) break;
		units = decode_utf8(&code, p);
		if(units == -1)
			break;

		p += units;
		if( code == '\n')
		{
			x = firstX;
			y += scaleY * fontGetInfo()->lineFeed;
		}else if(code > 0)
		{
			int glyphIdx = fontGlyphIndexFromCodePoint(code);
			fontGlyphPos_s data;
			fontCalcGlyphPos(&data, glyphIdx, flags, scaleX, scaleY);

			// Bind the correct texture sheet
			if (data.sheetIndex != lastSheet)
			{
				lastSheet = data.sheetIndex;
				C3D_TexBind(getTextUnit(GPU_TEXUNIT0), &mGlyphSheets[lastSheet]);
			}

			VertexPosTex* vertices = (VertexPosTex*)allocMemoryPoolAligned(sizeof(VertexPosTex) * 4, 8);
			if (!vertices) 
				break; // out of memory in pool

			// set position
			vertices[0].position = (Vector3) { x + data.vtxcoord.left, y + data.vtxcoord.bottom, 0.5f };
			vertices[1].position = (Vector3) { x + data.vtxcoord.right, y + data.vtxcoord.bottom, 0.5f };
			vertices[2].position = (Vector3) { x + data.vtxcoord.left, y + data.vtxcoord.top, 0.5f };
			vertices[3].position = (Vector3) { x + data.vtxcoord.right, y + data.vtxcoord.top, 0.5f };

			// set uv
			vertices[0].textcoord = (Vector2) { data.texcoord.left , data.texcoord.bottom };
			vertices[1].textcoord = (Vector2) { data.texcoord.right, data.texcoord.bottom };
			vertices[2].textcoord = (Vector2) { data.texcoord.left, data.texcoord.top };
			vertices[3].textcoord = (Vector2) { data.texcoord.right, data.texcoord.top };

			setupForPosTexlEnv(vertices, color.toU32(), TEX_FONT_GRAPHIC);

			C3D_DrawArrays(GPU_TRIANGLE_STRIP, 0, 4);

			x += data.xAdvance;
		}

	} while (code > 0);
}

void PPGraphics::DrawTextAutoWrap(const char* text, float x, float y, float w, float scaleX, float scaleY,
	Color color, bool baseline)
{
	ssize_t  units;
	u32 code;

	const u8* p = (const u8*)text;
	float firstX = x;
	u32 flags = GLYPH_POS_CALC_VTXCOORD | (baseline ? GLYPH_POS_AT_BASELINE : 0);
	int lastSheet = -1;
	do
	{
		if (!*p) break;
		units = decode_utf8(&code, p);
		if (units == -1)
			break;

		p += units;

		if (code == '\n')
		{
			x = firstX;
			y += scaleY * fontGetInfo()->lineFeed;
		}
		else if (code > 0)
		{
			int glyphIdx = fontGlyphIndexFromCodePoint(code);
			fontGlyphPos_s data;
			fontCalcGlyphPos(&data, glyphIdx, flags, scaleX, scaleY);
			
			// Bind the correct texture sheet
			if (data.sheetIndex != lastSheet)
			{
				lastSheet = data.sheetIndex;
				C3D_TexBind(getTextUnit(GPU_TEXUNIT0), &mGlyphSheets[lastSheet]);
			}

			VertexPosTex* vertices = (VertexPosTex*)allocMemoryPoolAligned(sizeof(VertexPosTex) * 4, 8);
			if (!vertices)
				break; // out of memory in pool

					   // set position
			vertices[0].position = (Vector3) { x + data.vtxcoord.left, y + data.vtxcoord.bottom, 0.5f };
			vertices[1].position = (Vector3) { x + data.vtxcoord.right, y + data.vtxcoord.bottom, 0.5f };
			vertices[2].position = (Vector3) { x + data.vtxcoord.left, y + data.vtxcoord.top, 0.5f };
			vertices[3].position = (Vector3) { x + data.vtxcoord.right, y + data.vtxcoord.top, 0.5f };

			// set uv
			vertices[0].textcoord = (Vector2) { data.texcoord.left, data.texcoord.bottom };
			vertices[1].textcoord = (Vector2) { data.texcoord.right, data.texcoord.bottom };
			vertices[2].textcoord = (Vector2) { data.texcoord.left, data.texcoord.top };
			vertices[3].textcoord = (Vector2) { data.texcoord.right, data.texcoord.top };

			setupForPosTexlEnv(vertices, color.toU32(), TEX_FONT_GRAPHIC);

			C3D_DrawArrays(GPU_TRIANGLE_STRIP, 0, 4);

			// this line is seem to over
			if (x + data.width > w)
			{
				x = firstX;
				y += scaleY * fontGetInfo()->lineFeed;
			}
			else
			{
				x += data.xAdvance;
			}
		}

	} while (code > 0);
}


Vector2 PPGraphics::GetTextSize(const char* text, float scaleX, float scaleY)
{
	ssize_t  units;
	u32 code;

	Vector2 result;
	float maxW = 0, cW = 0;

	const u8* p = (const u8*)text;
	u32 flags = GLYPH_POS_CALC_VTXCOORD | 0;
	result.y = scaleY * fontGetInfo()->lineFeed;
	do
	{
		if (!*p) break;
		units = decode_utf8(&code, p);
		if (units == -1)
			break;
		p += units;
		if (code == '\n')
		{
			if(cW < maxW) cW = maxW;
			maxW = 0;
			result.y += scaleY * fontGetInfo()->lineFeed;
		}else if (code > 0)
		{
			int glyphIdx = fontGlyphIndexFromCodePoint(code);
			fontGlyphPos_s data;
			fontCalcGlyphPos(&data, glyphIdx, flags, scaleX, scaleY);
			maxW += data.xAdvance;
		}

	} while (code > 0);
	if (cW < maxW) cW = maxW;
	result.x = cW;

	return result;
}

Vector3 PPGraphics::GetTextSizeAutoWrap(const char* text, float scaleX, float scaleY, float w)
{
	ssize_t  units;
	u32 code;

	Vector3 result;
	float maxW = 0;
	float padding = 2;

	const u8* p = (const u8*)text;
	u32 flags = GLYPH_POS_CALC_VTXCOORD | 0;
	result.x = 0;
	result.z = 1; // lines
	result.y = scaleY * fontGetInfo()->lineFeed + padding; // height
	do
	{
		if (!*p) break;
		units = decode_utf8(&code, p);
		if (units == -1)
			break;
		p += units;
		if (code == '\n')
		{
			if (result.x < maxW) result.x = maxW;
			maxW = 0;
			result.y += scaleY * fontGetInfo()->lineFeed + padding;
			result.z += 1;
		}
		else if (code > 0)
		{
			int glyphIdx = fontGlyphIndexFromCodePoint(code);
			fontGlyphPos_s data;
			fontCalcGlyphPos(&data, glyphIdx, flags, scaleX, scaleY);
			if (maxW + data.width > w)
			{
				if (result.x < maxW) result.x = maxW;
				maxW = 0;
				result.y += scaleY * fontGetInfo()->lineFeed + padding;
				result.z += 1;
			}
			else maxW += data.xAdvance;
		}

	} while (code > 0);
	if (result.x < maxW) result.x = maxW;

	return result;
}
