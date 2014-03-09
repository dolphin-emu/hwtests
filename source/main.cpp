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
	DO_TEST(reg.alpha == -392, "Values don't match (have: %d %d)", (s32)reg.alpha, (s32)reg.alpha);
	DO_TEST(reg.red == 837, "Values don't match (have: %d %d)", (s32)reg.red, (s32)reg.red);
	reg.low = 0x4BC6A8;
	DO_TEST(reg.alpha == -344, "Values don't match (have: %d %d)", (s32)reg.alpha);
	DO_TEST(reg.red == -836, "Values don't match (have: %d %d)", (s32)reg.red);
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
#include <unistd.h>
void TevCombinerTest()
{
	START_TEST();

	CGX_LOAD_BP_REG(Default<GenMode>().hex);
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

	auto tevreg = Default<TevReg>(1, false); // c0
	tevreg.red = 3;
	CGX_LOAD_BP_REG(tevreg.low);
	CGX_LOAD_BP_REG(tevreg.high);
	tevreg = Default<TevReg>(2, false); // c1
	tevreg.red = 0xf7;
	CGX_LOAD_BP_REG(tevreg.low);
	CGX_LOAD_BP_REG(tevreg.high);

	// First, test linear interpolation algorithm
	// Then test the order of lerp and scaling
	int step = 0;
	for (int i = 0; i < 0x1 || step < 3; ++i)
	{
		break;
		if (i == 0x100)
		{
			network_printf("step %d finished\n", step);
			step++;
			i = 0;

			if (step == 1)
			{
				// Enable scaling
				cc.shift = GX_CS_SCALE_4;
				CGX_LOAD_BP_REG(cc.hex);
			}
			else if (step == 2)
			{
				// Enable scaling, but only use the "d" input
				cc.a = TEVCOLORARG_ZERO;
				cc.b = TEVCOLORARG_ZERO;
				cc.c = TEVCOLORARG_ZERO;
				cc.d = TEVCOLORARG_C2;
//				cc.shift = GX_CS_SCALE_4;
				CGX_LOAD_BP_REG(cc.hex);
			}
			else if (step == 3)
			{
				// Enable scaling and lerp, a=c0, b=c2, c=c1, d=0
				cc.a = TEVCOLORARG_C0;
				cc.b = TEVCOLORARG_C2;
				cc.c = TEVCOLORARG_C1;
				cc.d = TEVCOLORARG_ZERO;
//				cc.shift = GX_CS_SCALE_4;
				CGX_LOAD_BP_REG(cc.hex);
			}
		}

		tevreg = Default<TevReg>(3, false); // c2
		tevreg.red = i;
		CGX_LOAD_BP_REG(tevreg.low);
		CGX_LOAD_BP_REG(tevreg.high);

		CGX_DrawFullScreenQuad(rmode->fbWidth, rmode->efbHeight);
		CGX_DoEfbCopyTex(0, 0, 100, 100, 0x6 /*RGBA8*/, false, test_buffer);
		CGX_ForcePipelineFlush();
		CGX_WaitForGpuToFinish();

		// Debug display code
		CGX_DoEfbCopyXfb(0, 0, rmode->fbWidth, rmode->efbHeight, xfbHeight, frameBuffer[fb], true);
		VIDEO_SetNextFramebuffer(frameBuffer[fb]);
		VIDEO_Flush();
		VIDEO_WaitVSync();
		fb ^= 1;

/*		if (step == 0)
			DO_TEST(GetTestBufferR(5, 5, 100) == 0xff * i / 255, "Wrong pixel read.. 0x%x", GetTestBufferR(5, 5, 100));
		if (step == 2)
			DO_TEST(GetTestBufferR(5, 5, 100) == ((i * 4) & 0xFF), "Wrong pixel read.. 0x%x", GetTestBufferR(5, 5, 100));
		if (step == 3)
			DO_TEST(GetTestBufferR(5, 5, 100) == ((i * 4) & 0xFF), "Wrong pixel read.. 0x%x", GetTestBufferR(5, 5, 100));*/

		network_printf("%d %d %d %d %d\n", i,
		               GetTestBufferR(5, 5, 100),  // hardware behavior
		               (0xff * i * ((step==1 || step==2 || step==3)?4:1) / 255),             // TFN behavior
		               ((0xff * (i + (i>>7))) >> 8) * ((step==1 || step==2 || step==3)?4:1), // software renderer behavior
		               (((0xff * i) >> 8)) * ((step==1 || step==2 || step==3)?4:1)             // suggested behavior
		              );

		WPAD_ScanPads();

		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
			break;
	}

	// OK, whatever.. just test everything
	cc.a = TEVCOLORARG_C0;
	cc.b = TEVCOLORARG_C1;
	cc.c = TEVCOLORARG_C2;
	cc.d = TEVCOLORARG_ZERO;
	cc.shift = GX_CS_SCALE_4;
	CGX_LOAD_BP_REG(cc.hex);
	for (int i = 0; i < 0x1000000; ++i)
	{
		int a = i >> 16;
		int b = (i>>8)&0xFF;
		int c = i&0xFF;
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

		CGX_DrawFullScreenQuad(rmode->fbWidth, rmode->efbHeight);
		CGX_DoEfbCopyTex(0, 0, 100, 100, 0x6 /*RGBA8*/, false, test_buffer);
		CGX_ForcePipelineFlush();
		CGX_WaitForGpuToFinish();

		// Debug display code
/*		CGX_DoEfbCopyXfb(0, 0, rmode->fbWidth, rmode->efbHeight, xfbHeight, frameBuffer[fb], true);
		VIDEO_SetNextFramebuffer(frameBuffer[fb]);
		VIDEO_Flush();
		VIDEO_WaitVSync();
		fb ^= 1;*/

		network_printf(" %02x %02x %02x: %02x %02x\n", a, b, c,
		               GetTestBufferR(5, 5, 100),  // hardware behavior
		               ((a*(255-c) + b*c) / 255) * 4             // TFN behavior
		              );

		WPAD_ScanPads();

		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
			break;
	}
	exit(0);

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


	do
	{
		WPAD_ScanPads();

		network_printf("frame...\n");

//		CGX_SetViewport(0.0, 0.0, (float)rmode->fbWidth, (float)rmode->efbHeight, 0.0, 1.0);

		GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);

		TevCombinerTest();

//		CGX_DoEfbCopyXfb(0, 0, rmode->fbWidth, rmode->efbHeight, &frameBuffer[fb], false, true);
//		CGX_ForcePipelineFlush();
/*		CGX_LOAD_BP_REG(DefaultGenMode());
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

		reg.hex = (BPMEM_TEV_REGISTER_L+6)<<24; // a2
		reg.a = 0x80;
		CGX_LOAD_BP_REG(reg.hex);*/
//		GX_SetNumChans(1); // damnit dirty state...
//		GX_SetNumTexGens(0);
//		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);*/

/*		CGX_DrawFullScreenQuad(rmode->fbWidth, rmode->efbHeight);
		CGX_ForcePipelineFlush();*/

		GX_DrawDone();
		GX_CopyDisp(frameBuffer[fb], true);

		VIDEO_SetNextFramebuffer(frameBuffer[fb]);

		VIDEO_Flush();
		VIDEO_WaitVSync();
		fb ^= 1; // flip framebuffer


		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A)
			goto stop;
	} while(true);

stop:

	network_printf("Shutting down...\n");
	network_shutdown();

	exit(0);
	return 0;
}
