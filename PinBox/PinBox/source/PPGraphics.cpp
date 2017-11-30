#include "PPGraphics.h"
#include "vshader_shbin.h"

static C3D_RenderTarget* targetTop = nullptr;

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
	u8 strice;
	GPU_TEXCOLOR texCol;
	u8 *frameData;
	u32 frameSize;
	bool isInitTexture = false;
	static Frame2D* alloc(u32 id)
	{
		Frame2D* ret = (Frame2D*)linearAlloc(sizeof(Frame2D));
		if (ret == nullptr) linearFree(ret);
		else
		{
			ret->id = id;
		}
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
		this->texCol = GPU_RGB565;
		this->strice = 2;
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

DVLB_s* vshader_dvlb;
shaderProgram_s program;
int uLoc_projection;
C3D_Mtx projection;
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
}

void updateTexture()
{
	if(frame == nullptr)
	{
		//TODO: some log fail ?
		return;
	}
	//------------------------------------------------------
	// we need check if frame data is null or not
	// incase redraw we will not need to put data into GPU again and again
	//------------------------------------------------------
	if (frame->frameData != nullptr) {
		u32 frameSize = frame->width * frame->height;

		/* ---------------------------------------------------
		 * Reverse texture if need
		 * ---------------------------------------------------
		u8 *gpusrc = linearAlloc(w*h*4);
		// GX_DisplayTransfer needs input buffer in linear RAM
		u8* src=success; u8 *dst=gpusrc;
		// ? outputs big endian rgba so we need to convert
		for(int i = 0; i<w*h; i++) {
			int r = *src++;
			int g = *src++;
			int b = *src++;
			int a = *src++;

			*dst++ = a;
			*dst++ = b;
			*dst++ = g;
			*dst++ = r;
		}
		 */

		//------------------------------------------------------
		if(!frame->isInitTexture)
		{
			//--------------------------------------------------
			// init texture and transfer data into GPU
			frame->isInitTexture = true;
			GSPGPU_FlushDataCache(frame->frameData, frameSize *  frame->strice);
			C3D_TexInit(&frame->spriteTexture, frame->width, frame->height, frame->texCol);
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

			//--------------------------------------------------
			// Configure depth test to overwrite pixels with the same depth (needed to draw overlapping sprites)
			// in case we just render 2d image so we do not need depth test
			//C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
		}else
		{
			//--------------------------------------------------
			// incase this texture already inited so we do not need to re-init
			GSPGPU_FlushDataCache(frame->frameData, frameSize *  frame->strice);
			C3D_SafeDisplayTransfer((u32*)frame->frameData, GX_BUFFER_DIM(frame->width, frame->height), (u32*)frame->spriteTexture.data, GX_BUFFER_DIM(frame->width, frame->height), TEXTURE_TRANSFER_FLAGS);
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
	initFrameRenderTarget(GFX_LEFT, GPU_RB_RGB565);
}

void ppGraphicsRender()
{
	if (frame != nullptr)
	{
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C3D_FrameDrawOn(targetTop);

		// Update the uniforms
		C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
		//NOTE: should we draw fixed size or follow frame size ?
		drawFrameVBO(0, 0, 0, FRAME_W, FRAME_H);
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