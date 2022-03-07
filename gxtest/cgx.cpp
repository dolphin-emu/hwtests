// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <assert.h>
#include <malloc.h>
#include <ogc/cache.h>
#include <ogc/gx.h>
#include <ogc/irq.h>
#include <ogc/machine/processor.h>
#include <ogc/system.h>
#include <string.h>

#include "common/CommonTypes.h"
#include "gxtest/BPMemory.h"
#include "gxtest/CPMemory.h"
#include "gxtest/XFMemory.h"
#include "gxtest/util.h"

#include "cgx.h"

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

// static CWGPipe* const wgPipe = (CWGPipe*)0xCC008000;

// TODO: Get rid of these definitions!
// struct GXFifoObj;
extern "C" {
GXFifoObj* GX_Init(void* base, u32 size);
}

static void __CGXFinishInterruptHandler(u32 irq, void* ctx);
static vu16* const _peReg = (u16*)0xCC001000;
static lwpq_t _cgxwaitfinish;
static vu32 _cgxfinished = 0;

#define CGX_LOAD_BP_REG(x)                                                                         \
  do                                                                                               \
  {                                                                                                \
    wgPipe->U8 = 0x61;                                                                             \
    wgPipe->U32 = (u32)(x);                                                                        \
  } while (0)

#define CGX_LOAD_CP_REG(x, y)                                                                      \
  do                                                                                               \
  {                                                                                                \
    wgPipe->U8 = 0x08;                                                                             \
    wgPipe->U8 = (u8)(x);                                                                          \
    wgPipe->U32 = (u32)(y);                                                                        \
  } while (0)

#define CGX_BEGIN_LOAD_XF_REGS(x, n)                                                               \
  do                                                                                               \
  {                                                                                                \
    wgPipe->U8 = 0x10;                                                                             \
    wgPipe->U32 = (u32)(((((n)&0xffff) - 1) << 16) | ((x)&0xffff));                                \
  } while (0)

void CGX_Init()
{
  // TODO: Is this leaking memory?
  void* gp_fifo = NULL;
  gp_fifo = memalign(32, 256 * 1024);
  memset(gp_fifo, 0, 256 * 1024);

  GX_Init(gp_fifo, 256 * 1024);

  LWP_InitQueue(&_cgxwaitfinish);

  IRQ_Request(IRQ_PI_PEFINISH, __CGXFinishInterruptHandler, NULL);
  __UnmaskIrq(IRQMASK(IRQ_PI_PEFINISH));
  _peReg[5] = 0x0F;
}

void CGX_SetViewport(float origin_x, float origin_y, float width, float height, float near, f32 far)
{
  CGX_BEGIN_LOAD_XF_REGS(0x101a, 6);
  wgPipe->F32 = width * 0.5f;
  wgPipe->F32 = -height * 0.5f;
  wgPipe->F32 = (far - near) * 16777215.0f;
  wgPipe->F32 = 342.0f + origin_x + width * 0.5f;
  wgPipe->F32 = 342.0f + origin_y + height * 0.5f;
  wgPipe->F32 = far * 16777215.0f;
}

void CGX_LoadPosMatrixDirect(f32 mt[3][4], u32 index)
{
  // Untested
  /*	CGX_BEGIN_LOAD_XF_REGS((index<<2)&0xFF, 12);
    WriteMtxPS4x2(mt, (void*)wgPipe);*/
  GX_LoadPosMtxImm(mt, index);
}

void CGX_LoadProjectionMatrixPerspective(float mtx[4][4])
{
  // Untested
  /*	CGX_BEGIN_LOAD_XF_REGS(0x1020, 7);
    wgPipe->F32 = mtx[0][0];
    wgPipe->F32 = mtx[0][2];
    wgPipe->F32 = mtx[1][1];
    wgPipe->F32 = mtx[1][2];
    wgPipe->F32 = mtx[2][2];
    wgPipe->F32 = mtx[2][3];
    wgPipe->U32 = 0;*/
  GX_LoadProjectionMtx(mtx, 0);
}

void CGX_LoadProjectionMatrixOrthographic(float mtx[4][4])
{
  // Untested
  /*	CGX_BEGIN_LOAD_XF_REGS(0x1020, 7);
    wgPipe->F32 = mtx[0][0];
    wgPipe->F32 = mtx[0][3];
    wgPipe->F32 = mtx[1][1];
    wgPipe->F32 = mtx[1][3];
    wgPipe->F32 = mtx[2][2];
    wgPipe->F32 = mtx[2][3];
    wgPipe->U32 = 1;*/
  GX_LoadProjectionMtx(mtx, 1);
}

void CGX_DoEfbCopyTex(u16 left, u16 top, u16 width, u16 height, void* dest, const EFBCopyParams& params)
{
  assert(left <= 1023);
  assert(top <= 1023);
  assert(width <= 1023);
  assert(height <= 1023);

  // TODO: GX_TF_Z16 seems to have special treatment in libogc? oO

  X10Y10 coords;
  coords.hex = BPMEM_EFB_TL << 24;
  coords.x = left;
  coords.y = top;
  CGX_LOAD_BP_REG(coords.hex);

  coords.hex = BPMEM_EFB_WH << 24;
  coords.x = width - 1;
  coords.y = height - 1;
  CGX_LOAD_BP_REG(coords.hex);

  // TODO: this one is hardcoded against dest_format=RGBA8...
  CGX_LOAD_BP_REG((BPMEM_MIPMAP_STRIDE << 24) | (((width + 3) >> 2) * 2));

  CGX_LOAD_BP_REG((BPMEM_EFB_ADDR << 24) | (MEM_VIRTUAL_TO_PHYSICAL(dest) >> 5));

  UPE_Copy reg;
  reg.Hex = BPMEM_TRIGGER_EFB_COPY << 24;
  reg.SetRealFormat(params.format);
  reg.clamp_top = params.clamp_top;
  reg.clamp_bottom = params.clamp_bottom;
  reg.unknown_bit = params.unknown_bit;
  reg.gamma = params.gamma;
  reg.half_scale = params.half_scale;
  reg.scale_invert = params.scale_invert;
  reg.clear = params.clear;
  reg.frame_to_field = params.frame_to_field;
  reg.copy_to_xfb = params.copy_to_xfb;
  reg.intensity_fmt = params.intensity_fmt;
  reg.auto_conv = params.auto_conv;
  CGX_LOAD_BP_REG(reg.Hex);

  DCFlushRange(dest, GX_GetTexBufferSize(width, height, GX_TF_RGBA8, GX_FALSE, 1));
}

void CGX_DoEfbCopyXfb(u16 left, u16 top, u16 width, u16 src_height, u16 dst_height, void* dest,
                      bool clear)
{
  assert(left <= 1023);
  assert(top <= 1023);
  assert(width <= 1023);
  assert(src_height <= 1023);

  /*	UPE_Copy reg;
    reg.Hex = BPMEM_TRIGGER_EFB_COPY<<24;
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
    CGX_LOAD_BP_REG(reg.Hex);*/

  GX_SetDispCopySrc(left, top, width, src_height);
  GX_SetDispCopyDst(width, dst_height);
  // SetCopyFilter, SetFieldMode, SetDispCopyGamma
  GX_CopyDisp(dest, clear);
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

static void __CGXFinishInterruptHandler([[maybe_unused]] u32 irq, [[maybe_unused]] void* ctx)
{
  _peReg[5] = (_peReg[5] & ~0x08) | 0x08;
  _cgxfinished = 1;

  LWP_ThreadBroadcast(_cgxwaitfinish);
}

void CGX_WaitForGpuToFinish()
{
  u32 level;

  _CPU_ISR_Disable(level);
  CGX_LOAD_BP_REG(0x45000002);  // draw done
  CGX_ForcePipelineFlush();

  _cgxfinished = 0;
  _CPU_ISR_Flash(level);

  while (!_cgxfinished)
    LWP_ThreadSleep(_cgxwaitfinish);

  _CPU_ISR_Restore(level);
}

void CGX_PEPokeAlphaMode(CompareMode func, u8 threshold)
{
  GX_PokeAlphaMode(static_cast<u8>(func), threshold);
}
void CGX_PEPokeAlphaUpdate(bool enable)
{
  GX_PokeAlphaUpdate(enable);
}
void CGX_PEPokeColorUpdate(bool enable)
{
  GX_PokeColorUpdate(enable);
}
void CGX_PEPokeDither(bool dither)
{
  GX_PokeDither(dither);
}
void CGX_PEPokeBlendMode(u8 type, SrcBlendFactor src_fact, DstBlendFactor dst_fact, LogicOp op)
{
  GX_PokeBlendMode(type, static_cast<u8>(src_fact), static_cast<u8>(dst_fact), static_cast<u8>(op));
}
void CGX_PEPokeAlphaRead(u8 mode)
{
  GX_PokeAlphaRead(mode);
}
void CGX_PEPokeDstAlpha(bool enable, u8 a)
{
  GX_PokeDstAlpha(enable, a);
}
void CGX_PEPokeZMode(bool comp_enable, CompareMode func, bool update_enable)
{
  GX_PokeZMode(comp_enable, static_cast<u8>(func), update_enable);
}

// The pixel_fmt arg is unused currently but exists for future compatibility
// if we need to do different types of reads for different formats
GXTest::Vec4<u8> CGX_PeekARGB(u16 x, u16 y, [[maybe_unused]] PixelFormat pixel_fmt)
{
  GXColor gx_color;
  GX_PeekARGB(x, y, &gx_color);
  return {.r = gx_color.r, .g = gx_color.g, .b = gx_color.b, .a = gx_color.a};
}
u32 CGX_PeekZ(u16 x, u16 y, [[maybe_unused]] PixelFormat pixel_fmt)
{
  u32 z;
  GX_PeekZ(x, y, &z);
  return z;
}
void CGX_PokeARGB(u16 x, u16 y, const GXTest::Vec4<u8>& color, [[maybe_unused]] PixelFormat pixel_fmt)
{
  GXColor gx_color{.r = color.r, .g = color.g, .b = color.b, .a = color.a};
  GX_PokeARGB(x, y, gx_color);
}
void CGX_PokeZ(u16 x, u16 y, u32 z, [[maybe_unused]] PixelFormat pixel_fmt)
{
  GX_PokeZ(x, y, z);
}
