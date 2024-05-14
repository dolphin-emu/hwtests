// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <math.h>
#include <ogcsys.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <wiiuse/wpad.h>

#include <initializer_list>
#include <vector>
#include <algorithm>

#include "common/hwtests.h"
#include "gxtest/cgx.h"
#include "gxtest/cgx_defaults.h"
#include "gxtest/util.h"

#define EFB_WIDTH 640
#define EFB_HEIGHT 528

#define BACKGROUND_BLUE 0
#define VIEWPORT_BLUE 128

__attribute__((aligned(32))) u8 clear_buffer[EFB_WIDTH*EFB_HEIGHT];
__attribute__((aligned(32))) u8 test_buffer[EFB_WIDTH*EFB_HEIGHT];

// CGX_DoEfbCopyTex causes problems since it only works right with RGBA8
void CopyToBuffer(u8 format, void* dest, bool clear)
{
  GX_SetTexCopySrc(0, 0, EFB_WIDTH, EFB_HEIGHT);
  GX_SetTexCopyDst(EFB_WIDTH, EFB_HEIGHT, format, false);
  GX_CopyTex(dest, clear);
  DCInvalidateRange(dest, GX_GetTexBufferSize(EFB_WIDTH, EFB_HEIGHT, format, false, 1));
}

void ConfigureGX()
{
  VAT vtxattr;
  vtxattr.g0.Hex = 0;
  vtxattr.g1.Hex = 0;
  vtxattr.g2.Hex = 0;

  vtxattr.g0.PosElements = VA_TYPE_POS_XY;
  vtxattr.g0.PosFormat = VA_FMT_S8;

  vtxattr.g0.Color0Elements = VA_TYPE_CLR_RGBA;
  vtxattr.g0.Color0Comp = VA_FMT_RGBA8;

  vtxattr.g0.ByteDequant = 1;

  TVtxDesc vtxdesc;
  vtxdesc.Hex = 0;
  vtxdesc.Position = VTXATTR_DIRECT;
  vtxdesc.Color0 = VTXATTR_DIRECT;

  CGX_LOAD_CP_REG(0x50, vtxdesc.Hex0);
  CGX_LOAD_CP_REG(0x60, vtxdesc.Hex1);

  CGX_LOAD_CP_REG(0x70, vtxattr.g0.Hex);
  CGX_LOAD_CP_REG(0x80, vtxattr.g1.Hex);
  CGX_LOAD_CP_REG(0x90, vtxattr.g2.Hex);

  CGX_BEGIN_LOAD_XF_REGS(0x1009, 1);
  wgPipe->U32 = 1;  // 1 color channel

  LitChannel chan;
  chan.hex = 0;
  chan.matsource = 1;                 // from vertex
  CGX_BEGIN_LOAD_XF_REGS(0x100e, 1);  // color channel 1
  wgPipe->U32 = chan.hex;
  CGX_BEGIN_LOAD_XF_REGS(0x1010, 1);  // alpha channel 1
  wgPipe->U32 = chan.hex;

  CGX_LOAD_BP_REG(CGXDefault<TwoTevStageOrders>(0).hex);
  CGX_LOAD_BP_REG(CGXDefault<TevStageCombiner::AlphaCombiner>(0).hex);
  auto cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
  cc.d = TevColorArg::RasColor;
  CGX_LOAD_BP_REG(cc.hex);

  auto genmode = CGXDefault<GenMode>();
  genmode.numtevstages = 0;  // One stage
  CGX_LOAD_BP_REG(genmode.hex);

  PEControl ctrl{.hex = BPMEM_ZCOMPARE << 24};
  ctrl.pixel_format = PixelFormat::RGB8_Z24;
  ctrl.zformat = DepthFormat::ZLINEAR;
  ctrl.early_ztest = 0;
  CGX_LOAD_BP_REG(ctrl.hex);

  auto zmode = CGXDefault<ZMode>();
  CGX_LOAD_BP_REG(zmode.hex);

  CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
  wgPipe->U32 = 1;  // 0 = enable clipping, 1 = disable clipping

  Mtx44 projection;
  guOrtho(projection, 0, 1, 0, 1, -1, 1);
  GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);

  CGX_SetViewport(0.0f, 0.0f, EFB_WIDTH, EFB_HEIGHT, 0.0f, 1.0f);

  // Don't use a custom sample pattern or vertical filter (both interfere with test results)
  GX_SetCopyFilter(GX_FALSE, NULL, GX_FALSE, NULL);

  // Copy twice with clear enabled so that clear_buffer contains a cleared EFB
  CopyToBuffer(GX_CTF_B8, clear_buffer, true);
  CopyToBuffer(GX_CTF_B8, clear_buffer, true);
}

void DrawQuad(u8 blue, s8 padding)
{
  // GXTest::Quad doesn't work
  s8 lower = 0 - padding;
  s8 upper = 1 + padding;
  GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
  GX_Position2s8(lower, lower);
  GX_Color4u8(  0,   0, blue, 255);
  GX_Position2s8(lower, upper);
  GX_Color4u8(  0, 255, blue, 255);
  GX_Position2s8(upper, upper);
  GX_Color4u8(255, 255, blue, 255);
  GX_Position2s8(upper, lower);
  GX_Color4u8(255,   0, blue, 255);
  GX_End();
}

void DrawQuads()
{
  // Lighter background - this is outside of the viewport
  // Currently removed by clipping, and not tested for at all.
  // TODO: Add additional testing for this, once the behavior of clipping is better understood
  DrawQuad(BACKGROUND_BLUE, 1);

  // Darker foreground, in the viewport
  DrawQuad(VIEWPORT_BLUE, 0);
}

u8 ReadSingleChannelTexture(const u8* data, int x, int y, int width)
{
  // See GX_GetTexBufferSize
  int xshift = 3;
  int yshift = 2;
  int blocksize = 32;
  int xmask = (1 << xshift) - 1;
  int ymask = (1 << yshift) - 1;

  u16 xBlk = x >> xshift;
  u16 yBlk = y >> yshift;
  u16 widthBlks = (width + xmask) >> xshift;
  u32 base = (yBlk * widthBlks + xBlk) * blocksize;
  u16 blkX = x & xmask;
  u16 blkY = y & ymask;
  u32 blkOff = (blkY << xshift) + blkX;

  return data[base + blkOff];
}

// These include unused bits
#define MAX_POS 0x1000
#define POS_MASK 0xfff
#define POS_REAL_MASK 0x7ff  // Actual mask that seems to be used by hardware
#define MAX_OFFSET 0x800
#define OFFSET_MASK 0x3ff
#define OFFSET_REAL_MASK 0x1ff  // Actual mask that seems to be used by hardware

struct Scissor
{
  constexpr Scissor(int x0, int y0, int x1, int y1, int x_off, int y_off) :
    x0(x0 & POS_MASK), y0(y0 & POS_MASK), x1(x1 & POS_MASK), y1(y1 & POS_MASK),
    x_off((x_off >> 1) & OFFSET_MASK), y_off((y_off >> 1) & OFFSET_MASK) {}

  // Inclusive ranges [x0, x1] and [y0, y1]
  int x0;
  int y0;
  int x1;
  int y1;
  int x_off;
  int y_off;

  void Apply() const
  {
    // GX_SetScissor and GX_SetScissorBoxOffset internally add an offset.
    // We want the raw registers instead.
    // Also, these fields are normally treated as 11 bits (though libogc uses 12 for y1);
    // however, to test for wrapping let's use the full 12 bits.
    // Note that the masks have been applied in the constructor.
    u32 tl = BPMEM_SCISSORTL << 24 | (x0 << 12) | (y0);
    u32 br = BPMEM_SCISSORBR << 24 | (x1 << 12) | (y1);
    // This field is treated as 10 bits, but testing seems to indicate that it's 9 bits.
    // We already shifted the offsets in the constructor, so no need to do it here.
    u32 off = BPMEM_SCISSOROFFSET << 24 | (y_off << 10) | (x_off);
    CGX_LOAD_BP_REG(tl);
    CGX_LOAD_BP_REG(br);
    CGX_LOAD_BP_REG(off);
  }
};
template <>
struct fmt::formatter<Scissor>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template <typename FormatContext>
  auto format(const Scissor& scissor, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(),
                          "x0 {:4d} y0 {:4d} x1 {:4d} y1 {:4d} xOff {:4d} yOff {:4d}",
                          scissor.x0, scissor.y0, scissor.x1, scissor.y1,
                          scissor.x_off<<1, scissor.y_off<<1);
  }
};

template<typename T>
struct Rect
{
  constexpr Rect(T x0, T y0, T x1, T y1) : x0(x0), y0(y0), x1(x1), y1(y1) {}

  // Half-open ranges [x0, x1) and [y0, y1)
  T x0;
  T y0;
  T x1;
  T y1;

  constexpr Rect<T> Offset(T x, T y) const
  {
    return Rect(x0 + x, y0 + y, x1 + x, y1 + y);
  }

  constexpr Rect<T> Intersect(const Rect<T>& other) const
  {
    T nx0 = std::clamp(x0, other.x0, other.x1);
    T ny0 = std::clamp(y0, other.y0, other.y1);
    T nx1 = std::clamp(x1, other.x0, other.x1);
    T ny1 = std::clamp(y1, other.y0, other.y1);

    return Rect(nx0, ny0, nx1, ny1);
  }

  constexpr T Width() const { return x1 - x0; }
  constexpr T Height() const { return y1 - y0; }
  constexpr T Area() const { return Width() * Height(); }
};

struct ScissorRange
{
  constexpr ScissorRange(s32 offset, u32 start, u32 end) : offset(offset), start(start), end(end) {}
  const s32 offset;
  const u32 start;
  const u32 end;
};

struct ScissorRect
{
  constexpr ScissorRect(ScissorRange x_range, ScissorRange y_range)
      :  // Rectangle ctor takes x0, y0, x1, y1.
        rect(x_range.start, y_range.start, x_range.end, y_range.end), x_off(x_range.offset),
        y_off(y_range.offset) {}

  Rect<u32> rect;
  s32 x_off;
  s32 y_off;

  int GetArea() const;
};


static std::vector<ScissorRange> ComputeScissorRanges(u32 start, u32 end, s32 offset, u32 efb_dim)
{
  std::vector<ScissorRange> ranges;

  for (s32 extra_off = -4096; extra_off <= 4096; extra_off += 1024)
  {
    s32 new_off = offset + extra_off;
    u32 new_start = std::clamp<s32>(start - new_off, 0, efb_dim);
    u32 new_end = std::clamp<s32>(end - new_off + 1, 0, efb_dim);
    if (new_start < new_end)
    {
      ranges.emplace_back(new_off, new_start, new_end);
    }
  }

  return ranges;
}

static std::vector<ScissorRect> ComputeScissorRects(const Scissor& scissor)
{
  std::vector<ScissorRect> result;

  u32 left = scissor.x0 & POS_REAL_MASK;
  u32 top = scissor.y0 & POS_REAL_MASK;
  u32 right = scissor.x1 & POS_REAL_MASK;
  u32 bottom = scissor.y1 & POS_REAL_MASK;
  u32 x_off = (scissor.x_off & OFFSET_REAL_MASK) << 1;
  u32 y_off = (scissor.y_off & OFFSET_REAL_MASK) << 1;
  // When left > right or top > bottom, nothing renders (even with wrapping from the offsets)
  if (left > right || top > bottom)
    return result;

  std::vector<ScissorRange> x_ranges = ComputeScissorRanges(left, right, x_off, EFB_WIDTH);
  std::vector<ScissorRange> y_ranges = ComputeScissorRanges(top, bottom, y_off, EFB_HEIGHT);

  result.reserve(x_ranges.size() * y_ranges.size());

  // Now we need to form actual rectangles from the x and y ranges,
  // which is a simple Cartesian product of x_ranges_clamped and y_ranges_clamped.
  // Each rectangle is also a Cartesian product of x_range and y_range, with
  // the rectangles being half-open (of the form [x0, x1) X [y0, y1)).
  for (const auto& x_range : x_ranges)
  {
    for (const auto& y_range : y_ranges)
    {
      result.emplace_back(x_range, y_range);
    }
  }

  return result;
}

std::vector<Rect<u32>> GetExpectedViewportRects(const Scissor& scissor)
{
  std::vector<ScissorRect> scissor_rects = ComputeScissorRects(scissor);
  Rect<u32> viewport(342, 342, 342+EFB_WIDTH, 342+EFB_HEIGHT);

  std::vector<Rect<u32>> result;

  for (const auto& scissor_rect : scissor_rects)
  {
    auto intersection = scissor_rect.rect.Offset(scissor_rect.x_off, scissor_rect.y_off).Intersect(viewport).Offset(-scissor_rect.x_off, -scissor_rect.y_off);
    if (intersection.Area() > 0)
    {
      result.push_back(intersection);
    }
  }

  return result;
}

bool InViewport(s32 x, s32 y)
{
  if (x < 0 || x >= EFB_WIDTH || y < 0 || y >= EFB_HEIGHT)
    return false;
  else
    return ReadSingleChannelTexture(test_buffer, x, y, EFB_WIDTH) == VIEWPORT_BLUE;
}

bool IsTestBufferCleared()
{
  if (memcmp(test_buffer, clear_buffer, EFB_WIDTH*EFB_HEIGHT) == 0)
    return true;

  // Log the different pixels to make it clear if it's just a random stray pixel or an actual rectangle.
  // This code exists because one of my Wiis randomly (and nondeterministically)
  // has a few stray pixels show up.  This may be due to failing RAM.
  for (int y = 0; y < EFB_HEIGHT; y++)
  {
    for (int x = 0; x < EFB_WIDTH; x++)
    {
      u8 expected = ReadSingleChannelTexture(clear_buffer, x, y, EFB_WIDTH);
      u8 actual = ReadSingleChannelTexture(test_buffer, x, y, EFB_WIDTH);
      if (expected != actual)
      {
        network_printf("Test buffer wasn't cleared at (%d, %d): expected %d, got %d\n", x, y, expected, actual);
      }
    }
  }
  return false;
}

bool CheckViewportRect(const Scissor& scissor, Rect<u32> rect)
{
  u32 x0 = rect.x0;
  u32 x1 = rect.x1 - 1;
  u32 y0 = rect.y0;
  u32 y1 = rect.y1 - 1;
  bool tl = InViewport(x0, y0);
  bool tr = InViewport(x1, y0);
  bool bl = InViewport(x0, y1);
  bool br = InViewport(x1, y1);

  if (!(tl || tr || bl || br))
  {
    // Nothing at any of the corners
    DO_TEST(false, "{}: Expected rect at {} {} {} {}", scissor, x0, y0, x1, y1);
    return false;
  }
  else if (tl && tr && bl && br)
  {
    // All corners are occupied successfully.  Check to make sure the rect isn't too small
    bool left  = InViewport(x0 - 1, y0) || InViewport(x0 - 1, y1);
    bool right = InViewport(x1 + 1, y0) || InViewport(x1 + 1, y1);
    bool top   = InViewport(x0, y0 - 1) || InViewport(x1, y0 - 1);
    bool bot   = InViewport(x0, y1 + 1) || InViewport(x1, y1 + 1);
    if (left || right || top || bot)
    {
      DO_TEST(false, "{}: Found rect at {} {} {} {} but actual rect was bigger",
              scissor, x0, y0, x1, y1);
      return false;
    }
    else
    {
      DO_TEST(true, "{}: Found rect at {} {} {} {}", scissor, x0, y0, x1, y1);
      return true;
    }
  }
  else
  {
    // Some of the corners are covered, others aren't
    DO_TEST(false, "{}: Expected rect at {} {} {} {} but actual rect was smaller",
            scissor, x0, y0, x1, y1);
    return false;
  }
}

void DoTest(bool add_gx_offset, int x0, int y0, int x1, int y1, int x_off, int y_off)
{
  if (add_gx_offset) {
    x0 += 342;
    y0 += 342;
    x1 += 342;
    y1 += 342;
    x_off += 342;
    y_off += 342;
  }

  Scissor scissor(x0, y0, x1, y1, x_off, y_off);
  scissor.Apply();

  DrawQuads();

  GXTest::DebugDisplayEfbContents();
  CopyToBuffer(GX_CTF_B8, test_buffer, true);
  CGX_ForcePipelineFlush();

  std::vector<Rect<u32>> expected = GetExpectedViewportRects(scissor);

  CGX_WaitForGpuToFinish();

  if (!expected.empty())
  {
    u32 num_ok = 0;
    for (const auto& rect : expected)
    {
      if (CheckViewportRect(scissor, rect))
        num_ok++;
    }
    DO_TEST(num_ok == expected.size(), "{}: {} of {} rects matched",
            scissor, num_ok, (u32)expected.size());
  }
  else
  {
    DO_TEST(IsTestBufferCleared(), "{}: There should be no rects", scissor);
  }
}

void ScissorTest()
{
  const u32 w = EFB_WIDTH-1;
  const u32 h = EFB_HEIGHT-1;

  START_TEST();  // x offset
  for (int x = 0; x < MAX_OFFSET; x += 2)
    DoTest(false, 0, 0, w, h, x, 0);
  END_TEST();
  START_TEST();  // y offset
  for (int y = 0; y < MAX_OFFSET; y += 2)
    DoTest(false, 0, 0, w, h, 0, y);
  END_TEST();
  START_TEST();  // Diag offset
  for (int i = 0; i < MAX_OFFSET; i += 2)
    DoTest(false, 0, 0, w, h, i, i);
  END_TEST();

  START_TEST();  // x offset gx
  for (int x = 0; x < MAX_OFFSET; x += 2)
    DoTest(true, 0, 0, w, h, x, 0);
  END_TEST();
  START_TEST();  // y offset gx
  for (int y = 0; y < MAX_OFFSET; y += 2)
    DoTest(true, 0, 0, w, h, 0, y);
  END_TEST();
  START_TEST();  // Diag offset gx
  for (int i = 0; i < MAX_OFFSET; i += 2)
    DoTest(true, 0, 0, w, h, i, i);
  END_TEST();

  START_TEST();  // x pos
  for (int x = 0; x < MAX_POS; x++)
    DoTest(false, x, 0, w, h, 0, 0);
  END_TEST();
  START_TEST();  // y pos
  for (int y = 0; y < MAX_POS; y++)
    DoTest(false, 0, y, w, h, 0, 0);
  END_TEST();
  START_TEST();  // Diag pos
  for (int i = 0; i < MAX_POS; i++)
    DoTest(false, i, i, w, h, 0, 0);
  END_TEST();

  START_TEST();  // x pos gx
  for (int x = 0; x < MAX_POS; x++)
    DoTest(true, x, 0, w, h, 0, 0);
  END_TEST();
  START_TEST();  // y pos gx
  for (int y = 0; y < MAX_POS; y++)
    DoTest(true, 0, y, w, h, 0, 0);
  END_TEST();
  START_TEST();  // Diag pos gx
  for (int i = 0; i < MAX_POS; i++)
    DoTest(true, i, i, w, h, 0, 0);
  END_TEST();

  START_TEST();  // x size
  for (int x = 0; x < MAX_POS; x++)
    DoTest(false, 0, 0, x, h, 0, 0);
  END_TEST();
  START_TEST();  // x size 2
  for (int x = 0; x < MAX_POS; x++)
    DoTest(false, 256, 0, x, h, 0, 0);
  END_TEST();
  START_TEST();  // y size
  for (int y = 0; y < MAX_POS; y++)
    DoTest(false, 0, 0, w, y, 0, 0);
  END_TEST();
  START_TEST();  // y size 2
  for (int y = 0; y < MAX_POS; y++)
    DoTest(false, 0, 256, w, y, 0, 0);

  END_TEST();
  START_TEST();  // x size gx
  for (int x = 0; x < MAX_POS; x++)
    DoTest(true, 0, 0, x, h, 0, 0);
  END_TEST();
  START_TEST();  // x size 2 gx
  for (int x = 0; x < MAX_POS; x++)
    DoTest(true, 256, 0, x, h, 0, 0);
  END_TEST();
  START_TEST();  // y size gx
  for (int y = 0; y < MAX_POS; y++)
    DoTest(true, 0, 0, w, y, 0, 0);
  END_TEST();
  START_TEST();  // y size 2 gx
  for (int y = 0; y < MAX_POS; y++)
    DoTest(true, 0, 256, w, y, 0, 0);

  END_TEST();
  START_TEST();  // multi x offset
  for (int x = 0; x < MAX_POS; x++)
    DoTest(false, x, 0, x+w, h, x, 0);
  END_TEST();
  START_TEST();  // multi y offset
  for (int y = 0; y < MAX_POS; y++)
    DoTest(false, 0, y, w, y+h, 0, y);
  END_TEST();
  START_TEST();  // Multi diag offset
  for (int i = 0; i < MAX_POS; i++)
    DoTest(false, i, i, i+w, i+h, i, i);

  END_TEST();
  START_TEST();  // multi neg x offset
  for (int x = 0; x < MAX_POS; x++)
    DoTest(false, -x, 0, w-x, h, x, 0);
  END_TEST();
  START_TEST();  // multi neg y offset
  for (int y = 0; y < MAX_POS; y++)
    DoTest(false, 0, -y, w, h-y, 0, y);
  END_TEST();
  START_TEST();  // Multi neg diag offset
  for (int i = 0; i < MAX_POS; i++)
    DoTest(false, -i, -i, w-i, h-i, i, i);
  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  GXTest::Init();
  ConfigureGX();

  ScissorTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}
