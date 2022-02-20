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

void CoordinatePrecisionTest()
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
  ac.d = TevAlphaArg::RasAlpha;
  CGX_LOAD_BP_REG(ac.hex);

  auto cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
  cc.d = TevColorArg::RasColor;
  CGX_LOAD_BP_REG(cc.hex);

  // Test at which coordinates a pixel is considered to be within a primitive.
  // TODO: Not sure how to interpret the results, yet.
  for (float xpos = 0.583328247070f; xpos <= 0.583328306675f; xpos = nextafterf(xpos, +1.0f))
  {
    CGX_SetViewport(xpos, 0.0f, 100.0f, 100.0f, 0.0f, 1.0f);

    // first off, clear the full area.
    GXTest::Quad()
        .VertexTopLeft(-2.0, 2.0, 1.0)
        .VertexBottomLeft(-2.0, -2.0, 1.0)
        .VertexTopRight(2.0, 2.0, 1.0)
        .VertexBottomRight(2.0, -2.0, 1.0)
        .ColorRGBA(0, 0, 0, 255)
        .Draw();
    GXTest::Quad().ColorRGBA(0, 255, 0, 255).Draw();

    // now, draw the actual testing quad.
    GXTest::Quad()
        .VertexTopLeft(0, 1.0, 1.0)
        .VertexBottomLeft(0, -1.0, 1.0)
        .ColorRGBA(255, 0, 255, 255)
        .Draw();
    GXTest::CopyToTestBuffer(50, 0, 127, 127);
    CGX_WaitForGpuToFinish();
    GXTest::DebugDisplayEfbContents();

    GXTest::Vec4<u8> result = GXTest::ReadTestBuffer(0, 0, 128);
    u8 expectation = (xpos <= 0.583328247070f) ? 255 : 0;
    int subsample_index = (int)(xpos * 12.0f) % 12;
    DO_TEST(result.r == expectation,
            "Incorrect rasterization (result={},expected={},screencoord={:.6f},subsample_index={})",
            result.r, expectation, xpos, subsample_index);
  }

  // Test for the default pixel subsample location by creating tiny viewports (smaller than the
  // pixel width)
  // By drawing a quad over the whole viewport, we can check if the current viewport spans the
  // subsample location.
  // The results seem to indicate that the default subsample is located close to (but somewhat off)
  // screen position 7/12.
  // I (neobrain) am not sure if the sample indeed is not at that location or if it's just due to
  // floating point rounding errors.
  for (float xpos = 0.583297669888f; xpos <= 0.583328306675; xpos = nextafterf(xpos, +1.0f))
  {
    CGX_SetViewport(0.0, 0.0f, 100.0f, 100.0f, 0.0f, 1.0f);

    // first off, clear the full area.
    GXTest::Quad()
        .VertexTopLeft(-2.0, 2.0, 1.0)
        .VertexBottomLeft(-2.0, -2.0, 1.0)
        .VertexTopRight(2.0, 2.0, 1.0)
        .VertexBottomRight(2.0, -2.0, 1.0)
        .ColorRGBA(0, 0, 0, 255)
        .Draw();
    GXTest::Quad().ColorRGBA(0, 255, 0, 255).Draw();

    // manual viewport setting to make sure we aren't limited (too much) by floating point precision
    // 2.0e-5 seems to be the smallest possible viewport width which behaves sane.
    // For this size, values of xpos from 0.583297729492 to 0.583328247070 will yield a covered
    // pixel
    float vp_width = 2.0e-5f;
    CGX_BEGIN_LOAD_XF_REGS(0x101a, 6);
    wgPipe->F32 = vp_width;
    wgPipe->F32 = -50.0f;
    wgPipe->F32 = 16777215.0f;
    wgPipe->F32 = 342.0f + xpos + vp_width;
    wgPipe->F32 = 392.0f;
    wgPipe->F32 = 16777215.0f;

    // now, draw the actual testing quad.
    GXTest::Quad().ColorRGBA(255, 0, 255, 255).Draw();
    GXTest::CopyToTestBuffer(0, 0, 127, 127);
    CGX_WaitForGpuToFinish();
    GXTest::DebugDisplayEfbContents();

    GXTest::Vec4<u8> result = GXTest::ReadTestBuffer(0, 0, 128);
    u8 expectation = (xpos == 0.583297669888f || xpos == 0.583328306675f) ? 0 : 255;
    int subsample_index = (int)(xpos * 12.0f) % 12;
    DO_TEST(result.r == expectation,
            "Incorrect rasterization (result={},expected={},screencoord={:.12f},subsample_index={})",
            result.r, expectation, xpos, subsample_index);
  }

  // Guardband clipping indeed uses floating point math!
  // Hence, the smallest floating point value smaller than -2.0 will yield a clipped primitive.
  CGX_SetViewport(100.0f, 100.0f, 100.0f, 100.0f, 0.0f, 1.0f);
  for (float xpos : {-2.0000000f, nextafterf(-2.0f, -1.0f)})
  {
    // first off, clear the full area, including the guardband
    GXTest::Quad()
        .VertexTopLeft(-2.0, 2.0, 1.0)
        .VertexBottomLeft(-2.0, -2.0, 1.0)
        .VertexTopRight(2.0, 2.0, 1.0)
        .VertexBottomRight(2.0, -2.0, 1.0)
        .ColorRGBA(0, 0, 0, 255)
        .Draw();

    // now, draw the actual testing quad such that all vertices are outside the viewport (and on the
    // same side of the viewport)
    // The two left vertices are at the border of the guardband; if they are outside the guardband,
    // the primitive gets clipped away.
    GXTest::Quad()
        .VertexTopLeft(xpos, 1.0, 1.0)
        .VertexBottomLeft(xpos, -1.0, 1.0)
        .VertexTopRight(xpos + 1.0, 1.0, 1.0)
        .VertexBottomRight(xpos + 1.0, -1.0, 1.0)
        .ColorRGBA(255, 0, 255, 255)
        .Draw();
    GXTest::CopyToTestBuffer(50, 100, 127, 127);
    CGX_WaitForGpuToFinish();
    GXTest::DebugDisplayEfbContents();

    GXTest::Vec4<u8> result = GXTest::ReadTestBuffer(10, 10, 128);

    int expectation = (xpos >= -2.0) ? 255 : 0;
    DO_TEST(result.r == expectation,
            "Incorrect guardband clipping (result={},expected={},xpos={:.10f})", result.r,
            expectation, xpos);
  };

  END_TEST();
}

int main()
{
  network_init();
  WPAD_Init();

  GXTest::Init();

  CoordinatePrecisionTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}
