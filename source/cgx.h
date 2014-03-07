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

#pragma once

typedef union _wgpipe
{
	volatile u8 U8;
	volatile s8 S8;
	volatile u16 U16;
	volatile s16 S16;
	volatile u32 U32;
	volatile s32 S32;
//	volatile f32 F32;
} WGPipe;

static WGPipe* const wgPipe = (WGPipe*)0xCC008000;

// TODO: Get rid of these definitions!
struct GXFifoObj;
extern "C"
{
GXFifoObj* GX_Init(void* base, u32 size);
}

void CGX_Init()
{
	// TODO: Is this leaking memory?
    void *gp_fifo = NULL;
    gp_fifo = memalign(32, 256*1024);
    memset(gp_fifo, 0, 256*1024);

	GX_Init(gp_fifo, 256*1024);
}

void CGX_DoEfbCopyTex(u16 left, u16 top, u16 width, u16 height, u8 dest_format, bool copy_to_intensity, void* dest, bool scale_down=false, bool clear=false)
{
	assert(left <= 1023);
	assert(top <= 1023);
	assert(width <= 1023);
	assert(height <= 1023);

	// TODO: GX_TF_Z16 seems to have special treatment in libogc? oO

	UPE_Copy reg;
	reg.Hex = 0;
	reg.target_pixel_format = ((dest_format << 1) & 0xE ) | (dest_format >> 3);
	reg.half_scale = scale_down;
	reg.clear = clear;
	reg.intensity_fmt = copy_to_intensity;

	X10Y10 coords;
	coords.hex = 0;
	coords.x = left;
	coords.y = top;
	wgPipe->U32 = (BPMEM_EFB_TL<<24) | coords.hex;
	coords.x = width - 1;
	coords.y = height - 1;
	wgPipe->U32 = (BPMEM_EFB_BR<<24) | coords.hex;

	wgPipe->U32 = (BPMEM_EFB_ADDR<<24) | MEM_VIRTUAL_TO_PHYSICAL(dest);
	wgPipe->U32 = (BPMEM_TRIGGER_EFB_COPY<<24) | reg.Hex;
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
