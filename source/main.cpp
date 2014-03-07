#include "Test.h"
#include <gccore.h>
#include <ogc/video.h>
#include "cgx.h"

static u32 fb = 0;
static void *frameBuffer[2] = { NULL, NULL};
static GXRModeObj *rmode;

union EfbCopyPixelRGBA8
{
	u32 hex;
};

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

u32 DefaultGenMode()
{
	GenMode genmode;
	genmode.hex = 0;
	genmode.numtexgens = 0;
	genmode.numcolchans = 1;
	genmode.numtevstages = 0; // One stage
	genmode.cullmode = 0; // No culling
	genmode.numindstages = 0;
	genmode.zfreeze = 0;
	return genmode.hex;
}

template<typename T>
T Default();

template<typename T>
T Default(int i);

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

void BitfieldTest()
{
	START_TEST();

	ColReg reg;
	reg.hex = 0x345678;
	DO_TEST(reg.a == -392, "Values don't match (have: %d %d)", (s32)reg.a, (s32)reg.a);
	DO_TEST(reg.b == 837, "Values don't match (have: %d %d)", reg.b, reg.b);
	reg.hex = 0x4BC6A8;
	DO_TEST(reg.a == -344, "Values don't match (have: %d %d)", reg.a);
	DO_TEST(reg.b == -836, "Values don't match (have: %d %d)", reg.b);
	reg.a = -263;
	reg.b = -345;
	DO_TEST(reg.a == -263, "Values don't match (have: %d %d)", reg.a);
	DO_TEST(reg.b == -345, "Values don't match (have: %d %d)", reg.b);
	reg.a = 15;
	reg.b = -619;
	DO_TEST(reg.a == 15, "Values don't match (have: %d %d)", reg.a);
	DO_TEST(reg.b == -619, "Values don't match (have: %d %d)", reg.b);
	reg.a = 523;
	reg.b = 176;
	DO_TEST(reg.a == 523, "Values don't match (have: %d %d)", reg.a);
	DO_TEST(reg.b == 176, "Values don't match (have: %d %d)", reg.b);

	END_TEST();
}

void TevCombinerTest()
{
	START_TEST();

	CGX_LOAD_BP_REG(DefaultGenMode());
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

	auto cc = Default<TevStageCombiner::ColorCombiner>(0);
	cc.a = TEVCOLORARG_C0;
	cc.b = TEVCOLORARG_C1;
	cc.c = TEVCOLORARG_C2;
	cc.d = TEVCOLORARG_ZERO;
	CGX_LOAD_BP_REG(cc.hex);

	CGX_LOAD_BP_REG(Default<TevStageCombiner::AlphaCombiner>(0).hex);

	ColReg reg;
	reg.hex = (BPMEM_TEV_REGISTER_L+2)<<24; // a0
	reg.a = 0x00;
	CGX_LOAD_BP_REG(reg.hex);
	reg.hex = (BPMEM_TEV_REGISTER_L+4)<<24; // a1
	reg.a = 0xff;
	CGX_LOAD_BP_REG(reg.hex);

	for (int i = 0; i < 0x100; ++i)
	{
		reg.hex = (BPMEM_TEV_REGISTER_L+6)<<24; // a2
		reg.a = i;
		CGX_LOAD_BP_REG(reg.hex);

		CGX_DrawFullScreenQuad(rmode->fbWidth, rmode->efbHeight);
		CGX_DoEfbCopyTex(0, 0, rmode->fbWidth, rmode->efbHeight, 0x6 /*RGBA8*/, false, test_buffer);
//		CGX_DoEfbCopyTex(0, 0, 10, 10, 0x6 /*RGBA8*/, false, test_buffer);
		CGX_ForcePipelineFlush();
		CGX_WaitForGpuToFinish();

	//	DO_TEST(GetTestBufferA(5, 5, rmode->fbWidth) == 0x13, "Wrong pixel read.. 0x%x", test_buffer[100]);
//		printf("%d: %x %x %x %x\n", i, GetTestBufferR(5, 5, rmode->fbWidth), GetTestBufferG(5, 5, rmode->fbWidth), GetTestBufferB(5, 5, rmode->fbWidth), GetTestBufferA(5, 5, rmode->fbWidth));
		printf("%d: %x %x %x\n", i, GetTestBufferR(5, 5, rmode->fbWidth), 0xff * i / 255, (0xff * i) >> 8);
	}
	END_TEST();
}

int main()
{
	VIDEO_Init();

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

	test_buffer = (EfbCopyPixelRGBA8*)memalign(32, 640*528*4);

	BitfieldTest();

/*	PE_CONTROL ctrl;
	ctrl.hex = BPMEM_ZCOMPARE<<24;
	ctrl.pixel_format = PIXELFMT_RGBA6_Z24;
	ctrl.zformat = ZC_LINEAR;
	ctrl.early_ztest = 0;
	CGX_LOAD_BP_REG(ctrl.hex);*/

	do
	{
		CGX_SetViewport(0.0, 0.0, (float)rmode->fbWidth, (float)rmode->efbHeight, 0.0, 1.0);

		// TODO: For some reason I get invalid results on first loop?
		if (fb)
			TevCombinerTest();

		CGX_DoEfbCopyXfb(0, 0, rmode->fbWidth, rmode->efbHeight, &frameBuffer[fb], false, true);
		CGX_ForcePipelineFlush();

		VIDEO_SetNextFramebuffer(frameBuffer[fb]);

		VIDEO_Flush();
		VIDEO_WaitVSync();
		fb ^= 1; // flip framebuffer
	} while(true);

	return 0;
}
