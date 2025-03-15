#include <vga.h>
#include <llscreen.h>
#include <llvesa.h>
#include <text.h>

//#include <libraries/lowlevel.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/videocontrol.h>
#include <workbench/startup.h>
#include <clib/alib_protos.h>
#include <clib/debug_protos.h>
#include <proto/intuition.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/keymap.h>
#include <proto/lowlevel.h>
#include <proto/dos.h>
//#include <proto/timer.h>
#include <proto/graphics.h>
#include <proto/icon.h>

#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>

#include <SDI_compiler.h>

//#define FPS_COUNTER

static byte vga_palette[3 * 256] = {
#include "vga_palette.h"
};
//byte vga_memory[256 * 1024];

static struct Window *window = NULL;
static struct Screen *screen = NULL;
static unsigned char ppal[256 * 4];
static ULONG spal[1 + (256 * 3) + 1];
static int updatePalette = FALSE;
struct Library *CyberGfxBase = NULL;
static int use_c2p = 0;
static int use_wcp = 0;
//static int use_ham = 0;
static int currentBitMap;
static struct ScreenBuffer *sbuf[2];
//static struct RastPort temprp;
static struct MsgPort *dispport;
static int safetochange;
static ULONG fsMonitorID = INVALID_ID;
//static ULONG fsModeID = INVALID_ID;
static char wndPubScreen[32] = {"Workbench"};
static int rtg320x240 = FALSE;
static UWORD *pointermem;
static int tooltypesParsed = FALSE;
static int lastWidth = 0;
static int lastHeight = 0;
#ifdef FPS_COUNTER
static int showfps = TRUE;
static struct EClockVal timeval;
/*
static struct EClockVal timeval2;
dword t0, t1, t2, t3, t4, t5, t6, t7, t8, t9;
*/
#endif

extern void ASM c2p1x1_8_c5_bm_040(REG(d0, WORD chunkyx), REG(d1, WORD chunkyy), REG(d2, WORD offsx), REG(d3, WORD offsy), REG(a0, APTR chunkyscreen), REG(a1, struct BitMap *bitmap));
#define c2p_write_bm c2p1x1_8_c5_bm_040

static void parseTooltypes(void)
{
	char *exename;
	struct DiskObject *appicon;

	if (ArgC == 0) {
		struct WBStartup *startup = (struct WBStartup *)ArgV;
		exename = (char *)startup->sm_ArgList->wa_Name;//kprintf("%s:%ld %s\n", __FUNCTION__, __LINE__, exename);
	} else {
		exename = ArgV[0];//kprintf("%s:%ld %s\n", __FUNCTION__, __LINE__, exename);
	}

	if ((appicon = GetDiskObject((STRPTR)exename))) {
		char *value;

		if ((value = (char *)FindToolType((STRPTR *)appicon->do_ToolTypes, (CONST_STRPTR)"FORCEMODE"))) {
			if (!strcmp(value, "NTSC"))
				fsMonitorID = NTSC_MONITOR_ID;
			else if (!strcmp(value, "PAL"))
				fsMonitorID = PAL_MONITOR_ID;
			else if (!strcmp(value, "MULTISCAN"))
				fsMonitorID = VGA_MONITOR_ID;
			else if (!strcmp(value, "EURO72"))
				fsMonitorID = EURO72_MONITOR_ID;
			else if (!strcmp(value, "EURO36"))
				fsMonitorID = EURO36_MONITOR_ID;
			else if (!strcmp(value, "SUPER72"))
				fsMonitorID = SUPER72_MONITOR_ID;
			else if (!strcmp(value, "DBLNTSC"))
				fsMonitorID = DBLNTSC_MONITOR_ID;
			else if (!strcmp(value, "DBLPAL"))
				fsMonitorID = DBLPAL_MONITOR_ID;
		}
		/*if (FindToolType((CONST_STRPTR *)appicon->do_ToolTypes, (CONST_STRPTR)"FORCE640") != NULL) {
			fsForce640 = TRUE;
		}*/
		if (FindToolType((CONST STRPTR *)appicon->do_ToolTypes, (CONST STRPTR)"RTG320X240") != NULL) {
			rtg320x240 = TRUE;
		}

		FreeDiskObject(appicon);
	}
}

static void shutdownvideo(void)
{
	lastWidth = lastHeight = 0;

	if (window) {
		CloseWindow(window);
		window = NULL;
	}
	if (sbuf[0]) {
		FreeScreenBuffer(screen, sbuf[0]);
		sbuf[0] = NULL;
	}
	if (sbuf[1]) {
		FreeScreenBuffer(screen, sbuf[1]);
		sbuf[1] = NULL;
	}
	if (dispport) {
		/*
		if (!safetochange) {
			while (GetMsg(dispport));
			safetochange = TRUE;
		}
		*/
		DeleteMsgPort(dispport);
		dispport = NULL;
	}
	if (screen) {
		CloseScreen(screen);
		screen = NULL;
	}
	//use_c2p = 0;
	if (pointermem) {
		FreeVec(pointermem);
		pointermem = NULL;
	}
	if (CyberGfxBase) {
		CloseLibrary(CyberGfxBase);
		CyberGfxBase = NULL;
	}
}

static int setvideomode(int x, int y, int c, int fs)
{
	if (!tooltypesParsed) {
		tooltypesParsed = TRUE;
		parseTooltypes();
	}

	//kprintf("%s %ld %ld %ld %ld\n", __FUNCTION__, lastWidth, x, lastHeight, y);
	if (lastWidth == x && lastHeight == y) {
		return 0;
	}

	//kprintf("%s(%ld,%ld,%ld,%ld)\n", __FUNCTION__, x, y, c, fs);

	shutdownvideo();

	CyberGfxBase = OpenLibrary((STRPTR)"cybergraphics.library", 41);
	if (fs) {
		ULONG modeID = INVALID_ID;

		if (fsMonitorID != (ULONG)INVALID_ID) {
			//printf("Using forced monitor: %08x\n", fsMonitorID);
		} else if (CyberGfxBase) {
			if (!(rtg320x240 && x == 320 && y == 200)) {
				modeID = BestCModeIDTags(
					CYBRBIDTG_Depth, 8,
					CYBRBIDTG_NominalWidth, x,
					CYBRBIDTG_NominalHeight, y,
					TAG_DONE);
			}

			if (modeID == (ULONG)INVALID_ID && x == 320 && y == 200) {
				// some cards like the Voodoo 3 lack a 320x200 mode
				modeID = BestCModeIDTags(
					CYBRBIDTG_Depth, 8,
					CYBRBIDTG_NominalWidth, 320,
					CYBRBIDTG_NominalHeight, 240,
					TAG_DONE);
			}
			//printf("%s RTG ID: %08lx\n", __FUNCTION__, modeID);
		}

		if (modeID == (ULONG)INVALID_ID) {
			modeID = BestModeID(
				BIDTAG_NominalWidth, x,
				BIDTAG_NominalHeight, y,
				BIDTAG_Depth, 8,
				//BIDTAG_DIPFMustNotHave, SPECIAL_FLAGS|DIPF_IS_LACE,
				(fsMonitorID == (ULONG)INVALID_ID) ? TAG_IGNORE : BIDTAG_MonitorID, fsMonitorID,
				TAG_DONE);
			//printf("%s native ID: %08lx\n", __FUNCTION__, modeID);
		}

		struct TagItem vctl[] =
		{
			//{VTAG_BORDERBLANK_SET, TRUE},
			{VC_IntermediateCLUpdate, FALSE},
			{VTAG_END_CM, 0}
		};

		screen = OpenScreenTags(0,
			modeID != (ULONG)INVALID_ID ? SA_DisplayID : TAG_IGNORE, modeID,
			//ntscHack != 0 ? SA_Top : TAG_IGNORE, ntscHack,
			SA_Width, x,
			SA_Height, y,
			SA_Depth, 8,
			SA_ShowTitle, FALSE,
			SA_Quiet, TRUE,
			SA_Draggable, FALSE,
			SA_Type, CUSTOMSCREEN,
			SA_VideoControl, (ULONG)vctl,
			//SA_DetailPen, blackcol,
			//SA_BlockPen, whitecol,
			//SA_Overscan, OSCAN_MAX,
			TAG_DONE);

		/*
		spal[0] = 256 << 16;
		SetRast(&screen->RastPort, 0);
		for (int i = 0; i < 256; i++)
			SetRGB32(&screen->ViewPort, i, 0, 0, 0);
		*/

		currentBitMap = 0;
		use_c2p = use_wcp = 0;

		if ((GetBitMapAttr(screen->RastPort.BitMap, BMA_FLAGS) & BMF_STANDARD)) {
			if (!(sbuf[0] = AllocScreenBuffer(screen, 0, SB_SCREEN_BITMAP)) || !(sbuf[1] = AllocScreenBuffer(screen, 0, SB_COPY_BITMAP))) {
				shutdownvideo();
				puts("Could not allocate the screen buffers");
				return -1;
			}
			//InitRastPort(&temprp);

			use_c2p = TRUE;
			/*
			safetochange = TRUE;
			dispport = CreateMsgPort();
			sbuf[0]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort = dispport;
			sbuf[1]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort = dispport;
			*/
		}
	}

	ULONG flags = WFLG_ACTIVATE | WFLG_RMBTRAP;
	//idcmp = /*IDCMP_CLOSEWINDOW | IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW |*/ IDCMP_RAWKEY;
	if (screen) {
		flags |= WFLG_BACKDROP | WFLG_BORDERLESS;
	} else {
		flags |=  WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET;
	}

	window = OpenWindowTags(0,
		WA_InnerWidth, x,
		WA_InnerHeight, y,
		screen ? TAG_IGNORE : WA_Title, (ULONG)"Speed Haste",
		WA_Flags, flags,
		screen ? WA_CustomScreen : TAG_IGNORE, (ULONG)screen,
		!screen ? WA_PubScreenName : TAG_IGNORE, (ULONG)wndPubScreen,
		//WA_IDCMP, idcmp,
		TAG_DONE);

	if (window == NULL) {
		shutdownvideo();
		puts("Could not open the window");
		return -1;
	}

	//pointermem = (UWORD *)AllocVec(2 * 6, MEMF_CHIP | MEMF_CLEAR);
	if (pointermem && window->Pointer != pointermem) {
		SetPointer(window, pointermem, 1, 1, 0, 0);
	}

	lastWidth = x;
	lastHeight = y;

#ifdef FPS_COUNTER
	ElapsedTime(&timeval);
#endif

	return 0;
}

#ifdef FPS_COUNTER
#include <sys/time.h>
static int time_started = FALSE;

/*
#define MS(x) ((x) >> 6)
dword getdelta(void)
{
	time_started = TRUE;
	ULONG timecount = ElapsedTime(&timeval2);
	return timecount;
#endif
}
*/
#endif

static void showframe(void)
{
	if (!screen) {
		return;
	}

#ifdef FPS_COUNTER
	if (showfps) {
		static char buf[64];
		static int lastfps = 0;
		static ULONG lasttime = 0;
		ULONG timecount = ElapsedTime(&timeval);
		int frametime = (timecount + lasttime) >> 1;
#ifdef __HAVE_68881__
		frametime = (int)((double)frametime*1000.0/65536.0);
#else
		frametime >>= 6; // TODO this is 2,4% slower
#endif
		int fps = (((1 << 16) / timecount) + lastfps) >> 1;
		sprintf(buf, "%2d fps (%2d ms)", fps, frametime);
		TEXT_Write(&FONT_Border, 0, 0, buf, 15);
		lastfps = fps;
		lasttime = timecount;
		/*
		if (time_started) {
			sprintf(buf, "t0 %2d t1 %2d t2 %2d t3 %2d t4 %2d t5 %2d t6 %2d t7 %2d t8 %2d t9 %2d ", MS(t0), MS(t1), MS(t2), MS(t3), MS(t4), MS(t5), MS(t6), MS(t7), MS(t8), MS(t9));
			TEXT_Write(&FONT_Border, 0, 9, buf, 15);
		}
		*/
	}
#endif

	if (use_c2p) {
		currentBitMap ^= 1;
		c2p_write_bm(LLS_SizeX, LLS_SizeY, 0, 0, LLS_Screen_, sbuf[currentBitMap]->sb_BitMap);
		if (dispport) {
			if (!safetochange) {
				while (!GetMsg(dispport)) WaitPort(dispport);
				safetochange = TRUE;
			}
		}
		if (ChangeScreenBuffer(screen, sbuf[currentBitMap])) {
			safetochange = FALSE;
		}
	} else if (CyberGfxBase) {
		// TODO upscale the menu when SVGAOn == TRUE
		/*if (screen->Width >= 640 && screen->Height >= 400) {
			ScalePixelArray(GX.chunky, GX.width, GX.height, GX.width, window->RPort, 0, 0, GX.width*2, GX.height*2, RECTFMT_LUT8);
		} else {*/
			WritePixelArray(LLS_Screen_, 0, 0, LLS_SizeX, window->RPort, 0, 0, LLS_SizeX, LLS_SizeY, RECTFMT_LUT8);
		//}
	}
	//WriteChunkyPixels(window->RPort, 0, 0, LLS_SizeX - 1, LLS_SizeY - 1, LLS_Screen_, LLS_SizeX);
	if (updatePalette) {
		LoadRGB32(&screen->ViewPort, spal);
		updatePalette = FALSE;
	}
}

void flushpalette(void)
{
	if (updatePalette) {
		LoadRGB32(&screen->ViewPort, spal);
		updatePalette = FALSE;
	}
}

static void setpalette(void)
{
	int i;

	if (screen) {
		//static ULONG spal[1 + (256 * 3) + 1];
		ULONG *sp = spal;
		const byte *vp = vga_palette;

		*sp++ = 256 << 16;
		for (i = 0; i < 256; i++) {
			*sp++ = ((ULONG)*vp++) << 26;
			*sp++ = ((ULONG)*vp++) << 26;
			*sp++ = ((ULONG)*vp++) << 26;
		}
		*sp = 0;

		//LoadRGB32(&screen->ViewPort, spal);
	} else {
		unsigned char *pp = ppal;
		const byte *vp = vga_palette;

		for (i = 0; i < 256; i++)
		{
			*pp++ = 0;
			*pp++ = *vp++ << 2;
			*pp++ = *vp++ << 2;
			*pp++ = *vp++ << 2;
		}

		showframe();
	}
}


bool LLV_SetModeRez(int w, int h, int depth) {
	//kprintf("%s(%ld,%ld,%ld)\n", __FUNCTION__, w, h, depth);
    return !setvideomode(w, h, 8, 1);
}

bool LLV_Init(void) {
    return TRUE;
}

void LLV_PutBuffer(const byte *org, int miny, int maxy) {
	showframe();
}

void LLS_UpdateVGA(void) {
	showframe();
}

void VGA_SetMode(byte mode)
{
	//kprintf("%s(%lx)\n", __FUNCTION__, mode);
	if (mode == 0x13) {
		setvideomode(LLS_SizeX, LLS_SizeY, 8, 1);
	} else {
		shutdownvideo();
	}
	/*
	3 - text mode
	0x13 - 320x200
	*/
}

void VGA_Tweak(void)
{
	// intro LLSVM_MODEY
}

void VGA_Set240(void)
{
	// unused
}

void VGA_Set360(void)
{
	// unused
}

void VGA_GetPalette(byte *dest, int start, int n)
{
	//kprintf("%s(%lx,%ld,%ld)\n", __FUNCTION__, dest, start, n);
	if (start == 0 && n == 256) {
		memcpy(dest, vga_palette, 768);
	} else {
		for (int i = 0; i < n; i++) {
			dest[i * 3 + 0] = vga_palette[start * 3 + i * 3 + 0];
			dest[i * 3 + 1] = vga_palette[start * 3 + i * 3 + 1];
			dest[i * 3 + 2] = vga_palette[start * 3 + i * 3 + 2];
		}
	}
}

void VGA_DumpPalette(const byte *src, int start, int n)
{
	//kprintf("%s(%lx,%ld,%ld)\n", __FUNCTION__, src, start, n);
	if (start == 0 && n == 256) {
		memcpy(vga_palette, src, 768);
	} else {
		for (int i = 0; i < n; i++) {
			vga_palette[start * 3 + i * 3 + 0] = src[i * 3 + 0];
			vga_palette[start * 3 + i * 3 + 1] = src[i * 3 + 1];
			vga_palette[start * 3 + i * 3 + 2] = src[i * 3 + 2];
		}
	}
	setpalette();
	updatePalette = TRUE;
}

void VGA_ZeroPalette(void)
{
	memset(vga_palette, 0, sizeof(vga_palette));
	setpalette();
	updatePalette = TRUE;
}

void VGA_FadeOutPalette(const byte *src, byte *dst, int n, byte r, byte g, byte b)
{//kprintf("%s(%lx, %lx, %ld, %lu, %lu, %lu)\n", __FUNCTION__, src, dst, n, r, g, b);
	byte c;
	word count;

	count = n;
	while (count--) {
		c = *src++;
		if (c < r) c++;
		else if (c > r) c--;
		*dst++ = c;

		c = *src++;
		if (c < g) c++;
		else if (c > b) c--;
		*dst++ = c;

		c = *src++;
		if (c < b) c++;
		else if (c > b) c--;
		*dst++ = c;
	}
}

void VGA_FadePalette(const byte *src, byte *dst, int n, const byte *target)
{//kprintf("%s(%lx, %lx, %ld, %lx)\n", __FUNCTION__);
	byte c, d;
	word count;

	count = n * 3;
	while (count--) {
		c = *src++;
		d = *target++;
		if (c < d) c++;
		else if (c > d) c--;
		*dst++ = c;
	}
}

void VGA_FullFadeOutPalette(byte *dst, int n, byte r, byte g, byte b, int pos)
{
	byte c;
	word count;
	byte ah;
	int diff;

	ah = 64-pos;

	count = n;
	while (count--) {
		c = *dst;
		diff = c - r;
		if (diff < 0 && -diff > ah) {
			*dst = r - ah;
		} else if (diff > ah) {
			*dst = r + ah;
		}
		dst++;

		c = *dst;
		diff = c - g;
		if (diff < 0 && -diff > ah) {
			*dst = g - ah;
		} else if (diff > ah) {
			*dst = g + ah;
		}
		dst++;

		c = *dst;
		diff = c - b;
		if (diff < 0 && -diff > ah) {
			*dst = b - ah;
		} else if (diff > ah) {
			*dst = b + ah;
		}
		dst++;
	}
}

void VGA_FullFadePalette(byte *dst, int n, const byte *target, int pos)
{
	//kprintf("%s(%lx, %ld, %lx, %ld)\n", __FUNCTION__, dst, n, target, pos);
	byte c, d;
	word count;
	byte ah;
	int diff;

	ah = 64-pos;

	count = n * 3;
	while (count--) {
		c = *dst;
		d = *target++;
		diff = c - d;
		if (diff < 0 && -diff > ah) {
			*dst = d - ah;
		} else if (diff > ah) {
			*dst = d + ah;
		}
		dst++;
	}
}

void VGA_VSync(void)
{//kprintf("%s()\n", __FUNCTION__);
	WaitTOF();
}

void VGA_ClearPage(word off, dword nbyt)
{
	//memset(&vga_memory[off << 2], 0, nbyt << 2);
	// TODO SetRast?
}
