#include "Test.h"
#include <stdlib.h>
#include <stdarg.h>
#include <network.h>
#include <gccore.h>
#include <gccore.h>
#include <ogc/video.h>
#include <wiiuse/wpad.h>
#include "cgx.h"

static u32 fb = 0;
static void *frameBuffer[2] = { NULL, NULL};
static GXRModeObj *rmode;

float yscale;
u32 xfbHeight;

union EfbCopyPixelRGBA8
{
	u32 hex;
};

int client_socket;
int server_socket;

#define TEST_BUFFER_SIZE (640*528*4)
static EfbCopyPixelRGBA8* test_buffer;

u8 GetTestBufferR(int s, int t, int width)
{
	u16 sBlk = s >> 2;
	u16 tBlk = t >> 2;
	u16 widthBlks = (width >> 2) + 1;
	u32 base = (tBlk * widthBlks + sBlk) << 5;
	u16 blkS = s & 3;
	u16 blkT =  t & 3;
	u32 blkOff = (blkT << 2) + blkS;

	u32 offset = (base + blkOff) << 1 ;
	const u8* valAddr = ((u8*)test_buffer) + offset;

	return valAddr[1];
}

u8 GetTestBufferG(int s, int t, int width)
{
	u16 sBlk = s >> 2;
	u16 tBlk = t >> 2;
	u16 widthBlks = (width >> 2) + 1;
	u32 base = (tBlk * widthBlks + sBlk) << 5;
	u16 blkS = s & 3;
	u16 blkT =  t & 3;
	u32 blkOff = (blkT << 2) + blkS;

	u32 offset = (base + blkOff) << 1 ;
	const u8* valAddr = ((u8*)test_buffer) + offset;

	return valAddr[32];
}

u8 GetTestBufferB(int s, int t, int width)
{
	u16 sBlk = s >> 2;
	u16 tBlk = t >> 2;
	u16 widthBlks = (width >> 2) + 1;
	u32 base = (tBlk * widthBlks + sBlk) << 5;
	u16 blkS = s & 3;
	u16 blkT =  t & 3;
	u32 blkOff = (blkT << 2) + blkS;

	u32 offset = (base + blkOff) << 1 ;
	const u8* valAddr = ((u8*)test_buffer) + offset;

	return valAddr[33];
}

u8 GetTestBufferA(int s, int t, int width)
{
	u16 sBlk = s >> 2;
	u16 tBlk = t >> 2;
	u16 widthBlks = (width >> 2) + 1;
	u32 base = (tBlk * widthBlks + sBlk) << 5;
	u16 blkS = s & 3;
	u16 blkT =  t & 3;
	u32 blkOff = (blkT << 2) + blkS;

	u32 offset = (base + blkOff) << 1 ;
	const u8* valAddr = ((u8*)test_buffer) + offset;

	return valAddr[0];
}

template<typename T>
T Default();

template<typename T>
T Default(int);

template<typename T>
T Default(int, bool);

template<>
GenMode Default<GenMode>()
{
	GenMode genmode;
	genmode.hex = BPMEM_GENMODE<<24;
	genmode.numtexgens = 0;
	genmode.numcolchans = 1;
	genmode.numtevstages = 0; // One stage
	genmode.cullmode = 0; // No culling
	genmode.numindstages = 0;
	genmode.zfreeze = 0;
	return genmode;
}

template<>
TevStageCombiner::ColorCombiner Default<TevStageCombiner::ColorCombiner>(int stage)
{
	TevStageCombiner::ColorCombiner cc;
	cc.hex = (BPMEM_TEV_COLOR_ENV+2*stage)<<24;
	cc.a = TEVCOLORARG_ZERO;
	cc.b = TEVCOLORARG_ZERO;
	cc.c = TEVCOLORARG_ZERO;
	cc.d = TEVCOLORARG_ZERO;
	cc.op = TEVOP_ADD;
	cc.bias = 0;
	cc.shift = TEVSCALE_1;
	cc.clamp = 0;
	cc.dest = GX_TEVPREV;
	return cc;
}

template<>
TevStageCombiner::AlphaCombiner Default<TevStageCombiner::AlphaCombiner>(int stage)
{
	TevStageCombiner::AlphaCombiner ac;
	ac.hex = (BPMEM_TEV_ALPHA_ENV+2*stage)<<24;
	ac.a = TEVALPHAARG_ZERO;
	ac.b = TEVALPHAARG_ZERO;
	ac.c = TEVALPHAARG_ZERO;
	ac.d = TEVALPHAARG_ZERO;
	ac.op = TEVOP_ADD;
	ac.bias = 0;
	ac.shift = TEVSCALE_1;
	ac.clamp = 1;
	ac.dest = GX_TEVPREV;
	return ac;
}

template<>
TwoTevStageOrders Default<TwoTevStageOrders>(int index)
{
	TwoTevStageOrders orders;
	orders.hex = (BPMEM_TREF+index)<<24;
	orders.texmap0 = GX_TEXMAP_NULL;
	orders.texcoord0 = GX_TEXCOORDNULL;
	orders.enable0 = 0;
	orders.colorchan0 = 0; // equivalent to GX_COLOR0A0
	return orders;
}

template<>
TevReg Default<TevReg>(int index, bool is_konst_color)
{
	TevReg tevreg;
	tevreg.hex = 0;
	tevreg.low = (BPMEM_TEV_REGISTER_L+2*index)<<24;
	tevreg.high = (BPMEM_TEV_REGISTER_H+2*index)<<24;
	tevreg.type_ra = is_konst_color;
	tevreg.type_bg = is_konst_color;
	return tevreg;
}

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

union int4
{
	struct
	{
		int r, g, b, a;
	};
	struct
	{
		int x, y, z, w;
	};
};

// TODO: Make this behave flexible with regards to the current EFB format!
int4 GetTevOutput(const GenMode& genmode, const TevStageCombiner::ColorCombiner& last_cc)
{
	int previous_stage = ((last_cc.hex >> 24)-BPMEM_TEV_COLOR_ENV)>>1;
	assert(previous_stage < 13);

	// FIRST RENDER PASS:
	// As set up by the caller, used to retrieve lower 8 bits of the TEV output

//	memset(test_buffer, 0, TEST_BUFFER_SIZE); // Just for debugging
	CGX_DrawFullScreenQuad(rmode->fbWidth, rmode->efbHeight);
	CGX_DoEfbCopyTex(0, 0, 100, 100, 0x6 /*RGBA8*/, false, test_buffer);
	CGX_ForcePipelineFlush();
	CGX_WaitForGpuToFinish();
	u16 result1r = GetTestBufferR(5, 5, 100);
	u16 result1g = GetTestBufferG(5, 5, 100);
	u16 result1b = GetTestBufferB(5, 5, 100);

	// SECOND RENDER PASS
	// Uses three additional TEV stages which shift the previous result
	// three bits to the right. This is necessary to read off the upper bits,
	// which got masked off when writing to the EFB in the first pass.
	auto gm = genmode;
	gm.numtevstages = previous_stage + 3; // three additional stages
	CGX_LOAD_BP_REG(gm.hex);

	// The following tev stages are exclusively used to rightshift the
	// upper bits such that they get written to the render target.

	auto cc1 = Default<TevStageCombiner::ColorCombiner>(previous_stage+1);
	cc1.d = last_cc.dest * 2;
	cc1.shift = TEVDIVIDE_2;
	CGX_LOAD_BP_REG(cc1.hex);

	cc1 = Default<TevStageCombiner::ColorCombiner>(previous_stage+2);
	cc1.d = last_cc.dest * 2;
	cc1.shift = TEVDIVIDE_2;
	CGX_LOAD_BP_REG(cc1.hex);

	cc1 = Default<TevStageCombiner::ColorCombiner>(previous_stage+3);
	cc1.d = last_cc.dest * 2;
	cc1.shift = TEVDIVIDE_2;
	CGX_LOAD_BP_REG(cc1.hex);

	memset(test_buffer, 0, TEST_BUFFER_SIZE);
	CGX_DrawFullScreenQuad(rmode->fbWidth, rmode->efbHeight);
	CGX_DoEfbCopyTex(0, 0, 100, 100, 0x6 /*RGBA8*/, false, test_buffer);
	CGX_ForcePipelineFlush();
	CGX_WaitForGpuToFinish();

	u16 result2r = GetTestBufferR(5, 5, 100) >> 5;
	u16 result2g = GetTestBufferG(5, 5, 100) >> 5;
	u16 result2b = GetTestBufferB(5, 5, 100) >> 5;

	// uh.. let's just say this works, but I guess it could be simplified.
	int4 result;
	result.r = result1r + ((result2r & 0x4) ? (-0x400+((result2r&0x3)<<8)) : (result2r<<8));
	result.g = result1g + ((result2g & 0x4) ? (-0x400+((result2g&0x3)<<8)) : (result2g<<8));
	result.b = result1b + ((result2b & 0x4) ? (-0x400+((result2b&0x3)<<8)) : (result2b<<8));
	// TODO: alpha bits!

	return result;
}

void TevCombinerTest()
{
	START_TEST();

	CGX_LOAD_BP_REG(Default<TwoTevStageOrders>(0).hex);

	CGX_BEGIN_LOAD_XF_REGS(0x1009, 1);
	wgPipe->U32 = 1; // 1 color channel

	LitChannel chan;
	chan.hex = 0;
	chan.matsource = 1; // from vertex
	CGX_BEGIN_LOAD_XF_REGS(0x100e, 1); // color channel 1
	wgPipe->U32 = chan.hex;
	CGX_BEGIN_LOAD_XF_REGS(0x1010, 1); // alpha channel 1
	wgPipe->U32 = chan.hex;

	CGX_LOAD_BP_REG(Default<TevStageCombiner::AlphaCombiner>(0).hex);

#if 1
	// Test if we can extract all bits of the tev combiner output...
	auto tevreg = Default<TevReg>(1, false); // c0
	for (tevreg.red = -1024; tevreg.red != 1023; tevreg.red = tevreg.red+1)
	{
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);

		auto genmode = Default<GenMode>();
		genmode.numtevstages = 0; // One stage
		CGX_LOAD_BP_REG(genmode.hex);

		auto cc = Default<TevStageCombiner::ColorCombiner>(0);
		cc.d = TEVCOLORARG_C0;
		CGX_LOAD_BP_REG(cc.hex);

		int result = GetTevOutput(genmode, cc).r;

		DO_TEST(result == tevreg.red, "Source test value %d: Got %d", tevreg.red, result);
	}
#endif

	// Now: Randomized testing of tev combiners.
	for (int i = 0x000000; i < 0x010000; ++i)
	{
		if ((i & 0xFF00) == i)
			network_printf("progress: %x\n", i);

		auto genmode = Default<GenMode>();
		genmode.numtevstages = 0; // One stage
		CGX_LOAD_BP_REG(genmode.hex);

		// Randomly configured TEV stage, output in PREV.
		auto cc = Default<TevStageCombiner::ColorCombiner>(0);
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
		tevreg = Default<TevReg>(1, false); // c0
		tevreg.red = a;
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);
		tevreg = Default<TevReg>(2, false); // c1
		tevreg.red = b;
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);
		tevreg = Default<TevReg>(3, false); // c2
		tevreg.red = c;
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);
		tevreg = Default<TevReg>(0, false); // prev
		tevreg.red = d;
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);

		int result = GetTevOutput(genmode, cc).r;

		int expected = TevCombinerExpectation(a, b, c, d, cc.shift, cc.bias, cc.op, cc.clamp);
		DO_TEST(result == expected, "Mismatch on a=%d, b=%d, c=%d, d=%d, shift=%d, bias=%d, op=%d, clamp=%d: expected %d, got %d", a, b, c, d, (u32)cc.shift, (u32)cc.bias, (u32)cc.op, (u32)cc.clamp, expected, result);

		WPAD_ScanPads();

		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
			break;
	}
	END_TEST();
}

int main()
{
	GXColor background = {0, 0x27, 0, 0xff};

	network_init();
	VIDEO_Init();
	WPAD_Init();

	network_printf("Hello world!\n");

	rmode = VIDEO_GetPreferredMode(NULL);

	frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(frameBuffer[fb]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	CGX_Init();

	GX_SetCopyClear(background, 0x00ffffff);
	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
    GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, 1, rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight==2*rmode->xfbHeight)?1:0));
	GX_SetDispCopyGamma(GX_GM_1_0);

	test_buffer = (EfbCopyPixelRGBA8*)memalign(32, 640*528*4);

	BitfieldTest();

	GX_SetTexCopySrc(0, 0, 100, 100);
	GX_SetTexCopyDst(100, 100, GX_TF_RGBA8, false);

	/*	PE_CONTROL ctrl;
	ctrl.hex = BPMEM_ZCOMPARE<<24;
	ctrl.pixel_format = PIXELFMT_RGBA6_Z24;
	ctrl.zformat = ZC_LINEAR;
	ctrl.early_ztest = 0;
	CGX_LOAD_BP_REG(ctrl.hex);*/

	// Dummy quad. Just to apply cached GX state...
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

	GX_SetNumChans(1); // damnit dirty state...
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

	// Testing begins here!
//	TevCombinerTest();

	// Shut down...
	network_printf("Shutting down...\n");
	network_shutdown();

	return 0;
}
