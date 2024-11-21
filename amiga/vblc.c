
#include <vbl.h>
#include <vga.h>
#include <string.h>
//#include <stdio.h>

#include <proto/lowlevel.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <clib/debug_protos.h>

    // Set this prior to using the module. By default it is set to
    // 0, meaning the VBL will have unrestricted access to the
    // retrace signal. If TRUE, the VBL will have to asume that it
    // can not sync correctly, and therefor revert to a flick-prone
    // pure timer mode and not wait for the retrace. The timer freq
    // will be at a rate of VBL_CompatibleMode frames, so set to 70
    // for 320x200-ratio modes and to 60 for 320x240-ratio modes.
    // BTW, remember that the timer cannot deliver less than 18.2 fps.
int VBL_CompatibleMode = 0;

/*
extern void VBL_InitializeA(int half);
#pragma aux VBL_InitializeA parm [ESI]
*/

volatile byte *VBL_Palette;
volatile byte *VBL_OldPal;
volatile word VBL_FirstColor = 256;
volatile word VBL_LastColor = 0;
//volatile dword VBL_PageOff = 0;
//volatile bool VBL_Active;
void (* volatile VBL_FullHandler)(void);

//volatile word ChangePageVal = 0;
//volatile byte ChangePageFlag = 0;
static struct Task *mainTask = NULL;
static UWORD waitFrames = 0;
#define VBL_SIGNAL SIGBREAKF_CTRL_F
#define VBL_COUNTER
#ifdef VBL_COUNTER
static ULONG VBLCounter = 0;
#endif
static int updatePalette = FALSE;


static int VBL_Handler(void)
{
	/*if (!VBL_Active)
		return 0;*/

#ifdef VBL_COUNTER
	VBLCounter++;
	if (waitFrames != 0 && VBLCounter >= waitFrames) {
		//kprintf("%s signaling after %ld frames\n", __FUNCTION__, waitFrames);
		waitFrames = 0;
		Signal(mainTask, VBL_SIGNAL);
	}
#else
	if (waitFrames > 0) {
		waitFrames--;
		if (waitFrames == 0) {
			Signal(mainTask, VBL_SIGNAL);
		}
	}
#endif

	if (VBL_FirstColor <= VBL_LastColor) {
		VGA_DumpPalette(VBL_Palette, VBL_FirstColor, VBL_FirstColor + VBL_LastColor + 1);
		VBL_FirstColor = 256;
		VBL_LastColor = 0;
		updatePalette = TRUE;
	}

	if (VBL_FullHandler) {
		VBL_FullHandler();
	}

	return 0;
}
//void VBL_TestHandler(void);

//extern HARD_TIRQHandler VBL_OldHandler;
static APTR intHandle = NULL;

bool VBL_Init(int nlines)
{
    if (intHandle != NULL)
        return TRUE;
//kprintf("%s(%ld)\n", __FUNCTION__, nlines);
    if (VBL_Palette == NULL) {
        VBL_Palette = NEW(768*2);
        if (VBL_Palette == NULL)
            return FALSE;
        memset(VBL_Palette, 0, 2*768);
        VBL_OldPal = VBL_Palette + 768;
    }

	mainTask = FindTask(NULL);

	intHandle = AddVBlankInt(VBL_Handler, NULL);
	if (intHandle == NULL) {
		// TODO free VBL_Palette
		return FALSE;
	}

    //VBL_Active = TRUE;

    VBL_FadePos = 0;

    return TRUE;
}

void VBL_Done(void)
{
    if (intHandle != NULL) {//kprintf("%s()\n", __FUNCTION__);
        RemVBlankInt(intHandle);
        intHandle = NULL;
    }
}

int VBL_VSync(int val)
{
	int ret = val;
	//kprintf("%s(%ld)\n", __FUNCTION__, val);
	if (val > 0) {
		SetSignal(0, VBL_SIGNAL);
		waitFrames = val;
		Wait(VBL_SIGNAL);
	}
#ifdef VBL_COUNTER
	VBLCounter = 0;
#endif

	if (updatePalette) {
		extern void flushpalette(void);
		flushpalette();
		updatePalette = FALSE;
	}

	return ret;
}

void VBL_ChangePage(int val)
{
	// TODO used by the intro
	//printf("%s(%d)\n", __FUNCTION__, val);
}

void VBL_DumpPalette(byte *src, int first, int size)
{
	byte *dest = VBL_Palette;
	for (int i = 0; i < size; i++)
	{
		dest[i * 3 + 0] = src[i * 3 + 0];
		dest[i * 3 + 1] = src[i * 3 + 1];
		dest[i * 3 + 2] = src[i * 3 + 2];
	}
	VBL_FirstColor = first;
	VBL_LastColor = first + size - 1;
}

void VBL_ZeroPalette(void)
{
	memset(VBL_Palette, 0, 768);
	VBL_FirstColor = 0;
	VBL_LastColor = 255;
}

volatile byte *VBL_DestPal = NULL;
volatile byte  VBL_DestRed = 0, VBL_DestGreen = 0, VBL_DestBlue = 0;
    // < 0 -> to rgb, == 0 -> nofade, > 0 -> to destpal.
volatile int   VBL_FadeSpeed = 0;
volatile int   VBL_FadeStartColor = 0, VBL_FadeNColors = 256;
volatile int   VBL_FadeMode = VBL_FADEFULL;
volatile int   VBL_FadePos  = 0;

void VBL_FadeHandler(void) {
    int i;

    if (VBL_FadePos > 0 && VBL_FadeSpeed > 0) {
        for (i = 0; i < VBL_FadeSpeed; i++) {
            if (VBL_FadePos > 64)
                VBL_FadePos = 64;
            if (VBL_FadeMode == VBL_FADEFAST) {
                if (VBL_DestPal != NULL)
                    VGA_FadePalette((byte*)VBL_Palette+3*VBL_FadeStartColor,
                                    (byte*)VBL_Palette+3*VBL_FadeStartColor,
                                    VBL_FadeNColors,
                                    (byte*)VBL_DestPal);
                else
                    VGA_FadeOutPalette((byte*)VBL_Palette+3*VBL_FadeStartColor,
                                       (byte*)VBL_Palette+3*VBL_FadeStartColor,
                                       VBL_FadeNColors,
                                       VBL_DestRed, VBL_DestGreen, VBL_DestBlue);
            } else {
                if (VBL_DestPal != NULL)
                    VGA_FullFadePalette((byte*)VBL_Palette+3*VBL_FadeStartColor,
                                        VBL_FadeNColors,
                                        (byte*)VBL_DestPal,
                                        VBL_FadePos);
                else
                    VGA_FullFadeOutPalette((byte*)VBL_Palette+3*VBL_FadeStartColor,
                                           VBL_FadeNColors,
                                           VBL_DestRed, VBL_DestGreen, VBL_DestBlue,
                                           VBL_FadePos);
            }
            VBL_FadePos++;
            if (VBL_FadePos > 64) {
                VBL_FadePos = 0;
                break;
            }
        }
        VBL_FirstColor = VBL_FadeStartColor;
        VBL_LastColor  = VBL_FadeStartColor + VBL_FadeNColors - 1;
    }
}

void VBL_RestoreSystemTime(void) {
}