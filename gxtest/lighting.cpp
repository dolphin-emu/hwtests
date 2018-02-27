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

void LightingTest()
{
  START_TEST();

  CGX_LOAD_BP_REG(CGXDefault<TwoTevStageOrders>(0).hex);

  CGX_BEGIN_LOAD_XF_REGS(0x1009, 1);
  wgPipe->U32 = 1;  // 1 color channel

  LitChannel chan;
  chan.hex = 0;
  chan.matsource = 0;  // from register
  chan.ambsource = 0;  // from register
  chan.enablelighting = true;
  CGX_BEGIN_LOAD_XF_REGS(0x100e, 1);  // color channel 1
  wgPipe->U32 = chan.hex;
  CGX_BEGIN_LOAD_XF_REGS(0x1010, 1);  // alpha channel 1
  wgPipe->U32 = chan.hex;

  CGX_LOAD_BP_REG(CGXDefault<TevStageCombiner::AlphaCombiner>(0).hex);

  auto genmode = CGXDefault<GenMode>();
  genmode.numtevstages = 0;  // One stage
  CGX_LOAD_BP_REG(genmode.hex);

  PE_CONTROL ctrl;
  ctrl.hex = BPMEM_ZCOMPARE << 24;
  ctrl.pixel_format = PIXELFMT_RGB8_Z24;
  ctrl.zformat = ZC_LINEAR;
  ctrl.early_ztest = 0;
  CGX_LOAD_BP_REG(ctrl.hex);

  // Test to check how the hardware rounds the final computation of the
  // lit color of a vertex.  The formula is basically just
  // (material color * lighting color), but the rounding isn't obvious
  // because the hardware uses fixed-point math and takes some shortcuts.
  for (int step = 0; step < 256 * 256; ++step)
  {
    int matcolor = step & 255;
    int ambcolor = step >> 8;

    auto zmode = CGXDefault<ZMode>();
    CGX_LOAD_BP_REG(zmode.hex);

    auto tevreg = CGXDefault<TevReg>(1, false);  // c0
    tevreg.red = 0xff;
    CGX_LOAD_BP_REG(tevreg.low);
    CGX_LOAD_BP_REG(tevreg.high);

    CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
    wgPipe->U32 = 0;  // 0 = enable clipping, 1 = disable clipping

    CGX_BEGIN_LOAD_XF_REGS(0x100a, 1);
    wgPipe->U32 = (ambcolor << 24) | 255;

    CGX_BEGIN_LOAD_XF_REGS(0x100c, 1);
    wgPipe->U32 = (matcolor << 24) | 255;

    int test_x = 125, test_y = 25;  // Somewhere within the viewport

    CGX_SetViewport(0.0f, 0.0f, 201.0f, 50.0f, 0.0f, 1.0f);
    auto cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
    cc.d = TEVCOLORARG_RASC;
    CGX_LOAD_BP_REG(cc.hex);
    GXTest::Quad().ColorRGBA(0, 0, 0, 0xff).Draw();

    GXTest::CopyToTestBuffer(0, 0, 199, 49);
    CGX_WaitForGpuToFinish();

    GXTest::Vec4<u8> result = GXTest::ReadTestBuffer(test_x, test_y, 200);
    int expected = (matcolor * (ambcolor + (ambcolor >> 7))) >> 8;
    DO_TEST(result.r == expected, "lighting test failed at amb %d mat %d actual %d", ambcolor,
            matcolor, result.r);

    GXTest::DebugDisplayEfbContents();
    WPAD_ScanPads();
    if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
      break;
  }

  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  GXTest::Init();

  LightingTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}
