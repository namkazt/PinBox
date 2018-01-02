#include "PPGraphics.h"
#include "vshader_shbin.h"
#include <cstdio>
#include <cstdlib>


/*
// VBO entry
typedef struct {
	float x, y, z;
	float u, v;
} VBOEntry;

struct Frame2D {
	u32 id;
	C3D_Tex spriteTexture;
	u32 width;
	u32 height;
	u32 strice;
	GPU_TEXCOLOR texCol;
	u8 *frameData;
	u32 frameSize;
	bool isInitTexture = false;
	static Frame2D* alloc(u32 id)
	{
		Frame2D* ret = new Frame2D();
		ret->id = id;
		return ret;
	}

	void setFrameData(u8* data, u32 size, u32 w, u32 h)
	{
		// in case frame data is not relase yet so we free it
		this->freeFrameData();
		//---------------------------------------------
		// init frame data and copy from source
		this->frameData = (u8*)linearAlloc(sizeof(u8) * size);
		memcpy(this->frameData, data, size);
		//---------------------------------------------
		//NOTE: this frame data need to be free after use
		this->frameSize = size;
		this->width = w;
		this->height = h;
		//---------------------------------------------
		// it take 2 bit each pixel for RGB565
		this->texCol = GPU_RGB8;
		this->strice = 3;
	}

	void freeFrameData()
	{
		if (this->frameData != nullptr) {
			linearFree(this->frameData);
			this->frameSize = 0;
			this->frameData = nullptr;
		}
	}
};





VBOEntry *vbo;
Frame2D* frame;

void initFrameRenderTarget(gfx3dSide_t side, GPU_COLORBUF colorFmt)
{
	targetTop = C3D_RenderTargetCreate(FRAME_H, FRAME_W, colorFmt, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetClear(targetTop, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetSetOutput(targetTop, GFX_TOP, side, DISPLAY_TRANSFER_FLAGS);

	//---------------------------------------------------------------------
	// Load the vertex shader, create a shader program and bind it
	//---------------------------------------------------------------------
	vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
	C3D_BindProgram(&program);
	// Get the location of the uniforms
	uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
	// Allocate VBO for 1 texture only ( 2 tris = 2 * 3 )
	vbo = (VBOEntry*)linearAlloc(sizeof(VBOEntry) * 6 * 1);

	// Configure attributes for use with the vertex shader
	// Attribute format and element count are ignored in immediate mode
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v2=texcoord
												   // Compute the projection matrix
												   // Note: we're setting top to 240 here so origin is at top left.
	Mtx_OrthoTilt(&projection, 0.0, FRAME_W, FRAME_H, 0.0, 0.0, 1.0, true);
	// Configure buffers
	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vbo, sizeof(VBOEntry), 2, 0x10);

	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
}

void updateTexture()
{
	if(frame == nullptr)
	{
		printf("Frame is null ?\n");
		//TODO: some log fail ?
		return;
	}
	//------------------------------------------------------
	// we need check if frame data is null or not
	// incase redraw we will not need to put data into GPU again and again
	//------------------------------------------------------
	if (frame->frameData != nullptr) {
		u32 frameSize = frame->width * frame->height;
		//------------------------------------------------------
		if(!frame->isInitTexture)
		{
			printf("init frame texture : %d - %d\n", frame->width, frame->height);
			//--------------------------------------------------
			// init texture and transfer data into GPU
			frame->isInitTexture = true;

			GSPGPU_FlushDataCache(frame->frameData, frameSize *  frame->strice);
			C3D_TexInit(&frame->spriteTexture, frame->width, frame->height, GPU_RGB8);
			u32 dim = GX_BUFFER_DIM(frame->width, frame->height);
			C3D_SafeDisplayTransfer((u32*)frame->frameData, dim, (u32*)frame->spriteTexture.data, dim, TEXTURE_TRANSFER_FLAGS);
			gspWaitForPPF();


			C3D_TexSetFilter(&frame->spriteTexture, GPU_LINEAR, GPU_NEAREST);
			C3D_TexBind(frame->id, &frame->spriteTexture);
			//--------------------------------------------------
			// Configure the first fragment shading substage to just pass through the texture color
			// See https://www.opengl.org/sdk/docs/man2/xhtml/glTexEnv.xml for more insight
			C3D_TexEnv* env = C3D_GetTexEnv(frame->id);
			C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
			C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
			C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

		}else
		{
			//--------------------------------------------------
			// incase this texture already inited so we do not need to re-init
			GSPGPU_FlushDataCache(frame->frameData, frameSize *  frame->strice);
			u32 dim = GX_BUFFER_DIM(frame->width, frame->height);
			C3D_SafeDisplayTransfer((u32*)frame->frameData, dim, (u32*)frame->spriteTexture.data, dim, TEXTURE_TRANSFER_FLAGS);
			gspWaitForPPF();
		}
		//--------------------------------------------------
		// free texture data on this frame
		frame->freeFrameData();
	}
	
}


void drawFrameVBO(size_t idx, float x, float y, float width, float height)
{
	float left = 0.0f;
	float right = 1.0f;
	float top = 0.0f;
	float bottom = 1.0f;
	VBOEntry *entry = &vbo[idx * 6];

	*entry++ = (VBOEntry) { x, y, 0.5f, left, top };
	*entry++ = (VBOEntry) { x, y + height, 0.5f, left, bottom };
	*entry++ = (VBOEntry) { x + width, y, 0.5f, right, top };

	*entry++ = (VBOEntry) { x + width, y, 0.5f, right, top };
	*entry++ = (VBOEntry) { x, y + height, 0.5f, left, bottom };
	*entry++ = (VBOEntry) { x + width, y + height, 0.5f, right, bottom };
}


void ppGraphicsInit()
{
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	// we use RGB 565 for small frame size
	initFrameRenderTarget(GFX_LEFT, GPU_RB_RGB8);
	drawFrameVBO(0, 0, 0, FRAME_W, FRAME_H);
}

void ppGraphicsRender()
{
	if (frame != nullptr)
	{
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C3D_FrameDrawOn(targetTop);

		// Update the uniforms
		C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);	
		
		C3D_DrawArrays(GPU_TRIANGLES, 0, 6);
		C3D_FrameEnd(0);
	}
}

void ppGraphicsExit()
{
	// Free the shader program
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
	// Free the vbo
	if(vbo != nullptr) linearFree(vbo);
	if(frame != nullptr)
	{
		frame->freeFrameData();
		linearFree(frame);
	}
	//------------------------
	C3D_Fini();
	gfxExit();
}

void ppGraphicsDrawFrame(u8* data, u32 size, u32 width, u32 height)
{
	if (frame == nullptr) frame = Frame2D::alloc(0);
	frame->setFrameData(data, size, width, height);
	updateTexture();
}
*/

//---------------------------------------------------------------------
// Graphics Instance
//---------------------------------------------------------------------
static PPGraphics* mInstance = nullptr;

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
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	//---------------------------------------------------------------------
	// Setup render target
	//---------------------------------------------------------------------
	mRenderTargetTop = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetClear(mRenderTargetTop, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetSetOutput(mRenderTargetTop, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
	
	mRenderTargetBtm = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetClear(mRenderTargetBtm, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetSetOutput(mRenderTargetBtm, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	//---------------------------------------------------------------------
	// Setup top screen sprite ( for video render )
	//---------------------------------------------------------------------
	mTopScreenSprite = new ppSprite();

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
	Mtx_OrthoTilt(&mProjectionBtm, 0.0, 300, 240, 0.0, 0.0, 1.0, true);

	C3D_CullFace(GPU_CULL_NONE);
	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);


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
	if(mTopScreenSprite->initialized)
	{
		C3D_TexDelete(&mTopScreenSprite->spriteTexture);
	}
	delete mTopScreenSprite;
	linearFree(memoryPoolAddr);
	free(mGlyphSheets);
	shaderProgramFree(&mShaderProgram);
	DVLB_Free(mVShaderDVLB);
	C3D_Fini();
	gfxExit();
}

void PPGraphics::checkStartRendering()
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
		DrawRectangle(0, 0, 1, 1, ppColor{ 0,0,0,255 });
	}
	else {
		C3D_FrameDrawOn(mRenderTargetBtm);
		// set uniform projection
		C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, mULocProjection, &mProjectionBtm);
		// draw a transparent and small dot to avoid soft lock
		// @ref : https://github.com/fincs/citro3d/issues/35
		DrawRectangle(0, 0, 1, 1, ppColor{ 0,0,0,255 });
	}
}

void PPGraphics::EndRender()
{
	C3D_FrameEnd(0);
}

unsigned int next_pow2(unsigned int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v >= 64 ? v : 64;
}

void PPGraphics::UpdateTopScreenSprite(u8* data, u32 size, u32 width, u32 height)
{
	mTopScreenSprite->width = width;
	mTopScreenSprite->height = height;

	u8* linearData = (u8*)linearAlloc(sizeof(u8) * size);
	memcpy(linearData, data, size);

	if(!mTopScreenSprite->initialized)
	{
		mTopScreenSprite->initialized = true;
		GSPGPU_FlushDataCache(linearData, size);
		C3D_TexInit(&mTopScreenSprite->spriteTexture, width, height, GPU_RGB8);
		u32 dim = GX_BUFFER_DIM(width, height);
		C3D_SafeDisplayTransfer((u32*)linearData, dim, (u32*)mTopScreenSprite->spriteTexture.data, dim, TEXTURE_TRANSFER_FLAGS);
		gspWaitForPPF();
		C3D_TexSetFilter(&mTopScreenSprite->spriteTexture, GPU_LINEAR, GPU_NEAREST);
		C3D_TexSetWrap(&mTopScreenSprite->spriteTexture, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);
	}else
	{
		GSPGPU_FlushDataCache(linearData, size);
		u32 dim = GX_BUFFER_DIM(width, height);
		C3D_SafeDisplayTransfer((u32*)linearData, dim, (u32*)mTopScreenSprite->spriteTexture.data, dim, TEXTURE_TRANSFER_FLAGS);
		gspWaitForPPF();
	}
	linearFree(linearData);
	
}

void PPGraphics::DrawTopScreenSprite()
{
	if (!mTopScreenSprite->initialized) {
		return;
	}

	C3D_TexBind(getTextUnit(GPU_TEXUNIT0), &mTopScreenSprite->spriteTexture);
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2);
	ppVertexPosTex* vertices = (ppVertexPosTex*)allocMemoryPoolAligned(sizeof(ppVertexPosTex) * 4, 8);
	if (!vertices)
		return; // out of memory in pool

	float x = 0, y = 0, w = 400.0f, h = 240.0f;
	// set position
	vertices[0].position = (ppVector3) { x, y, 0.5f };
	vertices[1].position = (ppVector3) { x + w, y, 0.5f };
	vertices[2].position = (ppVector3) { x, y + h, 0.5f };
	vertices[3].position = (ppVector3) { x + w, y + h, 0.5f };

	// set color
	vertices[0].textcoord = (ppVector2) { 0.0f, 0.0f };
	vertices[1].textcoord = (ppVector2) { 1.0f, 0.0f };
	vertices[2].textcoord = (ppVector2) { 0.0f, 1.0f};
	vertices[3].textcoord = (ppVector2) { 1.0f, 1.0f };

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vertices, sizeof(ppVertexPosTex), 2, 0x10);
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
	BufInfo_Add(bufInfo, vertices, sizeof(ppVertexPosCol), 2, 0x10);
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
	BufInfo_Add(bufInfo, vertices, sizeof(ppVertexPosTex), 2, 0x10);
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

void PPGraphics::DrawRectangle(float x, float y, float w, float h, ppColor color)
{
	ppVertexPosCol* vertices = (ppVertexPosCol*)allocMemoryPoolAligned(sizeof(ppVertexPosCol) * 4, 8);
	if (!vertices) return;

	// set position
	vertices[0].position = (ppVector3) { x, y, 0.5f };
	vertices[1].position = (ppVector3) { x+w, y, 0.5f };
	vertices[2].position = (ppVector3) { x, y+h, 0.5f };
	vertices[3].position = (ppVector3) { x+w, y+h, 0.5f };

	// set color
	vertices[0].color = color.toU32();
	vertices[1].color = vertices[0].color;
	vertices[2].color = vertices[0].color;
	vertices[3].color = vertices[0].color;

	setupForPosCollEnv(vertices);

	C3D_DrawArrays(GPU_TRIANGLE_STRIP, 0, 4);
}

void PPGraphics::DrawText(const char* text, float x, float y, float scaleX, float scaleY, ppColor color, bool baseline)
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

			ppVertexPosTex* vertices = (ppVertexPosTex*)allocMemoryPoolAligned(sizeof(ppVertexPosTex) * 4, 8);
			if (!vertices) 
				break; // out of memory in pool

			// set position
			vertices[0].position = (ppVector3) { x + data.vtxcoord.left, y + data.vtxcoord.bottom, 0.5f };
			vertices[1].position = (ppVector3) { x + data.vtxcoord.right, y + data.vtxcoord.bottom, 0.5f };
			vertices[2].position = (ppVector3) { x + data.vtxcoord.left, y + data.vtxcoord.top, 0.5f };
			vertices[3].position = (ppVector3) { x + data.vtxcoord.right, y + data.vtxcoord.top, 0.5f };

			// set color
			vertices[0].textcoord = (ppVector2) { data.texcoord.left , data.texcoord.bottom };
			vertices[1].textcoord = (ppVector2) { data.texcoord.right, data.texcoord.bottom };
			vertices[2].textcoord = (ppVector2) { data.texcoord.left, data.texcoord.top };
			vertices[3].textcoord = (ppVector2) { data.texcoord.right, data.texcoord.top };

			setupForPosTexlEnv(vertices, color.toU32(), TEX_FONT_GRAPHIC);

			C3D_DrawArrays(GPU_TRIANGLE_STRIP, 0, 4);

			x += data.xAdvance;
		}

	} while (code > 0);
}
