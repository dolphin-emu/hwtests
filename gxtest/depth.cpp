// adapted from	the	original acube demo	by tkcne.

// enjoy

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#define HW_PEEK 13884592

GXRModeObj	*screenMode;
static void	*frameBuffer;
static vu8	readyForCopy;
#define	FIFO_SIZE (256*1024)

s16	vertices[] ATTRIBUTE_ALIGN(32) = {
	0, 15, 0,
	-15, -15, 0,
	15,	-15, 0};

u8 colors[]	ATTRIBUTE_ALIGN(32)	= {
	255, 0,	0, 255,		// red
	0, 255,	0, 255,		// green
	0, 0, 255, 255};	// blue

void update_screen(Mtx viewMatrix);
static void	copy_buffers(u32 unused);
static void	test_peek(u16 x, u16 y);

int	main(void) {
	Mtx	view;
	Mtx	projection;
	GXColor	backgroundColor	= {0, 0, 0,	255};
	void *fifoBuffer = NULL;
  
	VIDEO_Init();		
	WPAD_Init();
	
	screenMode = VIDEO_GetPreferredMode(NULL);

	frameBuffer	= MEM_K0_TO_K1(SYS_AllocateFramebuffer(screenMode));
	
	// Initialise the console, required for printf
	console_init(frameBuffer,20,20,screenMode->fbWidth,screenMode->xfbHeight,screenMode->fbWidth*VI_DISPLAY_PIX_SZ);

	VIDEO_Configure(screenMode);
	VIDEO_SetNextFramebuffer(frameBuffer);
	VIDEO_SetPostRetraceCallback(copy_buffers);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();



	fifoBuffer = MEM_K0_TO_K1(memalign(32,FIFO_SIZE));
	memset(fifoBuffer,	0, FIFO_SIZE);

	GX_Init(fifoBuffer, FIFO_SIZE);
	GX_SetCopyClear(backgroundColor, 0x00ffffff);
	GX_SetViewport(0,0,screenMode->fbWidth,screenMode->efbHeight,0,1);
	GX_SetDispCopyYScale((f32)screenMode->xfbHeight/(f32)screenMode->efbHeight);
	GX_SetScissor(0,0,screenMode->fbWidth,screenMode->efbHeight);
	GX_SetDispCopySrc(0,0,screenMode->fbWidth,screenMode->efbHeight);
	GX_SetDispCopyDst(screenMode->fbWidth,screenMode->xfbHeight);
	GX_SetCopyFilter(screenMode->aa,screenMode->sample_pattern,
					 GX_TRUE,screenMode->vfilter);
	GX_SetFieldMode(screenMode->field_rendering,
					((screenMode->viHeight==2*screenMode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	GX_SetCullMode(GX_CULL_NONE);
	GX_CopyDisp(frameBuffer,GX_TRUE);
	GX_SetDispCopyGamma(GX_GM_1_0);

	guVector camera =	{0.0F, 0.0F, 0.0F};
	guVector up =	{0.0F, 1.0F, 0.0F};
	guVector look	= {0.0F, 0.0F, -1.0F};

	guPerspective(projection, 60, 1.33F, 10.0F,	300.0F);
	GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_INDEX8);
	GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX8);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,	GX_POS_XYZ,	GX_S16,	0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,	0);
	GX_SetArray(GX_VA_POS, vertices, 3*sizeof(s16));
	GX_SetArray(GX_VA_CLR0,	colors,	4*sizeof(u8));
	GX_SetNumChans(1);
	GX_SetNumTexGens(0);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

	while (1)
	{
		guLookAt(view, &camera,	&up, &look);
		GX_SetViewport(0,0,screenMode->fbWidth,screenMode->efbHeight,0,1);
		GX_InvVtxCache();
		GX_InvalidateTexAll();
		update_screen(view);

		WPAD_ScanPads();
		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);
	}
	return 0;
}

void update_screen(	Mtx	viewMatrix )
{
	Mtx	modelView;
	
	guMtxIdentity(modelView);
	guMtxTransApply(modelView, modelView, 0.0F,	0.0F, -50.0F);
	guMtxConcat(viewMatrix,modelView,modelView);
	
	GX_LoadPosMtxImm(modelView,	GX_PNMTX0);

	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);

	GX_Position1x8(0);
	GX_Color1x8(0);
	GX_Position1x8(1);
	GX_Color1x8(1);
	GX_Position1x8(2);
	GX_Color1x8(2);
	
	GX_End();

	GX_DrawDone();
	readyForCopy = GX_TRUE;
	
	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	printf("\x1b[2;0H");
	
	test_peek(320, 264);

	VIDEO_WaitVSync();
	return;
}

static void	copy_buffers(u32 count __attribute__ ((unused)))
{
	if (readyForCopy==GX_TRUE) {
		GX_PokeZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		GX_SetColorUpdate(GX_TRUE);
		GX_CopyDisp(frameBuffer,GX_TRUE);
		GX_Flush();
		readyForCopy = GX_FALSE;
	}
}

static void	test_peek(u16 x, u16 y)
{
	u32 z = 0;
	
	// Test a peek
	GX_PeekZ(x, y, &z);
	printf("Z Peek: %d\n",z);

	// Compare to hardware
	if (z != HW_PEEK)
		printf("The Z value is INACCURATE!\n");
	else
		printf("The Z value is accurate.\n");

	// Mark the location
	GX_PokeBlendMode(GX_BM_LOGIC, 1, 0, GX_LO_COPY);
	GX_PokeColorUpdate(GX_TRUE);
	GXColor color = { 255, 255, 0, 255 };
	GX_PokeARGB(x, y, color);
}
