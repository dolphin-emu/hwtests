// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "Test.h"
#include <stdlib.h>
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
	DO_TEST(reg.alpha == 837, "Values don't match (have: %d %d)", (s32)reg.alpha, (s32)reg.alpha);
	DO_TEST(reg.red == -392, "Values don't match (have: %d %d)", (s32)reg.red, (s32)reg.red);
	reg.low = 0x4BC6A8;
	DO_TEST(reg.alpha == -836, "Values don't match (have: %d %d)", (s32)reg.alpha);
	DO_TEST(reg.red == -344, "Values don't match (have: %d %d)", (s32)reg.red);
	reg.alpha = -263;
	reg.red = -345;
	DO_TEST(reg.alpha == -263, "Values don't match (have: %d %d)", (s32)reg.alpha);
	DO_TEST(reg.red == -345, "Values don't match (have: %d %d)", (s32)reg.red);
	reg.alpha = 15;
	reg.red = -619;
	DO_TEST(reg.alpha == 15, "Values don't match (have: %d %d)", (s32)reg.alpha);
	DO_TEST(reg.red == -619, "Values don't match (have: %d %d)", (s32)reg.red);
	reg.alpha = 523;
	reg.red = 176;
	DO_TEST(reg.alpha == 523, "Values don't match (have: %d %d)", (s32)reg.alpha);
	DO_TEST(reg.red == 176, "Values don't match (have: %d %d)", (s32)reg.red);

	END_TEST();
}

int TevCombinerExpectation(int a, int b, int c, int d, int shift, int bias, int op, int clamp)
{
	// TODO: Does not handle compare mode, yet
	c = c+(c>>7);
	u16 lshift = (shift == 1) ? 1 : (shift == 2) ? 2 : 0;
	u16 rshift = (shift == 3) ? 1 : 0;
	int round_bias = (shift==3) ? 0 /*: (cc.op==1) ? -128*/ : 128; // TODO: No idea if cc.op plays any role here.. results are still different anyway
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

	CGX_LOAD_BP_REG(CGXDefault<TevStageCombiner::AlphaCombiner>(0).hex);

	// Test if we can extract all bits of the tev combiner output...
	auto tevreg = CGXDefault<TevReg>(1, false); // c0
#if 0
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

		int result = GXTest::GetTevOutput(genmode, cc).r;

		DO_TEST(result == tevreg.red, "Source test value %d: Got %d", tevreg.red, result);
	}
#endif

	// Now: Randomized testing of tev combiners.
	for (int i = 0x000000; i < 0x010000; ++i)
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
		cc.d = TEVCOLORARG_ZERO;// TEVCOLORARG_CPREV; // NOTE: TEVCOLORARG_CPREV doesn't actually seem to fetch its data from PREV when used in the first stage?
		cc.shift = rand() % 4;
		cc.bias = rand() % 3;
		cc.op = 0; //rand()%2;
		cc.clamp = rand() % 2;
		CGX_LOAD_BP_REG(cc.hex);

		int a = rand() % 255;
		int b = rand() % 255;
		int c = rand() % 255;
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

		int result = GXTest::GetTevOutput(genmode, cc).r;

		int expected = TevCombinerExpectation(a, b, c, d, cc.shift, cc.bias, cc.op, cc.clamp);
		DO_TEST(result == expected, "Mismatch on a=%d, b=%d, c=%d, d=%d, shift=%d, bias=%d, op=%d, clamp=%d: expected %d, got %d", a, b, c, d, (u32)cc.shift, (u32)cc.bias, (u32)cc.op, (u32)cc.clamp, expected, result);

		WPAD_ScanPads();

		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
			break;
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

	for (int i = 0; i < 200; ++i)
	{
		network_printf("step %d\n", i);

		GX_SetViewport(100, 100, 50.f, 50, 0, 1);

		auto zmode = CGXDefault<ZMode>();
		CGX_LOAD_BP_REG(zmode.hex);

		auto cc = CGXDefault<TevStageCombiner::ColorCombiner>(0);
		cc.d = TEVCOLORARG_RASC;
		CGX_LOAD_BP_REG(cc.hex);
		GXTest::Quad().ColorRGBA(0,0x7f,0,0xff).Draw();

		cc.d = TEVCOLORARG_C0;
		CGX_LOAD_BP_REG(cc.hex);

		auto tevreg = CGXDefault<TevReg>(1, false); // c0
		tevreg.red = rand()%256;
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);

		CGX_BEGIN_LOAD_XF_REGS(0x1005, 1);
		wgPipe->U32 = 0; // 0 = enable clipping, 1 = disable clipping

		GXTest::Quad().ColorRGBA(0xff,0xff,0xff,0xff).AtDepth(1.00000001).Draw();
		CGX_WaitForGpuToFinish();

		GXTest::DebugDisplayEfbContents();
	}

	END_TEST();
}

int main()
{
	network_init();
	WPAD_Init();

	GXTest::Init();

	BitfieldTest();
//	TevCombinerTest();
	ClipTest();

	network_printf("Shutting down...\n");
	network_shutdown();

	return 0;
}
