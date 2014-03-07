#include "Test.h"
#include <gccore.h>
#include <ogc/video.h>
#include "cgx.h"

static u32 fb = 0;
static void *frameBuffer[2] = { NULL, NULL};
static GXRModeObj *rmode;

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

	do
	{
		CGX_SetViewport(0.0, 0.0, (float)rmode->fbWidth, (float)rmode->efbHeight, 0.0, 1.0);

		CGX_LOAD_BP_REG(DefaultGenMode());

		TevStageCombiner::ColorCombiner cc;
		cc.hex = BPMEM_TEV_COLOR_ENV<<24;
		cc.a = TEVCOLORARG_ZERO;
		cc.b = TEVCOLORARG_ZERO;
		cc.c = TEVCOLORARG_ZERO;
		cc.d = TEVCOLORARG_RASC;
		cc.op = TEVOP_ADD;
		cc.bias = 0;
		cc.shift = TEVSCALE_1;
		cc.clamp = 1;
		cc.dest = GX_TEVPREV;
		CGX_LOAD_BP_REG(cc.hex);

		TevStageCombiner::AlphaCombiner ac;
		ac.hex = BPMEM_TEV_ALPHA_ENV<<24;
		ac.a = TEVALPHAARG_ZERO;
		ac.b = TEVALPHAARG_ZERO;
		ac.c = TEVALPHAARG_ZERO;
		ac.d = TEVALPHAARG_RASA;
		ac.op = TEVOP_ADD;
		ac.bias = 0;
		ac.shift = TEVSCALE_1;
		ac.clamp = 1;
		ac.dest = GX_TEVPREV;
		CGX_LOAD_BP_REG(ac.hex);

		TwoTevStageOrders orders;
		orders.hex = BPMEM_TREF<<24;
		orders.texmap0 = GX_TEXMAP_NULL;
		orders.texcoord0 = GX_TEXCOORDNULL;
		orders.enable0 = 0;
		orders.colorchan0 = 0; // equivalent to GX_COLOR0A0
		CGX_LOAD_BP_REG(orders.hex);

		CGX_BEGIN_LOAD_XF_REGS(0x1009, 1);
		wgPipe->U32 = 1; // 1 color channel

		LitChannel chan;
		chan.hex = 0;
		chan.matsource = 1; // from vertex

		CGX_BEGIN_LOAD_XF_REGS(0x100e, 1); // color channel 1
		wgPipe->U32 = chan.hex;
		CGX_BEGIN_LOAD_XF_REGS(0x1010, 1); // alpha channel 1
		wgPipe->U32 = chan.hex;

		CGX_DrawFullScreenQuad(rmode->fbWidth, rmode->efbHeight);

		CGX_DoEfbCopyXfb(0, 0, rmode->fbWidth, rmode->efbHeight, &frameBuffer[fb], false, true);
		CGX_ForcePipelineFlush();

		VIDEO_SetNextFramebuffer(frameBuffer[fb]);

		VIDEO_Flush();
		VIDEO_WaitVSync();
		fb ^= 1; // flip framebuffer
	} while(true);

	return 0;
}
