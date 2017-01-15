// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <malloc.h>
#include <ogc/gx.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>

#include "hwtests.h"

#define DEFAULT_FIFO_SIZE (256 * 1024)

#define EFB_WIDTH 640
#define EFB_HEIGHT 528

#define OUT_WIDTH EFB_WIDTH
#define OUT_HEIGHT EFB_HEIGHT
#define OUT_SIZE (OUT_WIDTH * OUT_HEIGHT * 4)

struct Color {
  u8 r;
  u8 g;
  u8 b;
  u8 a;
};

static struct Color ReadTestBuffer(u8 *test_buffer, int s, int t, int width) {
  u16 sBlk = s >> 2;
  u16 tBlk = t >> 2;
  u16 widthBlks = (width >> 2) + 1;
  u32 base = (tBlk * widthBlks + sBlk) << 5;
  u16 blkS = s & 3;
  u16 blkT = t & 3;
  u32 blkOff = (blkT << 2) + blkS;

  u32 offset = (base + blkOff) << 1;
  const u8 *valAddr = ((u8 *)test_buffer) + offset;

  struct Color ret;
  ret.r = valAddr[1];
  ret.g = valAddr[32];
  ret.b = valAddr[33];
  ret.a = valAddr[0];
  return ret;
}

struct DXTBlock {
  u16 color1;
  u16 color2;
  u8 lines[4];
};

static struct Color MakeRGBA(int r, int g, int b, int a) {
  struct Color c = {(u8)r, (u8)g, (u8)b, (u8)a};
  return c;
}

static int DXTBlend(int v1, int v2) {
  // 3/8 blend, which is close to 1/3
  return ((v1 * 3 + v2 * 5) >> 3);
}

static u8 Convert3To8(u8 v) {
  // Swizzle bits: 00000123 -> 12312312
  return (v << 5) | (v << 2) | (v >> 1);
}

static u8 Convert4To8(u8 v) {
  // Swizzle bits: 00001234 -> 12341234
  return (v << 4) | v;
}

static u8 Convert5To8(u8 v) {
  // Swizzle bits: 00012345 -> 12345123
  return (v << 3) | (v >> 2);
}

static u8 Convert6To8(u8 v) {
  // Swizzle bits: 00123456 -> 12345612
  return (v << 2) | (v >> 4);
}

static struct Color ReadCmprBuffer(u8 *src, int s, int t, int imageWidth) {
  u16 sDxt = s >> 2;
  u16 tDxt = t >> 2;

  u16 sBlk = sDxt >> 1;
  u16 tBlk = tDxt >> 1;
  u16 widthBlks = (imageWidth >> 3) + 1;
  u32 base = (tBlk * widthBlks + sBlk) << 2;
  u16 blkS = sDxt & 1;
  u16 blkT = tDxt & 1;
  u32 blkOff = (blkT << 1) + blkS;

  u32 offset = (base + blkOff) << 3;

  struct DXTBlock *dxtBlock = (struct DXTBlock *)(src + offset);

  u16 c1 = (dxtBlock->color1);
  u16 c2 = (dxtBlock->color2);
  int blue1 = Convert5To8(c1 & 0x1F);
  int blue2 = Convert5To8(c2 & 0x1F);
  int green1 = Convert6To8((c1 >> 5) & 0x3F);
  int green2 = Convert6To8((c2 >> 5) & 0x3F);
  int red1 = Convert5To8((c1 >> 11) & 0x1F);
  int red2 = Convert5To8((c2 >> 11) & 0x1F);

  u16 ss = s & 3;
  u16 tt = t & 3;

  int colorSel = dxtBlock->lines[tt];
  int rs = 6 - (ss << 1);
  colorSel = (colorSel >> rs) & 3;
  colorSel |= c1 > c2 ? 0 : 4;

  struct Color color = {0, 0, 0, 0};

  switch (colorSel) {
  case 0:
  case 4:
    color = MakeRGBA(red1, green1, blue1, 255);
    break;
  case 1:
  case 5:
    color = MakeRGBA(red2, green2, blue2, 255);
    break;
  case 2:
    color = MakeRGBA(DXTBlend(red2, red1), DXTBlend(green2, green1),
                     DXTBlend(blue2, blue1), 255);
    break;
  case 3:
    color = MakeRGBA(DXTBlend(red1, red2), DXTBlend(green1, green2),
                     DXTBlend(blue1, blue2), 255);
    break;
  case 6:
    color = MakeRGBA((red1 + red2) / 2, (green1 + green2) / 2,
                     (blue1 + blue2) / 2, 255);
    break;
  case 7:
    color = MakeRGBA((red1 + red2) / 2, (green1 + green2) / 2,
                     (blue1 + blue2) / 2, 0);
    break;
  }
  return color;
}

static void CmprTest() {
  START_TEST();

  // setup the fifo and then init the flipper
  void *gp_fifo = memalign(32, DEFAULT_FIFO_SIZE);

  memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);
  if (!gp_fifo)
    return;

  GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);

  u8 *tmp = (u8 *)memalign(32, OUT_SIZE); // this is probably the wrong size
  u8 *out = (u8 *)memalign(32, OUT_SIZE);
  if (!out)
    return;
  memset(out, 0, OUT_SIZE);
  DCFlushRange(out, OUT_SIZE);

  // clears the bg to color and clears the z buffer
  GXColor background = {1, 0, 0, 0xff};
  GX_SetCopyClear(background, 0x00ffffff);

  GX_CopyDisp(tmp, GX_TRUE);

  // setup the copy for later
  GX_SetDispCopyYScale(1);
  GX_SetCopyFilter(GX_FALSE, 0, GX_FALSE, 0);
  GX_SetTexCopySrc(0, 0, OUT_WIDTH, OUT_HEIGHT);
  GX_SetTexCopyDst(OUT_WIDTH, OUT_HEIGHT, GX_TF_RGBA8, 0);

  {
    // setup some other defaults
    GX_SetCullMode(GX_CULL_NONE);
    GX_SetViewport(0, 0, EFB_WIDTH, EFB_HEIGHT, 0, 1);
    GX_SetScissor(0, 0, EFB_WIDTH, EFB_HEIGHT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    GX_SetNumChans(1);
    GX_SetNumTexGens(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
  }

  // clear buffer
  GX_CopyTex(out, GX_TRUE);

  u8 *texdata;
  {
#define TX_WIDTH 512
#define TX_HEIGHT 512
    const u32 width = TX_WIDTH;
    const u32 height = TX_HEIGHT;
    const u32 bytesize = (width / 4) * (height / 4) * 8;
    srand(0);
    texdata = (u8 *)memalign(32, bytesize);
    {
      u32 i;
      for (i = 0; i < bytesize; i++)
        texdata[i] = rand();

      for (i = 4; i < bytesize; i += 8)
        *(u32 *)&texdata[i] = 0x1B1B1B1B;
    }

    DCFlushRange(texdata, bytesize);

    GXTexObj texObj;
    GX_InitTexObj(&texObj, texdata, width, height, GX_TF_CMPR, GX_REPEAT,
                  GX_REPEAT, false);
    GX_InitTexObjFilterMode(&texObj, GX_NEAR, GX_NEAR);
    GX_LoadTexObj(&texObj, GX_TEXMAP0);

    GX_InvalidateTexAll();
  }

  {
    Mtx44 perspective;
    guOrtho(perspective, 0, EFB_HEIGHT - 1, 0, EFB_WIDTH - 1, 0, 300);
    GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    Mtx GXmodelView2D;
    guMtxIdentity(GXmodelView2D);
    guMtxTransApply(GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -5.0F);
    GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

    GX_SetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);

    u32 x = 0, y = 0;
    u32 width = TX_WIDTH;
    u32 height = TX_HEIGHT;
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4); // Draw A Quad
    GX_Position2f32(x, y);             // Top Left
    GX_TexCoord2f32(0.0, 0.0);
    GX_Position2f32(x + width - 1, y); // Top Right
    GX_TexCoord2f32(1.0, 0.0);
    GX_Position2f32(x + width - 1, y + height - 1); // Bottom Right
    GX_TexCoord2f32(1.0, 1.0);
    GX_Position2f32(x, y + height - 1); // Bottom Left
    GX_TexCoord2f32(0.0, 1.0);
    GX_End(); // Done Drawing The Quad
  }

  // final copy
  GX_CopyTex(out, GX_FALSE);

  // wait for the gpu
  GX_DrawDone();

  int count = 0;
  {
    int x, y;
    for (y = 0; y < 512; y++) {
      for (x = 0; x < 512; x++) {
        struct Color c = ReadTestBuffer(out, x, y, EFB_WIDTH - 1);
        struct Color c2 = ReadCmprBuffer(texdata, x, y, TX_WIDTH - 1);

        DO_TEST(c.r == c2.r && c.g == c2.g && c.b == c2.b,
                "Incorrect color (result=%02x%02x%02x, expected=%02x%02x%02x)",
                c.r, c.g, c.b, c2.r, c2.g, c2.b);
      }
    }
  }

  free(tmp);
  free(out);
  free(gp_fifo);
  free(texdata);

  END_TEST();
}

int main() {
  network_init();
  WPAD_Init();

  // GXTest::Init();

  CmprTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}
