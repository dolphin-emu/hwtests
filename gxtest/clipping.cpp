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

void ClipTest()
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

  for (int step = 0; step < 16; ++step)
  {
    auto zmode = CGXDefault<ZMode>();
    CGX_LOAD_BP_REG(zmode.hex);

    // First off, clear previous screen contents
    CGX_SetViewport(0.0f, 0.0f, 201.0f, 50.0f, 0.0f,
                    1.0f);  // stuff which really should not be filled
    auto cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
    cc.d = TEVCOLORARG_RASC;
    CGX_LOAD_BP_REG(cc.hex);
    GXTest::Quad().ColorRGBA(0, 0, 0, 0xff).Draw();

    CGX_SetViewport(75.0f, 0.0f, 100.0f, 50.0f, 0.0f, 1.0f);  // guardband
    cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
    cc.d = TEVCOLORARG_RASC;
    CGX_LOAD_BP_REG(cc.hex);
    GXTest::Quad().ColorRGBA(0, 0x7f, 0, 0xff).Draw();

    CGX_SetViewport(100.0f, 0.0f, 50.0f, 50.0f, 0.0f, 1.0f);  // viewport
    cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
    cc.d = TEVCOLORARG_RASC;
    CGX_LOAD_BP_REG(cc.hex);
    GXTest::Quad().ColorRGBA(0, 0xff, 0, 0xff).Draw();

    // Now, enable testing viewport and draw the (red) testing quad
    CGX_SetViewport(100.0f, 0.0f, 50.0f, 50.0f, 0.0f, 1.0f);

    cc.d = TEVCOLORARG_C0;
    CGX_LOAD_BP_REG(cc.hex);

    auto tevreg = CGXDefault<TevReg>(1, false);  // c0
    tevreg.red = 0xff;
    CGX_LOAD_BP_REG(tevreg.low);
    CGX_LOAD_BP_REG(tevreg.high);

    CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
    wgPipe->U32 = 0;  // 0 = enable clipping, 1 = disable clipping

    bool expect_quad_to_be_drawn = true;
    int test_x = 125, test_y = 25;  // Somewhere within the viewport
    GXTest::Quad test_quad;
    test_quad.ColorRGBA(0xff, 0xff, 0xff, 0xff);

    switch (step)
    {
    // Rendering outside the viewport when scissor rect is bigger than viewport
    // TODO: What about partially covered primitives?

    case 0:  // all vertices within viewport
      // Nothing to do
      break;

    case 1:  // two vertices outside viewport, but within guardband
      test_quad.VertexTopLeft(-1.8f, 1.0f, 1.0f).VertexBottomLeft(-1.8f, -1.0f, 1.0f);
      test_x = 75;  // TODO: Move closer to actual viewport, but debug readback issues first
      break;

    case 2:  // two vertices outside viewport and guardband
      test_quad.VertexTopLeft(-2.5f, 1.0f, 1.0f).VertexBottomLeft(-2.5f, -1.0f, 1.0f);
      test_x = 51;  // TODO: This is actually outside the guardband
      // TODO: Check x=50, should be green
      break;

    case 3:  // all vertices outside viewport, but within guardband and NOT on the same side of the
             // viewport
      test_quad.VertexTopLeft(-1.5f, 1.0f, 1.0f).VertexBottomLeft(-1.5f, -1.0f, 1.0f);
      test_quad.VertexTopRight(1.5f, 1.0f, 1.0f).VertexBottomRight(1.5f, 1.0f, 1.0f);
      test_x = 80;  // TODO: Move closer to actual viewport
      break;

    case 4:  // all vertices outside viewport and guardband, but NOT on the same side of the
             // viewport
      test_quad.VertexTopLeft(-2.5f, 1.0f, 1.0f).VertexBottomLeft(-2.5f, -1.0f, 1.0f);
      test_quad.VertexTopRight(2.5f, 1.0f, 1.0f).VertexBottomRight(2.5f, 1.0f, 1.0f);
      test_x = 51;  // TODO: This is actually outside the guardband
      // TODO: Check x=50,x=200?,x=201?, 50 and 201 should be green, 200 should be red
      break;

    case 5:  // all vertices outside viewport, but within guardband and on the same side of the
             // viewport
      test_quad.VertexTopLeft(-1.8f, 1.0f, 1.0f).VertexBottomLeft(-1.8f, -1.0f, 1.0f);
      test_quad.VertexTopRight(-1.2f, 1.0f, 1.0f).VertexBottomRight(-1.2f, 1.0f, 1.0f);
      test_quad.VertexTopRight(1.5f, 1.0f, 1.0f);
      expect_quad_to_be_drawn = false;
      break;

    case 6:  // guardband-clipping test
      // TODO: Currently broken
      // Exceeds the guard-band clipping plane by the viewport width,
      // so the primitive will get clipped such that one edge touches
      // the clipping plane.exactly at the vertical viewport center.
      // ASCII picture of clipped primitive (within guard-band region):
      // |-----  pixel row  0
      // |       pixel row  1
      // |       pixel row  2
      // |       pixel row  3
      // |       pixel row  4
      // \       pixel row  5 <-- vertical viewport center
      //  \      pixel row  6
      //   \     pixel row  7
      //    \    pixel row  8
      //     \   pixel row  9
      //      \  pixel row 10
      test_quad.VertexTopLeft(-4.0f, 1.0f, 1.0f);
      test_x = 51;  // TODO: This is actually outside the guardband
      test_y = 1;
      // TODO: Test y roughly equals 60 (there's no good way to test this without relying on
      // pixel-perfect clipping), here should NOT be a quad!
      break;

    // Depth clipping tests
    case 7:  // Everything behind z=w plane, depth clipping enabled
    case 8:  // Everything behind z=w plane, depth clipping disabled
      CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
      wgPipe->U32 = step - 7;  // 0 = enable clipping, 1 = disable clipping

      test_quad.AtDepth(1.1);
      expect_quad_to_be_drawn = false;
      break;

    case 9:   // Everything in front of z=0 plane, depth clipping enabled
    case 10:  // Everything in front of z=0 plane, depth clipping disabled
      CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
      wgPipe->U32 = step - 9;  // 0 = enable clipping, 1 = disable clipping

      test_quad.AtDepth(-0.00001);
      expect_quad_to_be_drawn = false;
      break;

    case 11:  // Very slightly behind z=w plane, depth clipping enabled
    case 12:  // Very slightly behind z=w plane, depth clipping disabled
      // TODO: For whatever reason, this doesn't actually work, yet
      // The GC/Wii GPU doesn't implement IEEE floats strictly, hence
      // the sum of the projected position's z and w is a very small
      // number, which by IEEE would be non-zero but which in fact is
      // treated as zero.
      // In particular, the value by IEEE is -0.00000011920928955078125.
      CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
      wgPipe->U32 = step - 11;  // 0 = enable clipping, 1 = disable clipping

      test_quad.AtDepth(1.0000001);
      break;

    case 13:  // One vertex behind z=w plane, depth clipping enabled
    case 14:  // One vertex behind z=w plane, depth clipping disabled
      CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
      wgPipe->U32 = step - 13;  // 0 = enable clipping, 1 = disable clipping

      test_quad.VertexTopLeft(-1.0f, 1.0f, 1.5f);

      // whole primitive gets clipped away if depth clipping is enabled.
      // Otherwise the whole primitive is drawn.
      expect_quad_to_be_drawn = step - 13;
      break;

    case 15:  // Three vertices with a very large value for z, depth clipping disabled
      CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
      wgPipe->U32 = 1;  // 0 = enable clipping, 1 = disable clipping

      test_quad.VertexTopLeft(-1.0f, 1.0f, 65537.f);
      test_quad.VertexTopRight(1.0f, 1.0f, 65537.f);
      test_quad.VertexBottomLeft(-1.0f, -1.0f, 65537.f);
      break;

      // TODO: One vertex with z < 0, depth clipping enabled, primitive gets properly (!) clipped
      // TODO: One vertex with z < 0, depth clipping disabled, whole primitive gets drawn
    }

    test_quad.Draw();
    GXTest::CopyToTestBuffer(0, 0, 199, 49);
    CGX_WaitForGpuToFinish();

    GXTest::Vec4<u8> result = GXTest::ReadTestBuffer(test_x, test_y, 200);
    if (expect_quad_to_be_drawn)
      DO_TEST(result.r == 0xff, "Clipping test failed at step {} (expected quad to be shown at "
                                "pixel ({}, {}), but it was not)",
              step, test_x, test_y);
    else
      DO_TEST(result.r == 0x00, "Clipping test failed at step {} (expected quad to be hidden at "
                                "pixel ({}, {}), but it was not)",
              step, test_x, test_y);

    GXTest::DebugDisplayEfbContents();
  }

  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  GXTest::Init();

  ClipTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}
