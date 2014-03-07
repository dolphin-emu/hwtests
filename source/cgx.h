// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

// "Custom GX"
// Replacement library for accessing the GPU
// libogc's GX contains bugs and uses internal state.
// Both of these things are not particularly good for a test suite.
// Hence, this file provides an alternative set of functions, which
// are roughly based on GX, but are supposed to use no internal state.
// They are based directly on Dolphin's register definitions, hence
// (hopefully) minimizing potential for mistakes.

#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <ogc/system.h>

#include "CommonTypes.h"
#include "BPMemory.h"
#include "CPMemory.h"
#include "XFMemory.h"

#pragma once

typedef float f32;

typedef union
{
	volatile u8 U8;
	volatile s8 S8;
	volatile u16 U16;
	volatile s16 S16;
	volatile u32 U32;
	volatile s32 S32;
	volatile f32 F32;
} CWGPipe;

//static CWGPipe* const wgPipe = (CWGPipe*)0xCC008000;

// TODO: Get rid of these definitions!
//struct GXFifoObj;
extern "C"
{
GXFifoObj* GX_Init(void* base, u32 size);
}

#define CGX_LOAD_BP_REG(x) \
	do { \
		wgPipe->U8 = 0x61; \
		wgPipe->U32 = (u32)(x); \
	} while(0)

#define CGX_LOAD_CP_REG(x, y) \
	do { \
		wgPipe->U8 = 0x08; \
		wgPipe->U8 = (u8)(x); \
		wgPipe->U32 = (u32)(y); \
	} while(0)

#define CGX_BEGIN_LOAD_XF_REGS(x, n) \
	do { \
		wgPipe->U8 = 0x10; \
		wgPipe->U32 = (u32)(((((n)&0xffff)-1)<<16)|((x)&0xffff)); \
	} while(0)

void CGX_Init()
{
	// TODO: Is this leaking memory?
    void *gp_fifo = NULL;
    gp_fifo = memalign(32, 256*1024);
    memset(gp_fifo, 0, 256*1024);

	GX_Init(gp_fifo, 256*1024);
}

void CGX_SetViewport(float origin_x, float origin_y, float width, float height, float near, f32 far)
{
	CGX_BEGIN_LOAD_XF_REGS(0x101a,6);
	wgPipe->F32 = width*0.5;
	wgPipe->F32 = -height*0.5;
	wgPipe->F32 = (far-near)*16777215.0;
	wgPipe->F32 = 342.0+origin_x+width*0.5;
	wgPipe->F32 = 342.0+origin_y+height*0.5;
	wgPipe->F32 = far*16777215.0;	
}

static inline void WriteMtxPS4x2(register f32 mt[3][4], register void* wgpipe)
{
	// Untested
	register f32 tmp0, tmp1, tmp2, tmp3;

	__asm__ __volatile__
		("psq_l %0,0(%4),0,0\n\
		psq_l %1,8(%4),0,0\n\
		psq_l %2,16(%4),0,0\n\
		psq_l %3,24(%4),0,0\n\
		psq_st %0,0(%5),0,0\n\
		psq_st %1,0(%5),0,0\n\
		psq_st %2,0(%5),0,0\n\
		psq_st %3,0(%5),0,0"
		: "=&f"(tmp0),"=&f"(tmp1),"=&f"(tmp2),"=&f"(tmp3)
		: "b"(mt), "b"(wgpipe)
		: "memory"
	);
}

void CGX_LoadPosMatrixDirect(f32 mt[3][4], u32 index)
{
	// Untested
	CGX_BEGIN_LOAD_XF_REGS((index<<2)&0xFF, 12);
	WriteMtxPS4x2(mt, (void*)wgPipe);
}

void CGX_LoadProjectionMatrixPerspective(float mtx[4][4])
{
	// Untested
	CGX_BEGIN_LOAD_XF_REGS(0x1020, 7);
	wgPipe->F32 = mtx[0][0];
	wgPipe->F32 = mtx[0][2];
	wgPipe->F32 = mtx[1][1];
	wgPipe->F32 = mtx[1][2];
	wgPipe->F32 = mtx[2][2];
	wgPipe->F32 = mtx[2][3];
	wgPipe->U32 = 0;
}

void CGX_LoadProjectionMatrixOrthographic(float mtx[4][4])
{
	// Untested
	CGX_BEGIN_LOAD_XF_REGS(0x1020, 7);
	wgPipe->F32 = mtx[0][0];
	wgPipe->F32 = mtx[0][3];
	wgPipe->F32 = mtx[1][1];
	wgPipe->F32 = mtx[1][3];
	wgPipe->F32 = mtx[2][2];
	wgPipe->F32 = mtx[2][3];
	wgPipe->U32 = 1;
}

void CGX_DoEfbCopyTex(u16 left, u16 top, u16 width, u16 height, u8 dest_format, bool copy_to_intensity, void* dest, bool scale_down=false, bool clear=false) // TODO: Clear color
{
	assert(left <= 1023);
	assert(top <= 1023);
	assert(width <= 1023);
	assert(height <= 1023);

	// TODO: GX_TF_Z16 seems to have special treatment in libogc? oO

	UPE_Copy reg;
	reg.Hex = 0;
	reg.target_pixel_format = ((dest_format << 1) & 0xE) | (dest_format >> 3);
	reg.half_scale = scale_down;
	reg.clear = clear;
	reg.intensity_fmt = copy_to_intensity;

	X10Y10 coords;
	coords.hex = 0;
	coords.x = left;
	coords.y = top;
	CGX_LOAD_BP_REG((BPMEM_EFB_TL << 24) | coords.hex);
	coords.x = width - 1;
	coords.y = height - 1;
	CGX_LOAD_BP_REG((BPMEM_EFB_BR << 24) | coords.hex);

	CGX_LOAD_BP_REG((BPMEM_EFB_ADDR<<24) | (MEM_VIRTUAL_TO_PHYSICAL(dest)>>5));
	CGX_LOAD_BP_REG((BPMEM_TRIGGER_EFB_COPY<<24) | reg.Hex);
}

void CGX_DoEfbCopyXfb(u16 left, u16 top, u16 width, u16 height, void* dest, bool scale_down=false, bool clear=false) // TODO: Other parameters...
{
	assert(left <= 1023);
	assert(top <= 1023);
	assert(width <= 1023);
	assert(height <= 1023);

	UPE_Copy reg;
	reg.Hex = 0;
	reg.clear = clear;
	reg.copy_to_xfb = 1;

	X10Y10 coords;
	coords.hex = 0;
	coords.x = left;
	coords.y = top;
	CGX_LOAD_BP_REG((BPMEM_EFB_TL << 24) | coords.hex);
	coords.x = width - 1;
	coords.y = height - 1;
	CGX_LOAD_BP_REG((BPMEM_EFB_BR << 24) | coords.hex);

	CGX_LOAD_BP_REG((BPMEM_EFB_ADDR<<24) | (MEM_VIRTUAL_TO_PHYSICAL(dest)>>5));

	CGX_LOAD_BP_REG((BPMEM_MIPMAP_STRIDE<<24) | (width >> 4));
	CGX_LOAD_BP_REG((BPMEM_TRIGGER_EFB_COPY<<24) | reg.Hex);
}

void CGX_ForcePipelineFlush()
{
	wgPipe->U32 = 0;
	wgPipe->U32 = 0;
	wgPipe->U32 = 0;
	wgPipe->U32 = 0;
	wgPipe->U32 = 0;
	wgPipe->U32 = 0;
	wgPipe->U32 = 0;
	wgPipe->U32 = 0;
}

// TODO: Get rid of GX usage!
extern "C"
{
void GX_DrawDone();
}

void CGX_WaitForGpuToFinish()
{
	GX_DrawDone();
}

// Utility drawing functions
void CGX_DrawFullScreenQuad(int viewport_width, int viewport_height)
{
	VAT vtxattr;
	vtxattr.g0.Hex = 0;
	vtxattr.g1.Hex = 0;
	vtxattr.g2.Hex = 0;

	vtxattr.g0.PosElements = VA_TYPE_POS_XYZ;
	vtxattr.g0.PosFormat = VA_FMT_F32;

	vtxattr.g0.Color0Elements = VA_TYPE_CLR_RGBA;
	vtxattr.g0.Color0Comp = VA_FMT_RGBA8;

	// TODO: Figure out what this does and why it needs to be 1 for Dolphin not to error out
	vtxattr.g0.ByteDequant = 1;

	TVtxDesc vtxdesc;
	vtxdesc.Hex = 0;
	vtxdesc.Position = VTXATTR_DIRECT;
	vtxdesc.Color0 = VTXATTR_DIRECT;

	// TODO: Not sure if the order of these two is correct
	CGX_LOAD_CP_REG(0x50, vtxdesc.Hex0);
	CGX_LOAD_CP_REG(0x60, vtxdesc.Hex1);

	CGX_LOAD_CP_REG(0x70, vtxattr.g0.Hex);
	CGX_LOAD_CP_REG(0x80, vtxattr.g1.Hex);
	CGX_LOAD_CP_REG(0x90, vtxattr.g2.Hex);

	/* TODO: Should reset this matrix..
	float mtx[3][4];
	memset(&mtx, 0, sizeof(mtx));
	mtx[0][0] = 1.0;
	mtx[1][1] = 1.0;
	mtx[2][2] = 1.0;
	CGX_LoadPosMatrixDirect(mtx, 0);*/

	float mtx[4][4];
	memset(mtx, 0, sizeof(mtx));
	mtx[0][0] = 1;
	mtx[1][1] = 1;
	mtx[2][2] = -1;
	CGX_LoadProjectionMatrixOrthographic(mtx);

	wgPipe->U8 = 0x80; // draw quads
	wgPipe->U16 = 4; // 4 vertices

	// Bottom right
	wgPipe->F32 = -1.0;
	wgPipe->F32 = 1.0;
	wgPipe->F32 = 1.0;
	wgPipe->U32 = 0xFFFFFFFF;

	// Top right
	wgPipe->F32 = 1.0;
	wgPipe->F32 = 1.0;
	wgPipe->F32 = 1.0;
	wgPipe->U32 = 0xFFFFFFFF;

	// Top left
	wgPipe->F32 = 1.0;
	wgPipe->F32 = -1.0;
	wgPipe->F32 = 1.0;
	wgPipe->U32 = 0xFFFFFFFF;

	// Bottom left
	wgPipe->F32 = -1.0;
	wgPipe->F32 = -1.0;
	wgPipe->F32 = 1.0;
	wgPipe->U32 = 0xFFFFFFFF;
}
