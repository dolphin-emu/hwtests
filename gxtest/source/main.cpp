// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <initializer_list>
#include "Test.h"
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>
#include "cgx.h"
#include "cgx_defaults.h"
#include "gxtest_util.h"

void BitfieldTest()
{
	START_TEST();

	TevReg reg;
	reg.hex = 0;
	reg.low = 0x345678;
	DO_TEST(reg.alpha == 837, "Values don't match (have: %d)", (s32)reg.alpha);
	DO_TEST(reg.red == -392, "Values don't match (have: %d)", (s32)reg.red);
	reg.low = 0x4BC6A8;
	DO_TEST(reg.alpha == -836, "Values don't match (have: %d)", (s32)reg.alpha);
	DO_TEST(reg.red == -344, "Values don't match (have: %d)", (s32)reg.red);
	reg.hex = 0;
	reg.alpha = -263;
	reg.red = -345;
	DO_TEST(reg.alpha == -263, "Values don't match (have: %d)", (s32)reg.alpha);
	DO_TEST(reg.red == -345, "Values don't match (have: %d)", (s32)reg.red);
	reg.alpha = 15;
	reg.red = -619;
	DO_TEST(reg.alpha == 15, "Values don't match (have: %d)", (s32)reg.alpha);
	DO_TEST(reg.red == -619, "Values don't match (have: %d)", (s32)reg.red);
	reg.alpha = 523;
	reg.red = 176;
	DO_TEST(reg.alpha == 523, "Values don't match (have: %d)", (s32)reg.alpha);
	DO_TEST(reg.red == 176, "Values don't match (have: %d)", (s32)reg.red);

	END_TEST();
}

int TevCombinerExpectation(int a, int b, int c, int d, int shift, int bias, int op, int clamp)
{
	a &= 255;
	b &= 255;
	c &= 255;

	// TODO: Does not handle compare mode, yet
	c = c+(c>>7);
	u16 lshift = (shift == 1) ? 1 : (shift == 2) ? 2 : 0;
	u16 rshift = (shift == 3) ? 1 : 0;
	int round_bias = (shift==3) ? 0 : ((op==1) ? 127 : 128);
	int expected = (((a*(256-c) + b*c) << lshift)+round_bias)>>8; // lerp
	expected = (d << lshift) + expected * ((op == 1) ? (-1) : 1);
	expected += ((bias == 2) ? -128 : (bias == 1) ? 128 : 0) << lshift;
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
	wgPipe->U32 = 1; // 1 color channel

	LitChannel chan;
	chan.hex = 0;
	chan.matsource = 1; // from vertex
	CGX_BEGIN_LOAD_XF_REGS(0x100e, 1); // color channel 1
	wgPipe->U32 = chan.hex;
	CGX_BEGIN_LOAD_XF_REGS(0x1010, 1); // alpha channel 1
	wgPipe->U32 = chan.hex;

	auto ac = CGXDefault<TevStageCombiner::AlphaCombiner>(0);
	CGX_LOAD_BP_REG(ac.hex);

	// Test if we can reliably extract all bits of the tev combiner output...
	auto tevreg = CGXDefault<TevReg>(1, false); // c0
	for (int i = 0; i < 2; ++i)
		for (tevreg.red = -1024; tevreg.red != 1023; tevreg.red = tevreg.red+1)
		{
			CGX_LOAD_BP_REG(tevreg.low);
			CGX_LOAD_BP_REG(tevreg.high);

			auto genmode = CGXDefault<GenMode>();
			genmode.numtevstages = 0; // One stage
			CGX_LOAD_BP_REG(genmode.hex);

			auto cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
			cc.d = TEVCOLORARG_C0;
			CGX_LOAD_BP_REG(cc.hex);

			PE_CONTROL ctrl;
			ctrl.hex = BPMEM_ZCOMPARE<<24;
			ctrl.zformat = ZC_LINEAR;
			ctrl.early_ztest = 0;
			if (i == 0)
			{
				// 8 bits per channel: No worries about GetTevOutput making
				// mistakes when writing to framebuffer or when performing
				// an EFB copy.
				ctrl.pixel_format = PIXELFMT_RGB8_Z24;
				CGX_LOAD_BP_REG(ctrl.hex);

				int result = GXTest::GetTevOutput(genmode, cc, ac).r;

				DO_TEST(result == tevreg.red, "Got %d, expected %d", result, (s32)tevreg.red);
			}
			else
			{
				// TODO: This doesn't quite work, yet.
				break;

				// 6 bits per channel: Implement GetTevOutput functionality
				// manually, to verify how tev output is truncated to 6 bit
				// and how EFB copies upscale that to 8 bit again.
				ctrl.pixel_format = PIXELFMT_RGBA6_Z24;
				CGX_LOAD_BP_REG(ctrl.hex);

				GXTest::Quad().AtDepth(1.0).ColorRGBA(255,255,255,255).Draw();
				GXTest::CopyToTestBuffer(0, 0, 99, 9);
				CGX_ForcePipelineFlush();
				CGX_WaitForGpuToFinish();
				u16 result = GXTest::ReadTestBuffer(5, 5, 100).r;

				int expected = (((tevreg.red+1)>>2)&0xFF) << 2;
				expected = expected | (expected>>6);
				DO_TEST(result == expected, "Run %d: Got %d, expected %d", (int)tevreg.red, result, expected);
			}
		}

	// Now: Randomized testing of tev combiners.
	for (int i = 0x000000; i < 0x000F000; ++i)
	{
		if ((i & 0xFF00) == i)
			network_printf("progress: %x\n", i);

		auto genmode = CGXDefault<GenMode>();
		genmode.numtevstages = 0; // One stage
		CGX_LOAD_BP_REG(genmode.hex);

		// Randomly configured TEV stage, output in PREV.
		auto cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
		cc.a = TEVCOLORARG_C0;
		cc.b = TEVCOLORARG_C1;
		cc.c = TEVCOLORARG_C2;
		cc.d = TEVCOLORARG_ZERO; // TEVCOLORARG_CPREV; // NOTE: TEVCOLORARG_CPREV doesn't actually seem to fetch its data from PREV when used in the first stage?
		cc.shift = rand() % 4;
		cc.bias = rand() % 3;
		cc.op = rand()%2;
		cc.clamp = rand() % 2;
		CGX_LOAD_BP_REG(cc.hex);

		int a = -1024 + (rand() % 2048);
		int b = -1024 + (rand() % 2048);
		int c = -1024 + (rand() % 2048);
		int d = 0; //-1024 + (rand() % 2048);
		tevreg = CGXDefault<TevReg>(1, false); // c0
		tevreg.red = a;
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);
		tevreg = CGXDefault<TevReg>(2, false); // c1
		tevreg.red = b;
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);
		tevreg = CGXDefault<TevReg>(3, false); // c2
		tevreg.red = c;
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);
		tevreg = CGXDefault<TevReg>(0, false); // prev
		tevreg.red = d;
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);

		PE_CONTROL ctrl;
		ctrl.hex = BPMEM_ZCOMPARE<<24;
		ctrl.pixel_format = PIXELFMT_RGB8_Z24;
		ctrl.zformat = ZC_LINEAR;
		ctrl.early_ztest = 0;
		CGX_LOAD_BP_REG(ctrl.hex);

		int result = GXTest::GetTevOutput(genmode, cc, ac).r;

		int expected = TevCombinerExpectation(a, b, c, d, cc.shift, cc.bias, cc.op, cc.clamp);
		DO_TEST(result == expected, "Mismatch on a=%d, b=%d, c=%d, d=%d, shift=%d, bias=%d, op=%d, clamp=%d: expected %d, got %d", a, b, c, d, (u32)cc.shift, (u32)cc.bias, (u32)cc.op, (u32)cc.clamp, expected, result);

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
		genmode.numtevstages = 0; // One stage
		CGX_LOAD_BP_REG(genmode.hex);

		auto cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
		cc.a = TEVCOLORARG_C2;
		cc.b = TEVCOLORARG_C1;
		CGX_LOAD_BP_REG(cc.hex);

		auto ac = CGXDefault<TevStageCombiner::AlphaCombiner>(0);
		ac.bias = TevBias_COMPARE;
		ac.a = TEVALPHAARG_A0; // different from color combiner
		ac.b = TEVALPHAARG_A1; // same as color combiner
		ac.c = TEVALPHAARG_A2;
		CGX_LOAD_BP_REG(ac.hex);

		PE_CONTROL ctrl;
		ctrl.hex = BPMEM_ZCOMPARE<<24;
		ctrl.pixel_format = PIXELFMT_RGBA6_Z24;
		ctrl.zformat = ZC_LINEAR;
		ctrl.early_ztest = 0;
		CGX_LOAD_BP_REG(ctrl.hex);

		tevreg = CGXDefault<TevReg>(1, false); // c0
		tevreg.red = 127; // 127 is always NOT less than 127.
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);

		tevreg = CGXDefault<TevReg>(2, false); // c1
		tevreg.red = 127;
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);

		tevreg = CGXDefault<TevReg>(3, false); // c2
		tevreg.red = 127+i; // 127+i is less than 127 iff i>0.
		tevreg.alpha = 255;
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);

		int result = GXTest::GetTevOutput(genmode, cc, ac).a;
		int expected = (i == 1) ? 255 : 0;
		DO_TEST(result == expected, "Mismatch on run %d: expected %d, got %d", i, expected, result);
	}

	END_TEST();
}

void ClipTest()
{
	START_TEST();

	CGX_LOAD_BP_REG(CGXDefault<TwoTevStageOrders>(0).hex);

	CGX_BEGIN_LOAD_XF_REGS(0x1009, 1);
	wgPipe->U32 = 1; // 1 color channel

	LitChannel chan;
	chan.hex = 0;
	chan.matsource = 1; // from vertex
	CGX_BEGIN_LOAD_XF_REGS(0x100e, 1); // color channel 1
	wgPipe->U32 = chan.hex;
	CGX_BEGIN_LOAD_XF_REGS(0x1010, 1); // alpha channel 1
	wgPipe->U32 = chan.hex;

	CGX_LOAD_BP_REG(CGXDefault<TevStageCombiner::AlphaCombiner>(0).hex);

	auto genmode = CGXDefault<GenMode>();
	genmode.numtevstages = 0; // One stage
	CGX_LOAD_BP_REG(genmode.hex);

	PE_CONTROL ctrl;
	ctrl.hex = BPMEM_ZCOMPARE<<24;
	ctrl.pixel_format = PIXELFMT_RGB8_Z24;
	ctrl.zformat = ZC_LINEAR;
	ctrl.early_ztest = 0;
	CGX_LOAD_BP_REG(ctrl.hex);

	for (int step = 0; step < 13; ++step)
	{
		auto zmode = CGXDefault<ZMode>();
		CGX_LOAD_BP_REG(zmode.hex);

		// First off, clear previous screen contents
		CGX_SetViewport(0.0f, 0.0f, 201.0f, 50.0f, 0.0f, 1.0f); // stuff which really should not be filled
		auto cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
		cc.d = TEVCOLORARG_RASC;
		CGX_LOAD_BP_REG(cc.hex);
		GXTest::Quad().ColorRGBA(0,0,0,0xff).Draw();

		CGX_SetViewport(75.0f, 0.0f, 100.0f, 50.0f, 0.0f, 1.0f); // guardband
		cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
		cc.d = TEVCOLORARG_RASC;
		CGX_LOAD_BP_REG(cc.hex);
		GXTest::Quad().ColorRGBA(0,0x7f,0,0xff).Draw();

		CGX_SetViewport(100.0f, 0.0f, 50.0f, 50.0f, 0.0f, 1.0f); // viewport
		cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
		cc.d = TEVCOLORARG_RASC;
		CGX_LOAD_BP_REG(cc.hex);
		GXTest::Quad().ColorRGBA(0,0xff,0,0xff).Draw();

		// Now, enable testing viewport and draw the (red) testing quad
		CGX_SetViewport(100.0f, 0.0f, 50.0f, 50.0f, 0.0f, 1.0f);

		cc.d = TEVCOLORARG_C0;
		CGX_LOAD_BP_REG(cc.hex);

		auto tevreg = CGXDefault<TevReg>(1, false); // c0
		tevreg.red = 0xff;
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);

		CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
		wgPipe->U32 = 0; // 0 = enable clipping, 1 = disable clipping

		bool expect_quad_to_be_drawn = true;
		int test_x = 125, test_y = 25; // Somewhere within the viewport
		GXTest::Quad test_quad;
		test_quad.ColorRGBA(0xff,0xff,0xff,0xff);

		switch (step)
		{
		// Rendering outside the viewport when scissor rect is bigger than viewport
		// TODO: What about partially covered primitives?

		case 0: // all vertices within viewport
			// Nothing to do
			break;


		case 1: // two vertices outside viewport, but within guardband
			test_quad.VertexTopLeft(-1.8f, 1.0f, 1.0f).VertexBottomLeft(-1.8f, -1.0f, 1.0f);
			test_x = 75; // TODO: Move closer to actual viewport, but debug readback issues first
			break;

		case 2: // two vertices outside viewport and guardband
			test_quad.VertexTopLeft(-2.5f, 1.0f, 1.0f).VertexBottomLeft(-2.5f, -1.0f, 1.0f);
			test_x = 51; // TODO: This is actually outside the guardband
			// TODO: Check x=50, should be green
			break;

		case 3: // all vertices outside viewport, but within guardband and NOT on the same side of the viewport
			test_quad.VertexTopLeft(-1.5f, 1.0f, 1.0f).VertexBottomLeft(-1.5f, -1.0f, 1.0f);
			test_quad.VertexTopRight(1.5f, 1.0f, 1.0f).VertexBottomRight(1.5f, 1.0f, 1.0f);
			test_x = 80; // TODO: Move closer to actual viewport
			break;

		case 4: // all vertices outside viewport and guardband, but NOT on the same side of the viewport
			test_quad.VertexTopLeft(-2.5f, 1.0f, 1.0f).VertexBottomLeft(-2.5f, -1.0f, 1.0f);
			test_quad.VertexTopRight(2.5f, 1.0f, 1.0f).VertexBottomRight(2.5f, 1.0f, 1.0f);
			test_x = 51; // TODO: This is actually outside the guardband
			// TODO: Check x=50,x=200?,x=201?, 50 and 201 should be green, 200 should be red
			break;

		case 5: // all vertices outside viewport, but within guardband and on the same side of the viewport
			test_quad.VertexTopLeft(-1.8f, 1.0f, 1.0f).VertexBottomLeft(-1.8f, -1.0f, 1.0f);
			test_quad.VertexTopRight(-1.2f, 1.0f, 1.0f).VertexBottomRight(-1.2f, 1.0f, 1.0f);
			test_quad.VertexTopRight(1.5f, 1.0f, 1.0f);
			expect_quad_to_be_drawn = false;
			break;

		case 6: // guardband-clipping test
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
			test_x = 51; // TODO: This is actually outside the guardband
			test_y = 1;
			// TODO: Test y roughly equals 60 (there's no good way to test this without relying on pixel-perfect clipping), here should NOT be a quad!
			break;

		// Depth clipping tests
		case 7:  // Everything behind z=w plane, depth clipping enabled
		case 8:  // Everything behind z=w plane, depth clipping disabled
			CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
			wgPipe->U32 = step - 7; // 0 = enable clipping, 1 = disable clipping

			test_quad.AtDepth(1.1);
			expect_quad_to_be_drawn = false;
			break;

		case 9:  // Everything in front of z=0 plane, depth clipping enabled
		case 10:  // Everything in front of z=0 plane, depth clipping disabled
			CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
			wgPipe->U32 = step - 9; // 0 = enable clipping, 1 = disable clipping

			test_quad.AtDepth(-0.00001);
			expect_quad_to_be_drawn = false;
			break;

		case 11: // Very slightly behind z=w plane, depth clipping enabled
		case 12: // Very slightly behind z=w plane, depth clipping disabled
			// TODO: For whatever reason, this doesn't actually work, yet
			// The GC/Wii GPU doesn't implement IEEE floats strictly, hence
			// the sum of the projected position's z and w is a very small
			// number, which by IEEE would be non-zero but which in fact is
			// treated as zero.
			// In particular, the value by IEEE is -0.00000011920928955078125.
			CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
			wgPipe->U32 = step - 11; // 0 = enable clipping, 1 = disable clipping

			test_quad.AtDepth(1.0000001);
			break;

		case 13:  // One vertex behind z=w plane, depth clipping enabled
		case 14:  // One vertex behind z=w plane, depth clipping disabled
			CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
			wgPipe->U32 = step - 13; // 0 = enable clipping, 1 = disable clipping

			test_quad.VertexTopLeft(-1.0f, 1.0f, 1.5f);

			// whole primitive gets clipped away
			expect_quad_to_be_drawn = false;
			break;

		case 15:  // Three vertices with a very large value for z, depth clipping disabled
			CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
			wgPipe->U32 = 1; // 0 = enable clipping, 1 = disable clipping

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
			DO_TEST(result.r == 0xff, "Clipping test failed at step %d (expected quad to be shown at pixel (%d, %d), but it was not)", step, test_x, test_y);
		else
			DO_TEST(result.r == 0x00, "Clipping test failed at step %d (expected quad to be hidden at pixel (%d, %d), but it was not)", step, test_x, test_y);

		GXTest::DebugDisplayEfbContents();
	}

	END_TEST();
}

void CoordinatePrecisionTest()
{
	START_TEST();

	CGX_SetViewport(0.0f, 0.0f, 100.0f, 100.0f, 0.0f, 1.0f);

	CGX_LOAD_BP_REG(CGXDefault<TwoTevStageOrders>(0).hex);

	CGX_BEGIN_LOAD_XF_REGS(0x1009, 1);
	wgPipe->U32 = 1; // 1 color channel

	LitChannel chan;
	chan.hex = 0;
	chan.matsource = 1; // from vertex
	CGX_BEGIN_LOAD_XF_REGS(0x100e, 1); // color channel 1
	wgPipe->U32 = chan.hex;
	CGX_BEGIN_LOAD_XF_REGS(0x1010, 1); // alpha channel 1
	wgPipe->U32 = chan.hex;

	auto ac = CGXDefault<TevStageCombiner::AlphaCombiner>(0);
	ac.d = TEVALPHAARG_RASA;
	CGX_LOAD_BP_REG(ac.hex);

	auto cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
	cc.d = TEVCOLORARG_RASC;
	CGX_LOAD_BP_REG(cc.hex);

	// Test at which coordinates a pixel is considered to be within a primitive.
	// TODO: Not sure how to interpret the results, yet.
	// NOTE: Tests for the values -0.08833329, -0.08833328, 0.01166687, 0.03166687, 0.05166687, 0.9166688 still fail.
	int index = 0;
	for (float xpos : {-0.98833334f, -0.96833336f, -0.94833338f, -0.92833334f,
	                   -0.90833336f, -0.88833338f, -0.86833340f, -0.84833336f,
	                   -0.82833338f, -0.80833340f, -0.78833336f, -0.76833338f,
	                   -0.74833339f, -0.72833335f, -0.70833337f, -0.68833339f,
	                   -0.66833335f, -0.64833337f, -0.62833339f, -0.60833335f,
	                   -0.58833337f, -0.56833339f, -0.54833335f, -0.52833337f,
	                   -0.50833338f, -0.48833331f, -0.46833333f, -0.44833332f,
	                   -0.42833331f, -0.40833333f, -0.38833332f, -0.36833334f,
	                   -0.34833333f, -0.32833332f, -0.30833334f, -0.28833333f,
	                   -0.26833332f, -0.24833331f, -0.22833331f, -0.20833330f,
	                   -0.18833330f, -0.16833331f, -0.14833331f, -0.12833330f,
	                   -0.10833330f, -0.08833329f, -0.06833329f, -0.04833329f,
	                   -0.02833329f, -0.00833328f, 0.01166687f, 0.03166687f,
	                   0.05166687f, 0.07166687f, 0.09166688f, 0.11166687f,
	                   0.13166688f, 0.15166688f, 0.17166688f, 0.19166687f,
	                   0.21166688f, 0.23166688f, 0.25166687f, 0.27166688f,
	                   0.29166690f, 0.31166688f, 0.33166689f, 0.35166690f,
	                   0.37166688f, 0.39166689f, 0.41166687f, 0.43166688f,
	                   0.45166689f, 0.47166687f, 0.49166688f, 0.51166689f,
	                   0.53166687f, 0.55166692f, 0.57166690f, 0.59166688f,
	                   0.61166692f, 0.63166690f, 0.65166688f, 0.67166692f,
	                   0.69166690f, 0.71166688f, 0.73166692f, 0.75166690f,
	                   0.77166688f, 0.79166692f, 0.81166691f, 0.83166689f,
	                   0.85166693f, 0.87166691f, 0.89166689f, 0.91166687f,
	                   0.93166691f, 0.95166689f, 0.97166687f, 0.99166691f,
	                   // For any of the values below, the pixel should be considered outside the primitive
	                   // These are the closest representable 32 bit floating point numbers to the above ones
	                   -0.98833328f, -0.96833330f, -0.94833332f, -0.92833328f,
	                   -0.90833330f, -0.88833332f, -0.86833334f, -0.84833330f,
	                   -0.82833332f, -0.80833334f, -0.78833330f, -0.76833332f,
	                   -0.74833333f, -0.72833329f, -0.70833331f, -0.68833333f,
	                   -0.66833329f, -0.64833331f, -0.62833333f, -0.60833329f,
	                   -0.58833331f, -0.56833333f, -0.54833329f, -0.52833331f,
	                   -0.50833333f, -0.48833328f, -0.46833330f, -0.44833329f,
	                   -0.42833328f, -0.40833330f, -0.38833329f, -0.36833331f,
	                   -0.34833330f, -0.32833329f, -0.30833331f, -0.28833330f,
	                   -0.26833329f, -0.24833329f, -0.22833329f, -0.20833328f,
	                   -0.18833329f, -0.16833329f, -0.14833330f, -0.12833329f,
	                   -0.10833329f, -0.08833329f, -0.06833328f, -0.04833328f,
	                   -0.02833328f, -0.00833328f, 0.01166687f, 0.03166687f,
	                   0.05166687f, 0.07166688f, 0.09166688f, 0.11166688f,
	                   0.13166690f, 0.15166689f, 0.17166689f, 0.19166689f,
	                   0.21166690f, 0.23166689f, 0.25166690f, 0.27166691f,
	                   0.29166692f, 0.31166691f, 0.33166692f, 0.35166693f,
	                   0.37166691f, 0.39166692f, 0.41166690f, 0.43166691f,
	                   0.45166692f, 0.47166690f, 0.49166691f, 0.51166695f,
	                   0.53166693f, 0.55166698f, 0.57166696f, 0.59166694f,
	                   0.61166698f, 0.63166696f, 0.65166694f, 0.67166698f,
	                   0.69166696f, 0.71166694f, 0.73166698f, 0.75166696f,
	                   0.77166694f, 0.79166698f, 0.81166697f, 0.83166695f,
	                   0.85166699f, 0.87166697f, 0.89166695f, 0.91166693f,
	                   0.93166697f, 0.95166695f, 0.97166693f, 0.99166697f})
	{
		// first off, clear the full area.
		GXTest::Quad().VertexTopLeft(-2.0, 2.0, 1.0).VertexBottomLeft(-2.0, -2.0, 1.0).VertexTopRight(2.0, 2.0, 1.0).VertexBottomRight(2.0, -2.0, 1.0).ColorRGBA(0,0,0,255).Draw();
		GXTest::Quad().ColorRGBA(0,255,0,255).Draw();

		// now, draw the actual testing quad.
		GXTest::Quad().VertexTopLeft(xpos, 1.0, 1.0).VertexBottomLeft(xpos, -1.0, 1.0).ColorRGBA(255,0,255,255).Draw();
		GXTest::CopyToTestBuffer(0, 0, 127, 127);
		CGX_WaitForGpuToFinish();
		GXTest::DebugDisplayEfbContents();

		GXTest::Vec4<u8> result = GXTest::ReadTestBuffer(index%100, 0, 128);
		u8 expectation = (index < 100) ? 255 : 0;
		float estimated_screencoord = xpos*50.f+50.f; // does not take into account rounding or anything like that
		DO_TEST(result.r == expectation, "Incorrect rasterization (result=%d,expected=%d,xpos=%.8f,screencoord=%.6f)", result.r, expectation, xpos, estimated_screencoord);

		++index;
	}

	// Guardband clipping indeed uses floating point math!
	// Hence, the smallest floating point value smaller than -2.0 will yield a clipped primitive.
	CGX_SetViewport(100.0f, 100.0f, 100.0f, 100.0f, 0.0f, 1.0f);
	for (float xpos : {-2.0000000, -2.0000002})
	{
		// first off, clear the full area, including the guardband
		GXTest::Quad().VertexTopLeft(-2.0, 2.0, 1.0).VertexBottomLeft(-2.0, -2.0, 1.0).VertexTopRight(2.0, 2.0, 1.0).VertexBottomRight(2.0, -2.0, 1.0).ColorRGBA(0,0,0,255).Draw();

		// now, draw the actual testing quad such that all vertices are outside the viewport (and on the same side of the viewport)
		// The two left vertices are at the border of the guardband; if they are outside the guardband, the primitive gets clipped away.
		GXTest::Quad().VertexTopLeft(xpos, 1.0, 1.0).VertexBottomLeft(xpos, -1.0, 1.0).VertexTopRight(xpos+1.0, 1.0, 1.0).VertexBottomRight(xpos+1.0, -1.0, 1.0).ColorRGBA(255,0,255,255).Draw();
		GXTest::CopyToTestBuffer(50, 100, 127, 127);
		CGX_WaitForGpuToFinish();
		GXTest::DebugDisplayEfbContents();

		GXTest::Vec4<u8> result = GXTest::ReadTestBuffer(10, 10, 128);

		int expectation = (xpos >= -2.0) ? 255 : 0;
		DO_TEST(result.r == expectation, "Incorrect guardband clipping (result=%d,expected=%d,xpos=%.10f)", result.r, expectation, xpos);
	};

	END_TEST();
}
int main()
{
	network_init();
	WPAD_Init();

	GXTest::Init();

	BitfieldTest();
	TevCombinerTest();
	ClipTest();
	CoordinatePrecisionTest();

	network_printf("Shutting down...\n");
	network_shutdown();

	return 0;
}
