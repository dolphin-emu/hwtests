// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cmath>
#include <fmt/format.h>

#include <ogcsys.h>
#include <wiiuse/wpad.h>
#include "common/hwtests.h"
#include "gxtest/cgx.h"
#include "gxtest/cgx_defaults.h"
#include "gxtest/util.h"

// Use all copy filter values (0-63*3), instead of only 64
#define FULL_COPY_FILTER_COEFS true
// Use all gamma values, instead of just 1.0 (0)
#define FULL_GAMMA true
// Use all pixel formats, instead of just the ones that work
#define FULL_PIXEL_FORMATS false
// Also set the copy filter values for prev and next rows
#define CHECK_PREV_AND_NEXT true

struct CopyFilterTestContext
{
  PixelFormat pixel_fmt;
  GammaCorrection gamma;
  u8 prev_copy_filter_sum;
  u8 copy_filter_sum;
  u8 next_copy_filter_sum;
  bool intensity_fmt;
};
template <>
struct fmt::formatter<CopyFilterTestContext>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template <typename FormatContext>
  auto format(const CopyFilterTestContext& test, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(),
                          "pixel_fmt: {}, gamma: {}, copy filter: {}/{}/{}, intensity: {}",
                          test.pixel_fmt, test.gamma, test.prev_copy_filter_sum,
                          test.copy_filter_sum, test.next_copy_filter_sum, test.intensity_fmt);
  }
};

void SetCopyFilter(u8 prev_copy_filter_sum, u8 copy_filter_sum, u8 next_copy_filter_sum);

GXTest::Vec4<u8> GenerateEFBColor(u16 x, u16 y)
{
  const u8 r = static_cast<u8>(x);
  const u8 g = static_cast<u8>(y == 4 ? x : (y == 3 ? 255 : 0));
  const u8 b = static_cast<u8>(x);
  const u8 a = static_cast<u8>(x);
  return {r, g, b, a};
}

u32 GenerateEFBDepth([[maybe_unused]] u16 x, u16 y)
{
  return y < 4 ? 0x123456 : 0x789abc;
}

static void SetPixelFormat(PixelFormat pixel_fmt)
{
  PEControl ctrl{.hex = BPMEM_ZCOMPARE << 24};
  ctrl.pixel_format = pixel_fmt;
  ctrl.zformat = DepthFormat::ZLINEAR;
  ctrl.early_ztest = false;
  CGX_LOAD_BP_REG(ctrl.hex);
}

static void FillEFB(PixelFormat pixel_fmt)
{
  SetPixelFormat(pixel_fmt);
  CGX_WaitForGpuToFinish();

  // Needed for clear to work properly. GX_CopyTex ors with 0xf, but the top bit indicating update also must be set
  CGX_LOAD_BP_REG(BPMEM_ZMODE << 24 | 0x1f);
  CGX_LOAD_BP_REG(BPMEM_CLEAR_Z << 24 | 0x123456);
  // For some reason, Z isn't cleared properly if the height isn't set to 4 (3 inclusive)
  GXTest::CopyToTestBuffer(0, 0, 255, 3, {.clear = true});
  CGX_LOAD_BP_REG(BPMEM_CLEAR_Z << 24 | 0x789abc);
  GXTest::CopyToTestBuffer(0, 4, 255, 7, {.clear = true});
  CGX_WaitForGpuToFinish();

  CGX_PEPokeDither(false);
  CGX_PEPokeAlphaUpdate(true);
  CGX_PEPokeColorUpdate(true);
  CGX_PEPokeBlendMode(GX_BM_NONE, SrcBlendFactor::Zero, DstBlendFactor::Zero, LogicOp::Set);
  CGX_PEPokeAlphaRead(GX_READ_NONE);
  CGX_PEPokeZMode(false, CompareMode::Always, true);

  // For some reason GX_PokeARGB hangs when using this format
  if (pixel_fmt == PixelFormat::RGB565_Z16)
    return;

  for (u16 x = 0; x < 256; x++)
  {
    for (u16 y = 0; y < 8; y++)
    {
      CGX_PokeARGB(x, y, GenerateEFBColor(x, y), pixel_fmt);
      // GX_PokeZ doesn't seem to work at all
      // CGX_PokeZ(x, y, GenerateEFBDepth(x, y), pixel_fmt);
    }
  }
}

#if FULL_GAMMA
static const std::array<GammaCorrection, 4> GAMMA_VALUES = { GammaCorrection::Gamma1_0, GammaCorrection::Gamma1_7, GammaCorrection::Gamma2_2, GammaCorrection::Invalid2_2 };
#else
static const std::array<GammaCorrection, 1> GAMMA_VALUES = { GammaCorrection::Gamma1_0 };
#endif

#if FULL_PIXEL_FORMATS
static const std::array<PixelFormat, 8> PIXEL_FORMATS = { PixelFormat::RGB8_Z24, PixelFormat::RGBA6_Z24, PixelFormat::RGB565_Z16, PixelFormat::Z24, PixelFormat::Y8, PixelFormat::U8, PixelFormat::V8, PixelFormat::YUV420 };
#else
// These formats work on Dolphin and on real hardware
static const std::array<PixelFormat, 3> PIXEL_FORMATS = { PixelFormat::RGB8_Z24, PixelFormat::RGBA6_Z24, PixelFormat::Z24 };
#endif

// Applies to current row
#define MAX_COPY_FILTER_CUR 63*3
#define MAX_COPY_FILTER_PREV 63*2
#define MAX_COPY_FILTER_NEXT 63*2
void SetCopyFilter(const CopyFilterTestContext& ctx)
{
  SetCopyFilter(ctx.prev_copy_filter_sum, ctx.copy_filter_sum, ctx.next_copy_filter_sum);
}

void SetCopyFilter(u8 prev_copy_filter_sum, u8 copy_filter_sum, u8 next_copy_filter_sum)
{
  // Each field in the copy filter ranges from 0-63, and the middle 3 values
  // all apply to the current row of pixels.  This means that up to 63*3
  // can be used for the current row (while 63*2 is the max for the others).
  // If the value is outside of that range, we just treat it as the maximum.
  CopyFilterCoefficients coef;
  coef.Low = BPMEM_COPYFILTER0 << 24;
  coef.High = BPMEM_COPYFILTER1 << 24;

  // Previous row (w0, w1)
  coef.w0 = std::min<u8>(prev_copy_filter_sum, 63);
  if (prev_copy_filter_sum > 63)
    coef.w1 = std::min<u8>(prev_copy_filter_sum - 63, 63);
  // Current row (w2, w3, w4)
  coef.w3 = std::min<u8>(copy_filter_sum, 63);
  if (copy_filter_sum > 63)
    coef.w2 = std::min<u8>(copy_filter_sum - 63, 63);
  if (copy_filter_sum > 63 * 2)
    coef.w4 = std::min<u8>(copy_filter_sum - 63 * 2, 63);
  // Next row (w5, w6)
  coef.w5 = std::min<u8>(next_copy_filter_sum, 63);
  if (next_copy_filter_sum > 63)
    coef.w6 = std::min<u8>(next_copy_filter_sum - 63, 63);

  CGX_LOAD_BP_REG(coef.Low);
  CGX_LOAD_BP_REG(coef.High);
}

u8 SixBit(u8 value)
{
  return (value & 0xfc) | ((value & 0xc0) >> 6);
}

u8 FiveBit(u8 value)
{
  return (value & 0xf8) | ((value & 0xe0) >> 5);
}

u8 Y8Transform(u8 value)
{
  if (value <= 1)
    return 0;
  else
    return 255;
}

u8 U8Transform(u8 value)
{
  if (value <= 1)
  {
    return 0;
  }
  else if (value & 1)
  {
    return 255;
  }
  else
  {
    /*
    switch (value & 0xc0)
    {
    case 0x00: return (value & 2) ? 44 : 12;
    case 0x40: return (value & 2) ? 109 : 77;
    case 0x80: return (value & 2) ? 174 : 142;
    case 0xc0: return (value & 2) ? 239 : 207;
    }
    */
    return 12 + 65 * ((value & 0xc0) >> 6) + 32 * ((value & 2) >> 1);
  }
}

u8 V8Transform(u8 value)
{
  if (value & 1)
    return value;
  else
    return FiveBit(value);
}

GXTest::Vec4<u8> PredictEfbColor(u16 x, u16 y, PixelFormat pixel_fmt, bool efb_peek = false)
{
  GXTest::Vec4<u8> color = GenerateEFBColor(x, y);
  switch (pixel_fmt)
  {
  case PixelFormat::RGB8_Z24:
  case PixelFormat::YUV420:
  default:
    return {color.r, color.g, color.b, 255};
  case PixelFormat::RGBA6_Z24:
    return {SixBit(color.r), SixBit(color.g), SixBit(color.b), SixBit(color.a)};
  case PixelFormat::RGB565_Z16:
    // Not fully tested due to the EFB poke issue
    return {FiveBit(color.r), SixBit(color.g), FiveBit(color.b), 255};
  case PixelFormat::Z24:
  {
    const u32 depth = GenerateEFBDepth(x, y);
    const u8 r = (depth >> 16) & 255;
    const u8 g = (depth >> 8) & 255;
    const u8 b = depth & 255;
    return {r, g, b, 255};
  }
  // These worked when setting r, g, and b to the same value, but don't work anymore
  case PixelFormat::Y8:
    if (!efb_peek)
    {
      // This gives correct results for texture copies...
      return {color.r, color.g, color.b, 255};
    }
    else
    {
      // But this is the logic behind peeks?
      return {Y8Transform(color.r), Y8Transform(color.g), Y8Transform(color.b), 255};
    }
  case PixelFormat::U8:
    if (efb_peek)
    {
      // This only works for EFB peeks
      return {U8Transform(color.r), U8Transform(color.g), U8Transform(color.b), 255};
    }
    else
    {
      // Dunno
      return {0, 0, 0, 255};
    }
  case PixelFormat::V8:
    // This works but makes no sense
    return {V8Transform(color.r), V8Transform(color.g), V8Transform(color.b), 255};
  }
}

u8 Predict(u8 prev, u8 current, u8 next, const CopyFilterTestContext& ctx)
{
  // Apply copy filter
  u32 prediction_i = static_cast<u32>(prev) * static_cast<u32>(ctx.prev_copy_filter_sum);
  prediction_i += static_cast<u32>(current) * static_cast<u32>(ctx.copy_filter_sum);
  prediction_i += static_cast<u32>(next) * static_cast<u32>(ctx.next_copy_filter_sum);
  prediction_i >>= 6;  // Divide by 64
  // The clamping seems to happen in the range[0, 511]; if the value is outside
  // an overflow will still occur.  This happens if copy_filter_sum >= 128.
  prediction_i &= 0x1ffu;
  prediction_i = std::min(prediction_i, 0xffu);
  // Apply gamma
  if (ctx.gamma != GammaCorrection::Gamma1_0)
  {
    // Convert from [0-255] to [0-1]
    float prediction_f = static_cast<float>(prediction_i) / 255.f;
    switch (ctx.gamma)
    {
    case GammaCorrection::Gamma1_7:
      prediction_f = std::pow(prediction_f, 1 / 1.7f);
      break;
    case GammaCorrection::Gamma2_2:
    case GammaCorrection::Invalid2_2:
    default:
      prediction_f = std::pow(prediction_f, 1 / 2.2f);
      break;
    }
    // Due to how exponentials work, std::pow will always map from [0, 1] to [0, 1],
    // so no overflow can occur here.  (pow is continuous, 0^x is 0 for x > 0,
    // and 1^x is 1, so y in [0, 1] has y^x in [0, 1])
    // Convert back from [0, 1] to [0, 255]
    prediction_i = static_cast<u32>(std::round(prediction_f * 255.f));
  }
  return static_cast<u8>(prediction_i);
}

GXTest::Vec4<u8> Predict(GXTest::Vec4<u8> prev_efb_color, GXTest::Vec4<u8> efb_color, GXTest::Vec4<u8> next_efb_color, const CopyFilterTestContext& ctx)
{
  const u8 r = Predict(prev_efb_color.r, efb_color.r, next_efb_color.r, ctx);
  const u8 g = Predict(prev_efb_color.g, efb_color.g, next_efb_color.g, ctx);
  const u8 b = Predict(prev_efb_color.b, efb_color.b, next_efb_color.b, ctx);
  const u8 a = efb_color.a;  // Copy filter doesn't apply to alpha
  if (ctx.intensity_fmt)
  {
    // BT.601 conversion
    const u16 y = +66 * r + 129 * g + +25 * b;
    const s16 u = -38 * r + -74 * g + 112 * b;
    const s16 v = 112 * r + -94 * g + -18 * b;
    const u8 y_round = static_cast<u8>((y >> 8) + ((y >> 7) & 1) + 16);
    const u8 u_round = static_cast<u8>((u >> 8) + ((u >> 7) & 1) + 128);
    const u8 v_round = static_cast<u8>((v >> 8) + ((v >> 7) & 1) + 128);
    return { y_round, u_round, v_round, a };
  }
  else
  {
    return { r, g, b, a };
  }
}

void CopyFilterTest(const CopyFilterTestContext& ctx)
{
  START_TEST();

  SetCopyFilter(ctx);
  GXTest::CopyToTestBuffer(0, 0, 255, 7, {.gamma = ctx.gamma, .intensity_fmt = ctx.intensity_fmt, .auto_conv = ctx.intensity_fmt});
  CGX_WaitForGpuToFinish();

  for (u16 x = 0; x < 256; x++)
  {
    // Reduce bit depth based on the format
    GXTest::Vec4<u8> prev_efb_color = PredictEfbColor(x, 3, ctx.pixel_fmt);
    GXTest::Vec4<u8> efb_color = PredictEfbColor(x, 4, ctx.pixel_fmt);
    GXTest::Vec4<u8> next_efb_color = PredictEfbColor(x, 5, ctx.pixel_fmt);
    // Make predictions based on the copy filter and gamma
    GXTest::Vec4<u8> expected = Predict(prev_efb_color, efb_color, next_efb_color, ctx);
    GXTest::Vec4<u8> actual = GXTest::ReadTestBuffer(x, 4, 256);
    DO_TEST(actual.r == expected.r, "Predicted wrong red   value for x {} with {}: expected {} from {}/{}/{}, was {}", x, ctx, expected.r, prev_efb_color.r, efb_color.r, next_efb_color.r, actual.r);
    DO_TEST(actual.g == expected.g, "Predicted wrong green value for x {} with {}: expected {} from {}/{}/{}, was {}", x, ctx, expected.g, prev_efb_color.g, efb_color.g, next_efb_color.g, actual.g);
    DO_TEST(actual.b == expected.b, "Predicted wrong blue  value for x {} with {}: expected {} from {}/{}/{}, was {}", x, ctx, expected.b, prev_efb_color.b, efb_color.b, next_efb_color.b, actual.b);
    DO_TEST(actual.a == expected.a, "Predicted wrong alpha value for x {} with {}: expected {} from {}/{}/{}, was {}", x, ctx, expected.a, prev_efb_color.a, efb_color.a, next_efb_color.a, actual.a);
  }

  END_TEST();
}

void CheckEFB(PixelFormat pixel_fmt)
{
  // For some reason GX_PokeARGB hangs when using this format
  if (pixel_fmt == PixelFormat::RGB565_Z16)
    return;

  START_TEST();

  if (pixel_fmt != PixelFormat::Z24)
  {
    for (u16 x = 0; x < 256; x++)
    {
      for (u16 y = 0; y < 8; y++)
      {
        GXTest::Vec4<u8> actual = CGX_PeekARGB(x, y, pixel_fmt);
        GXTest::Vec4<u8> expected = PredictEfbColor(x, y, pixel_fmt, true);

        DO_TEST(actual.r == expected.r, "Predicted wrong red   value for x {} y {} pixel format {} using peeks: expected {}, was {}", x, y, pixel_fmt, expected.r, actual.r);
        DO_TEST(actual.g == expected.g, "Predicted wrong green value for x {} y {} pixel format {} using peeks: expected {}, was {}", x, y, pixel_fmt, expected.g, actual.g);
        DO_TEST(actual.b == expected.b, "Predicted wrong blue  value for x {} y {} pixel format {} using peeks: expected {}, was {}", x, y, pixel_fmt, expected.b, actual.b);
        DO_TEST(actual.a == expected.a, "Predicted wrong alpha value for x {} y {} pixel format {} using peeks: expected {}, was {}", x, y, pixel_fmt, expected.a, actual.a);
      }
    }
  }
  else
  {
    for (u16 x = 0; x < 256; x++)
    {
      for (u16 y = 0; y < 8; y++)
      {
        u32 actual = CGX_PeekZ(x, y, pixel_fmt);
        u32 expected = GenerateEFBDepth(x, y);

        DO_TEST(actual == expected, "Predicted wrong z value for x {} y {} pixel format {} using peeks: expected {}, was {}", x, y, pixel_fmt, expected, actual);
      }
    }
  }

  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  GXTest::Init();
  network_printf("FULL_COPY_FILTER_COEFS: %s\n", FULL_COPY_FILTER_COEFS ? "true" : "false");
  network_printf("FULL_GAMMA: %s\n", FULL_GAMMA ? "true" : "false");
  network_printf("FULL_PIXEL_FORMATS: %s\n", FULL_PIXEL_FORMATS ? "true" : "false");

  for (PixelFormat pixel_fmt : PIXEL_FORMATS)
  {
    FillEFB(pixel_fmt);
    CheckEFB(pixel_fmt);

#if FULL_COPY_FILTER_COEFS
    for (u8 copy_filter_sum = 0; copy_filter_sum <= MAX_COPY_FILTER_CUR; copy_filter_sum++)
#else
    const u8 copy_filter_sum = 64;
#endif
    {
      for (GammaCorrection gamma : GAMMA_VALUES)
      {
#if CHECK_PREV_AND_NEXT
        // Start at 2 to avoid boring case of cur_row = prev_row = next_row = false
        // which would encode all copy filter parameters as 0
        // That case is already covered by copy_filter_sum = 0 anyways
        for (u32 flags = 2; flags < 16; flags++)
#else
        for (u32 flags = 2; flags < 4; flags++)
#endif
        {
          const bool intensity_fmt = (flags & 1) != 0;
          const bool cur_row = (flags & 2) != 0;
          const bool prev_row = (flags & 4) != 0;
          const bool next_row = (flags & 8) != 0;

          const u8 prev_sum = std::min(prev_row ? copy_filter_sum : 0, MAX_COPY_FILTER_PREV);
          const u8 cur_sum = std::min(cur_row ? copy_filter_sum : 0, MAX_COPY_FILTER_CUR);
          const u8 next_sum = std::min(next_row ? copy_filter_sum : 0, MAX_COPY_FILTER_NEXT);

          CopyFilterTest({pixel_fmt, gamma, prev_sum, cur_sum, next_sum, intensity_fmt});

          WPAD_ScanPads();
          if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
            goto done;
        }
      }
    }
  }
done:

  report_test_results();
  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}
