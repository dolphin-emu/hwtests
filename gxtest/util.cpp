// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <assert.h>
#include <gccore.h>
#include <malloc.h>
#include <ogc/video.h>
#include <string.h>

#include "gxtest/cgx.h"
#include "gxtest/cgx_defaults.h"
#include "gxtest/util.h"

//#define ENABLE_DEBUG_DISPLAY

namespace GXTest
{
#define TEST_BUFFER_SIZE (640 * 528 * 4)
static u32* test_buffer;

#ifdef ENABLE_DEBUG_DISPLAY
static u32 fb = 0;
static void* frameBuffer[2] = {NULL, NULL};
static GXRModeObj* rmode;

float yscale;
u32 xfbHeight;
#endif

void Init()
{
  GXColor background = {0, 0x27, 0, 0xff};

#if defined(ENABLE_DEBUG_DISPLAY)
  VIDEO_Init();

  rmode = VIDEO_GetPreferredMode(NULL);

  frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
  frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

  VIDEO_Configure(rmode);
  VIDEO_SetNextFramebuffer(frameBuffer[fb]);
  VIDEO_SetBlack(FALSE);
  VIDEO_Flush();
  VIDEO_WaitVSync();
  if (rmode->viTVMode & VI_NON_INTERLACE)
    VIDEO_WaitVSync();

  CGX_Init();

  GX_SetCopyClear(background, 0x00ffffff);
  GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
  yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
  xfbHeight = GX_SetDispCopyYScale(yscale);
  GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
  GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
  GX_SetDispCopyDst(rmode->fbWidth, xfbHeight);
  GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, 1, rmode->vfilter);
  GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? 1 : 0));
  GX_SetDispCopyGamma(GX_GM_1_0);
#else
  CGX_Init();

  GX_SetCopyClear(background, 0x00ffffff);
  GX_SetViewport(0, 0, 640, 528, 0, 1);
  GX_SetScissor(0, 0, 640, 528);
#endif

  test_buffer = (u32*)memalign(32, 640 * 528 * 4);

  GX_SetTexCopySrc(0, 0, 100, 100);
  GX_SetTexCopyDst(100, 100, GX_TF_RGBA8, false);

  // We draw a dummy quad here to apply cached GX state...
  // TODO: Implement proper GPU initialization in GX so that we don't need
  // to do this anymore
  GX_ClearVtxDesc();
  GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
  GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

  GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
  GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

  Mtx model;
  guMtxIdentity(model);
  GX_LoadPosMtxImm(model, GX_PNMTX0);

  float mtx[4][4];
  memset(mtx, 0, sizeof(mtx));
  mtx[0][0] = 1;
  mtx[1][1] = 1;
  mtx[2][2] = -1;
  CGX_LoadProjectionMatrixOrthographic(mtx);

  GX_SetNumChans(1);  // damnit dirty state...
  GX_SetNumTexGens(0);
  GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
  GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

  GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
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

  GX_End();
  GX_Flush();

  PE_CONTROL ctrl;
  ctrl.hex = BPMEM_ZCOMPARE << 24;
  ctrl.pixel_format = PIXELFMT_RGBA6_Z24;
  ctrl.zformat = ZC_LINEAR;
  ctrl.early_ztest = 0;
  CGX_LOAD_BP_REG(ctrl.hex);
}

void DebugDisplayEfbContents()
{
#ifdef ENABLE_DEBUG_DISPLAY
  CGX_DoEfbCopyXfb(0, 0, rmode->fbWidth, rmode->efbHeight, xfbHeight, frameBuffer[fb]);
  CGX_WaitForGpuToFinish();

  VIDEO_SetNextFramebuffer(frameBuffer[fb]);
  VIDEO_Flush();
  VIDEO_WaitVSync();
  fb ^= 1;
#endif
}

Vec4<u8> ReadTestBuffer(int s, int t, int width)
{
  u16 sBlk = s >> 2;
  u16 tBlk = t >> 2;
  u16 widthBlks = (width + 3) >> 2;
  u32 base = (tBlk * widthBlks + sBlk) << 5;
  u16 blkS = s & 3;
  u16 blkT = t & 3;
  u32 blkOff = (blkT << 2) + blkS;

  u32 offset = (base + blkOff) << 1;
  const u8* valAddr = ((u8*)test_buffer) + offset;

  Vec4<u8> ret;
  ret.r = valAddr[1];
  ret.g = valAddr[32];
  ret.b = valAddr[33];
  ret.a = valAddr[0];
  return ret;
}

Quad::Quad()
{
  // top left
  x[0] = -1.0;
  y[0] = 1.0;
  z[0] = 1.0;

  // top right
  x[1] = 1.0;
  y[1] = 1.0;
  z[1] = 1.0;

  // bottom right
  x[2] = 1.0;
  y[2] = -1.0;
  z[2] = 1.0;

  // bottom left
  x[3] = -1.0;
  y[3] = -1.0;
  z[3] = 1.0;

  has_color = false;
}

Quad& Quad::VertexTopLeft(f32 x, f32 y, f32 z)
{
  this->x[0] = x;
  this->y[0] = y;
  this->z[0] = z;
  return *this;
}

Quad& Quad::VertexTopRight(f32 x, f32 y, f32 z)
{
  this->x[1] = x;
  this->y[1] = y;
  this->z[1] = z;
  return *this;
}

Quad& Quad::VertexBottomRight(f32 x, f32 y, f32 z)
{
  this->x[2] = x;
  this->y[2] = y;
  this->z[2] = z;
  return *this;
}

Quad& Quad::VertexBottomLeft(f32 x, f32 y, f32 z)
{
  this->x[3] = x;
  this->y[3] = y;
  this->z[3] = z;
  return *this;
}

Quad& Quad::AtDepth(f32 depth)
{
  z[0] = z[1] = z[2] = z[3] = depth;

  return *this;
}

Quad& Quad::ColorRGBA(u8 r, u8 g, u8 b, u8 a)
{
  color = ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | (u32)a;
  has_color = true;

  return *this;
}

void Quad::Draw()
{
  VAT vtxattr;
  vtxattr.g0.Hex = 0;
  vtxattr.g1.Hex = 0;
  vtxattr.g2.Hex = 0;

  vtxattr.g0.PosElements = VA_TYPE_POS_XYZ;
  vtxattr.g0.PosFormat = VA_FMT_F32;

  if (has_color)
  {
    vtxattr.g0.Color0Elements = VA_TYPE_CLR_RGBA;
    vtxattr.g0.Color0Comp = VA_FMT_RGBA8;
  }

  // TODO: Figure out what this does and why it needs to be 1 for Dolphin not to error out
  vtxattr.g0.ByteDequant = 1;

  TVtxDesc vtxdesc;
  vtxdesc.Hex = 0;
  vtxdesc.Position = VTXATTR_DIRECT;

  if (has_color)
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

  wgPipe->U8 = 0x80;  // draw quads
  wgPipe->U16 = 4;    // 4 vertices

  for (int i = 0; i < 4; ++i)
  {
    wgPipe->F32 = x[i];
    wgPipe->F32 = y[i];
    wgPipe->F32 = z[i];

    if (has_color)
      wgPipe->U32 = color;
  }
}

void CopyToTestBuffer(int left_most_pixel, int top_most_pixel, int right_most_pixel,
                      int bottom_most_pixel)
{
  // TODO: Do we need to impose additional constraints on the parameters?
  memset(test_buffer, 0, TEST_BUFFER_SIZE);
  CGX_DoEfbCopyTex(left_most_pixel, top_most_pixel, right_most_pixel - left_most_pixel + 1,
                   bottom_most_pixel - top_most_pixel + 1, 0x6 /*RGBA8*/, false, test_buffer);
}

Vec4<int> GetTevOutput(const GenMode& genmode, const TevStageCombiner::ColorCombiner& last_cc,
                       const TevStageCombiner::AlphaCombiner& last_ac)
{
  int previous_stage = ((last_cc.hex >> 24) - BPMEM_TEV_COLOR_ENV) >> 1;
  assert(previous_stage < 13);
  assert(previous_stage == (((last_ac.hex >> 24) - BPMEM_TEV_ALPHA_ENV) >> 1));

  // The TEV output gets truncated to 8 bits when writing to the EFB.
  // Hence, we cannot retrieve all 11 TEV output bits directly.
  // Instead, we're performing two render passes, one of which retrieves
  // the lower 6 output bits, the other one of which retrieves the upper
  // 5 bits.

  // FIRST RENDER PASS:
  // As set up by the caller, with one additional tev stage multiplying the result by 4.
  // This will retrieve the lower 6 bits of the TEV output.

  auto gm = genmode;
  gm.numtevstages = previous_stage + 1;  // one additional stage
  CGX_LOAD_BP_REG(gm.hex);

  // Enable new TEV stage. Note that we are using the "a" input here to make
  // sure the input doesn't get erroneously clamped to 11 bit range.
  auto cc1 = CGXDefault<TevStageCombiner::ColorCombiner>(previous_stage + 1);
  cc1.a = last_cc.dest * 2;
  cc1.shift = TEVSCALE_4;
  CGX_LOAD_BP_REG(cc1.hex);

  auto ac1 = CGXDefault<TevStageCombiner::AlphaCombiner>(previous_stage + 1);
  ac1.a = last_ac.dest * 2;
  ac1.shift = TEVSCALE_4;
  CGX_LOAD_BP_REG(ac1.hex);

  memset(test_buffer, 0, TEST_BUFFER_SIZE);  // Just for debugging
  Quad().AtDepth(1.0).ColorRGBA(255, 255, 255, 255).Draw();
  CGX_DoEfbCopyTex(0, 0, 100, 100, 0x6 /*RGBA8*/, false, test_buffer);
  CGX_ForcePipelineFlush();
  CGX_WaitForGpuToFinish();
  u16 result1r = ReadTestBuffer(5, 5, 100).r >> 2;
  u16 result1g = ReadTestBuffer(5, 5, 100).g >> 2;
  u16 result1b = ReadTestBuffer(5, 5, 100).b >> 2;
  u16 result1a = ReadTestBuffer(5, 5, 100).a >> 2;

  // SECOND RENDER PASS
  // Uses three additional TEV stages which shift the previous result
  // three bits to the right. This is necessary to read off the 5 upper bits,
  // 3 of which got masked off when writing to the EFB in the first pass.
  gm = genmode;
  gm.numtevstages = previous_stage + 3;  // three additional stages
  CGX_LOAD_BP_REG(gm.hex);

  // The following tev stages are exclusively used to rightshift the
  // upper bits such that they get written to the render target.
  cc1 = CGXDefault<TevStageCombiner::ColorCombiner>(previous_stage + 1);
  cc1.d = last_cc.dest * 2;
  cc1.shift = TEVDIVIDE_2;
  CGX_LOAD_BP_REG(cc1.hex);

  ac1 = CGXDefault<TevStageCombiner::AlphaCombiner>(previous_stage + 1);
  ac1.d = last_ac.dest * 2;
  ac1.shift = TEVDIVIDE_2;
  CGX_LOAD_BP_REG(ac1.hex);

  cc1 = CGXDefault<TevStageCombiner::ColorCombiner>(previous_stage + 2);
  cc1.d = last_cc.dest * 2;
  cc1.shift = TEVDIVIDE_2;
  CGX_LOAD_BP_REG(cc1.hex);

  ac1 = CGXDefault<TevStageCombiner::AlphaCombiner>(previous_stage + 2);
  ac1.d = last_ac.dest * 2;
  ac1.shift = TEVDIVIDE_2;
  CGX_LOAD_BP_REG(ac1.hex);

  cc1 = CGXDefault<TevStageCombiner::ColorCombiner>(previous_stage + 3);
  cc1.d = last_cc.dest * 2;
  cc1.shift = TEVDIVIDE_2;
  CGX_LOAD_BP_REG(cc1.hex);

  ac1 = CGXDefault<TevStageCombiner::AlphaCombiner>(previous_stage + 3);
  ac1.d = last_ac.dest * 2;
  ac1.shift = TEVDIVIDE_2;
  CGX_LOAD_BP_REG(ac1.hex);

  memset(test_buffer, 0, TEST_BUFFER_SIZE);
  Quad().AtDepth(1.0).ColorRGBA(255, 255, 255, 255).Draw();
  CGX_DoEfbCopyTex(0, 0, 100, 100, 0x6 /*RGBA8*/, false, test_buffer);
  CGX_ForcePipelineFlush();
  CGX_WaitForGpuToFinish();

  u16 result2r = ReadTestBuffer(5, 5, 100).r >> 3;
  u16 result2g = ReadTestBuffer(5, 5, 100).g >> 3;
  u16 result2b = ReadTestBuffer(5, 5, 100).b >> 3;
  u16 result2a = ReadTestBuffer(5, 5, 100).a >> 3;

  // uh.. let's just say this works, but I guess it could be simplified.
  Vec4<int> result;
  result.r = result1r + ((result2r & 0x10) ? (-0x400 + ((result2r & 0xF) << 6)) : (result2r << 6));
  result.g = result1g + ((result2g & 0x10) ? (-0x400 + ((result2g & 0xF) << 6)) : (result2g << 6));
  result.b = result1b + ((result2b & 0x10) ? (-0x400 + ((result2b & 0xF) << 6)) : (result2b << 6));
  result.a = result1a + ((result2a & 0x10) ? (-0x400 + ((result2a & 0xF) << 6)) : (result2a << 6));
  return result;
}
}
