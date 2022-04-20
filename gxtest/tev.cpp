// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <initializer_list>
#include <math.h>
#include <ogcsys.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>
#include "common/hwtests.h"
#include "gxtest/cgx.h"
#include "gxtest/cgx_defaults.h"
#include "gxtest/util.h"

int TevCombinerExpectation(int a, int b, int c, int d, TevScale scale, TevBias bias, TevOp op, bool clamp)
{
  a &= 255;
  b &= 255;
  c &= 255;

  // TODO: Does not handle compare mode, yet
  c = c + (c >> 7);
  u16 lshift = (scale == TevScale::Scale2) ? 1 : (scale == TevScale::Scale4) ? 2 : 0;
  u16 rshift = (scale == TevScale::Divide2) ? 1 : 0;
  int round_bias = (scale == TevScale::Divide2) ? 0 : ((op == TevOp::Sub) ? 127 : 128);
  int expected = (((a * (256 - c) + b * c) << lshift) + round_bias) >> 8;  // lerp
  expected = (d << lshift) + expected * ((op == TevOp::Sub) ? (-1) : 1);
  expected += ((bias == TevBias::SubHalf) ? -128 : (bias == TevBias::AddHalf) ? 128 : 0) << lshift;
  expected >>= rshift;
  if (clamp)
    expected = (expected < 0) ? 0 : (expected > 255) ? 255 : expected;
  else
    expected = (expected < -1024) ? -1024 : (expected > 1023) ? 1023 : expected;
  return expected;
}

void TevCombinerTest()
{
  START_TEST();

  CGX_LOAD_BP_REG(CGXDefault<TwoTevStageOrders>(0).hex);

  CGX_BEGIN_LOAD_XF_REGS(0x1009, 1);
  wgPipe->U32 = 1;  // 1 color channel

  LitChannel chan;
  chan.hex = 0;
  chan.matsource = 1;                 // from vertex
  CGX_BEGIN_LOAD_XF_REGS(0x100e, 1);  // color channel 1
  wgPipe->U32 = chan.hex;
  CGX_BEGIN_LOAD_XF_REGS(0x1010, 1);  // alpha channel 1
  wgPipe->U32 = chan.hex;

  auto ac = CGXDefault<TevStageCombiner::AlphaCombiner>(0);
  CGX_LOAD_BP_REG(ac.hex);

  // Test if we can reliably extract all bits of the tev combiner output...
  auto tevreg = CGXDefault<TevReg>(1, false);  // c0
  for (int i = 0; i < 2; ++i)
    for (tevreg.ra.red = -1024; tevreg.ra.red != 1023; tevreg.ra.red = tevreg.ra.red + 1)
    {
      CGX_LOAD_BP_REG(tevreg.ra.hex);
      CGX_LOAD_BP_REG(tevreg.bg.hex);

      auto genmode = CGXDefault<GenMode>();
      genmode.numtevstages = 0;  // One stage
      CGX_LOAD_BP_REG(genmode.hex);

      auto cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
      cc.d = TevColorArg::Color0;
      CGX_LOAD_BP_REG(cc.hex);

      PEControl ctrl;
      ctrl.hex = BPMEM_ZCOMPARE << 24;
      ctrl.zformat = DepthFormat::ZLINEAR;
      ctrl.early_ztest = false;
      if (i == 0)
      {
        // 8 bits per channel: No worries about GetTevOutput making
        // mistakes when writing to framebuffer or when performing
        // an EFB copy.
        ctrl.pixel_format = PixelFormat::RGB8_Z24;
        CGX_LOAD_BP_REG(ctrl.hex);

        int result = GXTest::GetTevOutput(genmode, cc, ac).r;

        DO_TEST(result == tevreg.ra.red, "Got {}, expected {}", result, tevreg.ra.red);
      }
      else
      {
        // TODO: This doesn't quite work, yet.
        break;

        // 6 bits per channel: Implement GetTevOutput functionality
        // manually, to verify how tev output is truncated to 6 bit
        // and how EFB copies upscale that to 8 bit again.
        ctrl.pixel_format = PixelFormat::RGBA6_Z24;
        CGX_LOAD_BP_REG(ctrl.hex);

        GXTest::Quad().AtDepth(1.0).ColorRGBA(255, 255, 255, 255).Draw();
        GXTest::CopyToTestBuffer(0, 0, 99, 9);
        CGX_ForcePipelineFlush();
        CGX_WaitForGpuToFinish();
        u16 result = GXTest::ReadTestBuffer(5, 5, 100).r;

        int expected = (((tevreg.ra.red + 1) >> 2) & 0xFF) << 2;
        expected = expected | (expected >> 6);
        DO_TEST(result == expected, "Run {}: Got {}, expected {}", tevreg.ra.red, result,
                expected);
      }
    }

  // Now: Randomized testing of tev combiners.
  for (int i = 0x000000; i < 0x000F000; ++i)
  {
    if ((i & 0xFF00) == i)
      network_printf("progress: %x\n", i);

    auto genmode = CGXDefault<GenMode>();
    genmode.numtevstages = 0;  // One stage
    CGX_LOAD_BP_REG(genmode.hex);

    // Randomly configured TEV stage, output in PREV.
    auto cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
    cc.a = TevColorArg::Color0;
    cc.b = TevColorArg::Color1;
    cc.c = TevColorArg::Color2;
    cc.d = TevColorArg::Zero;  // TevColorArg::PrevColor; // NOTE: TevColorArg::PrevColor doesn't actually seem
                              // to fetch its data from PREV when used in the first stage?
    cc.scale = static_cast<TevScale>(rand() % 4);
    cc.bias = static_cast<TevBias>(rand() % 3);
    cc.op = static_cast<TevOp>(rand() % 2);
    cc.clamp = static_cast<bool>(rand() % 2);
    CGX_LOAD_BP_REG(cc.hex);

    int a = -1024 + (rand() % 2048);
    int b = -1024 + (rand() % 2048);
    int c = -1024 + (rand() % 2048);
    int d = 0;                              //-1024 + (rand() % 2048);
    tevreg = CGXDefault<TevReg>(1, false);  // c0
    tevreg.ra.red = a;
    CGX_LOAD_BP_REG(tevreg.ra.hex);
    CGX_LOAD_BP_REG(tevreg.bg.hex);
    tevreg = CGXDefault<TevReg>(2, false);  // c1
    tevreg.ra.red = b;
    CGX_LOAD_BP_REG(tevreg.ra.hex);
    CGX_LOAD_BP_REG(tevreg.bg.hex);
    tevreg = CGXDefault<TevReg>(3, false);  // c2
    tevreg.ra.red = c;
    CGX_LOAD_BP_REG(tevreg.ra.hex);
    CGX_LOAD_BP_REG(tevreg.bg.hex);
    tevreg = CGXDefault<TevReg>(0, false);  // prev
    tevreg.ra.red = d;
    CGX_LOAD_BP_REG(tevreg.ra.hex);
    CGX_LOAD_BP_REG(tevreg.bg.hex);

    PEControl ctrl;
    ctrl.hex = BPMEM_ZCOMPARE << 24;
    ctrl.pixel_format = PixelFormat::RGB8_Z24;
    ctrl.zformat = DepthFormat::ZLINEAR;
    ctrl.early_ztest = false;
    CGX_LOAD_BP_REG(ctrl.hex);

    int result = GXTest::GetTevOutput(genmode, cc, ac).r;

    int expected = TevCombinerExpectation(a, b, c, d, cc.scale, cc.bias, cc.op, cc.clamp);
    DO_TEST(result == expected, "Mismatch on a={}, b={}, c={}, d={}, shift={}, bias={}, op={}, "
                                "clamp={}: expected {}, got {}",
            a, b, c, d, cc.scale, cc.bias, cc.op, cc.clamp, expected, result);

    WPAD_ScanPads();

    if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
      break;
  }

  // Testing compare mode: (a.r > b.r) ? c.a : 0
  // One of the following will be the case for the alpha combiner:
  // (1) a.r will be assigned the value of c2.r (color combiner setting)
  // (2) a.r will be assigned the value of c0.r (alpha combiner setting)
  // If (1) is the case, the first run of this test will return black and
  // the second one will return red. If (2) is the case, the test will
  // always return black.
  // Indeed, this test shows that the alpha combiner reads the a and b
  // inputs from the color combiner setting, i.e. scenario (1) is
  // accurate to hardware behavior.
  for (int i = 0; i < 2; ++i)
  {
    auto genmode = CGXDefault<GenMode>();
    genmode.numtevstages = 0;  // One stage
    CGX_LOAD_BP_REG(genmode.hex);

    auto cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
    cc.a = TevColorArg::Color2;
    cc.b = TevColorArg::Color1;
    CGX_LOAD_BP_REG(cc.hex);

    auto ac = CGXDefault<TevStageCombiner::AlphaCombiner>(0);
    ac.bias = TevBias::Compare;
    ac.a = TevAlphaArg::Alpha0;  // different from color combiner
    ac.b = TevAlphaArg::Alpha1;  // same as color combiner
    ac.c = TevAlphaArg::Alpha2;
    CGX_LOAD_BP_REG(ac.hex);

    PEControl ctrl;
    ctrl.hex = BPMEM_ZCOMPARE << 24;
    ctrl.pixel_format = PixelFormat::RGBA6_Z24;
    ctrl.zformat = DepthFormat::ZLINEAR;
    ctrl.early_ztest = false;
    CGX_LOAD_BP_REG(ctrl.hex);

    tevreg = CGXDefault<TevReg>(1, false);  // c0
    tevreg.ra.red = 127;                       // 127 is always NOT less than 127.
    CGX_LOAD_BP_REG(tevreg.ra.hex);
    CGX_LOAD_BP_REG(tevreg.bg.hex);

    tevreg = CGXDefault<TevReg>(2, false);  // c1
    tevreg.ra.red = 127;
    CGX_LOAD_BP_REG(tevreg.ra.hex);
    CGX_LOAD_BP_REG(tevreg.bg.hex);

    tevreg = CGXDefault<TevReg>(3, false);  // c2
    tevreg.ra.red = 127 + i;                   // 127+i is less than 127 iff i>0.
    tevreg.ra.alpha = 255;
    CGX_LOAD_BP_REG(tevreg.ra.hex);
    CGX_LOAD_BP_REG(tevreg.bg.hex);

    int result = GXTest::GetTevOutput(genmode, cc, ac).a;
    int expected = (i == 1) ? 255 : 0;
    DO_TEST(result == expected, "Mismatch on run {}: expected {}, got {}", i, expected, result);
  }

  END_TEST();
}

void KonstTest()
{
  START_TEST();

  CGX_LOAD_BP_REG(CGXDefault<TwoTevStageOrders>(0).hex);

  CGX_BEGIN_LOAD_XF_REGS(0x1009, 1);
  wgPipe->U32 = 1;  // 1 color channel

  LitChannel chan;
  chan.hex = 0;
  chan.matsource = 0;  // from register
  chan.ambsource = 0;  // from register
  chan.enablelighting = false;
  CGX_BEGIN_LOAD_XF_REGS(0x100e, 1);  // color channel 1
  wgPipe->U32 = chan.hex;
  CGX_BEGIN_LOAD_XF_REGS(0x1010, 1);  // alpha channel 1
  wgPipe->U32 = chan.hex;

  auto genmode = CGXDefault<GenMode>();
  genmode.numtevstages = 1;  // Two stages
  CGX_LOAD_BP_REG(genmode.hex);

  PEControl ctrl;
  ctrl.hex = BPMEM_ZCOMPARE << 24;
  ctrl.pixel_format = PixelFormat::RGB8_Z24;
  ctrl.zformat = DepthFormat::ZLINEAR;
  ctrl.early_ztest = false;
  CGX_LOAD_BP_REG(ctrl.hex);

  CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
  wgPipe->U32 = 0;  // 0 = enable clipping, 1 = disable clipping

  // Set up "konst" colors with recognizable values.
  for (int i = 0; i < 4; ++i)
  {
    auto tevreg = CGXDefault<TevReg>(i, true);
    tevreg.ra.red = 10 + 50 * i;
    tevreg.bg.green = 20 + 50 * i;
    tevreg.bg.blue = 30 + 50 * i;
    tevreg.ra.alpha = 40 + 50 * i;
    CGX_LOAD_BP_REG(tevreg.ra.hex);
    CGX_LOAD_BP_REG(tevreg.bg.hex);
  }

  // Make sure blending is set appropriately (no blending).
  BlendMode bm;
  bm.hex = (BPMEM_BLENDMODE << 24);
  bm.blendenable = false;
  bm.logicopenable = false;
  bm.dither = true;
  bm.colorupdate = true;
  bm.alphaupdate = true;
  bm.dstfactor = DstBlendFactor::InvSrcAlpha;
  bm.srcfactor = SrcBlendFactor::SrcAlpha;
  bm.subtract = false;
  bm.logicmode = LogicOp::Clear;
  CGX_LOAD_BP_REG(bm.hex);

  // Test for values returned for "konst" TEV inputs.  Goes through
  // all the possible values for kasel and kcsel.
  for (int step = 0; step < 64; ++step)
  {
    auto zmode = CGXDefault<ZMode>();
    CGX_LOAD_BP_REG(zmode.hex);

    int test_x = 125, test_y = 25;  // Somewhere within the viewport

    // First stage pulls in konst values.
    CGX_SetViewport(0.0f, 0.0f, 201.0f, 50.0f, 0.0f, 1.0f);
    auto cc0 = CGXDefault<TevStageCombiner::ColorCombiner>(0);
    cc0.d = TevColorArg::Konst;
    CGX_LOAD_BP_REG(cc0.hex);
    auto ac0 = CGXDefault<TevStageCombiner::AlphaCombiner>(0);
    ac0.d = TevAlphaArg::Konst;
    CGX_LOAD_BP_REG(ac0.hex);

    // Second stage makes sure the output is always in the red channel.
    auto cc1 = CGXDefault<TevStageCombiner::ColorCombiner>(1);
    cc1.d = step < 32 ? TevColorArg::PrevColor : TevColorArg::PrevAlpha;
    CGX_LOAD_BP_REG(cc1.hex);
    auto ac1 = CGXDefault<TevStageCombiner::AlphaCombiner>(1);
    ac1.d = TevAlphaArg::Konst;
    CGX_LOAD_BP_REG(ac1.hex);

    TevKSel sel;
    sel.hex = BPMEM_TEV_KSEL << 24;
    sel.swap1 = 0;
    sel.swap2 = 0;
    sel.kcsel0 = step < 32 ? static_cast<KonstSel>(step & 31) : KonstSel::V1;
    sel.kcsel1 = KonstSel::V1;
    sel.kasel0 = step >= 32 ? static_cast<KonstSel>(step & 31) : KonstSel::V1;
    sel.kasel1 = KonstSel::V1;
    CGX_LOAD_BP_REG(sel.hex);

    GXTest::Quad().ColorRGBA(0, 0, 0, 0xff).Draw();

    GXTest::CopyToTestBuffer(0, 0, 199, 49);
    CGX_WaitForGpuToFinish();

    GXTest::Vec4<u8> result = GXTest::ReadTestBuffer(test_x, test_y, 200);
    int expected[] = {
        255, 223, 191, 159, 128, 96, 64,  32,  0,  0,  0,   0,   10, 60, 110, 160,
        10,  60,  110, 160, 20,  70, 120, 170, 30, 80, 130, 180, 40, 90, 140, 190,
        255, 223, 191, 159, 128, 96, 64,  32,  0,  0,  0,   0,   0,  0,  0,   0,
        10,  60,  110, 160, 20,  70, 120, 170, 30, 80, 130, 180, 40, 90, 140, 190,
    };

    DO_TEST(expected[step] == result.r, "konst test failed; actual {}, expected {}", result.r,
            expected[step]);

    GXTest::DebugDisplayEfbContents();
  }

  // Test the behavior of TevColorArg::Half.
  {
    auto genmode = CGXDefault<GenMode>();
    genmode.numtevstages = 0;  // One stage
    CGX_LOAD_BP_REG(genmode.hex);

    auto zmode = CGXDefault<ZMode>();
    CGX_LOAD_BP_REG(zmode.hex);

    int test_x = 125, test_y = 25;  // Somewhere within the viewport

    // First stage pulls in konst values.
    CGX_SetViewport(0.0f, 0.0f, 201.0f, 50.0f, 0.0f, 1.0f);
    auto cc0 = CGXDefault<TevStageCombiner::ColorCombiner>(0);
    cc0.d = TevColorArg::Half;
    CGX_LOAD_BP_REG(cc0.hex);
    auto ac0 = CGXDefault<TevStageCombiner::AlphaCombiner>(0);
    ac0.d = TevAlphaArg::Zero;
    CGX_LOAD_BP_REG(ac0.hex);

    GXTest::Quad().ColorRGBA(0, 0, 0, 0xff).Draw();

    GXTest::CopyToTestBuffer(0, 0, 199, 49);
    CGX_WaitForGpuToFinish();

    GXTest::Vec4<u8> result = GXTest::ReadTestBuffer(test_x, test_y, 200);

    DO_TEST(result.r == 128, "TevColorArg::Half test failed; actual {}", result.r);

    GXTest::DebugDisplayEfbContents();
  }

  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  GXTest::Init();

  TevCombinerTest();
  KonstTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}
