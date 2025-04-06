// --------------------------- JOYSTICK.C ------------------------------
// For use with WATCOM 9.5 + DOS4GW
// (C) Copyright 1993/4 by Jare & JCAB of Iguana.

#include <joystick.h>

#define USE_PSXPORT
//#define GAMEPORT_TEST

//#include <string.h>
#include <stdlib.h>
#include <proto/lowlevel.h>

#ifdef USE_PSXPORT
#include <proto/exec.h>
#include <devices/gameport.h>
#include <psxport.h>

static struct MsgPort *gameport_mp = NULL;
static struct IOStdReq *gameport_io = NULL;
static BOOL gameport_is_open = FALSE;
static struct InputEvent gameport_ie;
static UWORD gameport_buttons = 0;
static BOOL analog_centered = FALSE;
static int analog_clx;
static int analog_cly;
static int analog_crx;
static int analog_cry;
static struct GamePortTrigger gameport_gpt = {
	GPTF_UPKEYS | GPTF_DOWNKEYS,	/* gpt_Keys */
	0,				/* gpt_Timeout */
	1,				/* gpt_XDelta */
	1				/* gpt_YDelta */
};

#define LOG_INFO printf
#define LOG_DEBUG(...)

static void psxport_init(void) {
	if ((gameport_mp = CreateMsgPort())) {
		if ((gameport_io = (struct IOStdReq *)CreateIORequest(gameport_mp, sizeof(struct IOStdReq)))) {
			int ix;
			BYTE gameport_ct;
			for (ix=0; ix<4; ix++) {
#ifdef GAMEPORT_TEST
				if (!OpenDevice((STRPTR)"gameport.device", ix, (struct IORequest *)gameport_io, 0)) {
#else
				if (!OpenDevice((STRPTR)"psxport.device", ix, (struct IORequest *)gameport_io, 0)) {
#endif
					Forbid();
					gameport_io->io_Command = GPD_ASKCTYPE;
					gameport_io->io_Length = 1;
					gameport_io->io_Data = &gameport_ct;
					DoIO((struct IORequest *)gameport_io);
					if (gameport_ct == GPCT_NOCONTROLLER) {
#ifdef GAMEPORT_TEST
						gameport_ct = GPCT_ABSJOYSTICK;
#else
						gameport_ct = GPCT_ALLOCATED;
#endif
						gameport_io->io_Command = GPD_SETCTYPE;
						gameport_io->io_Length = 1;
						gameport_io->io_Data = &gameport_ct;
						DoIO((struct IORequest *)gameport_io);

						Permit();

						gameport_io->io_Command = GPD_SETTRIGGER;
						gameport_io->io_Length = sizeof(struct GamePortTrigger);
						gameport_io->io_Data = &gameport_gpt;
						DoIO((struct IORequest *)gameport_io);

						gameport_io->io_Command = GPD_READEVENT;
						gameport_io->io_Length = sizeof(struct InputEvent);
						gameport_io->io_Data = &gameport_ie;
						SendIO((struct IORequest *)gameport_io);
						gameport_is_open = TRUE;

						LOG_INFO("unit %d opened", ix);

						break;
					} else {
						Permit();
						LOG_INFO("unit %d in use", ix);
						CloseDevice((struct IORequest *)gameport_io);
					}
				} else {
					LOG_DEBUG("unit %d won't open", ix);
				}
			}
		}
	}
}

static void psxport_uninit(void) {
	if (gameport_is_open) {
		AbortIO((struct IORequest *)gameport_io);
		WaitIO((struct IORequest *)gameport_io);
		BYTE gameport_ct = GPCT_NOCONTROLLER;
		gameport_io->io_Command = GPD_SETCTYPE;
		gameport_io->io_Length = 1;
		gameport_io->io_Data = &gameport_ct;
		DoIO((struct IORequest *)gameport_io);
		CloseDevice((struct IORequest *)gameport_io);
		gameport_is_open = FALSE;
	}
	if (gameport_io != NULL) {
		DeleteIORequest((struct IORequest *)gameport_io);
		gameport_io = NULL;
	}
	if (gameport_mp != NULL) {
		DeleteMsgPort(gameport_mp);
		gameport_mp = NULL;
	}
}

static void psxport_read(int *x, int *y, uint *b) {
	if (!gameport_is_open) {
		return;
	}

	if (GetMsg(gameport_mp) != NULL) {
#ifdef GAMEPORT_TEST
		if (x) *x = gameport_ie.ie_X * 127;
		if (y) *y = gameport_ie.ie_Y * 127;
		if (b) *b = !(gameport_ie.ie_Code & IECODE_UP_PREFIX);
		//kprintf("code %lx X %ld Y %ld\n", gameport_ie.ie_Code, gameport_ie.ie_X, gameport_ie.ie_Y);
#else
		if ((PSX_CLASS(gameport_ie) == PSX_CLASS_JOYPAD) || (PSX_CLASS(gameport_ie) == PSX_CLASS_WHEEL))
			analog_centered = FALSE;

		if ((PSX_CLASS(gameport_ie) == PSX_CLASS_ANALOG) || (PSX_CLASS(gameport_ie) == PSX_CLASS_ANALOG2) || (PSX_CLASS(gameport_ie) == PSX_CLASS_ANALOG_MODE2)) {
			int analog_lx = PSX_LEFTX(gameport_ie);
			int analog_ly = PSX_LEFTY(gameport_ie);
			int analog_rx = PSX_RIGHTX(gameport_ie);
			int analog_ry = PSX_RIGHTY(gameport_ie);

			if (!analog_centered) {
				analog_clx = analog_lx;
				analog_cly = analog_ly;
				analog_crx = analog_rx;
				analog_cry = analog_ry;
				analog_centered = TRUE;
			}

			// left analog stick
			if (x) *x = (analog_lx - analog_clx);
			//if (y) *y = (analog_ly - analog_cly);

			// right analog stick
			//if (x) *x = (analog_rx - analog_crx);
			if (y) *y = (analog_ry - analog_cry);
		}

		if (PSX_CLASS(gameport_ie) != PSX_CLASS_MOUSE) {
			UWORD gameport_curr = ~PSX_BUTTONS(gameport_ie);
			UWORD gameport_old = gameport_buttons;

#define BUTTON_CHANGED(button, ptr, on, off) if ((gameport_curr & (button)) || (gameport_old & (button)) != (gameport_curr & (button))) { if ((ptr)) *(ptr) = (gameport_curr & (button)) ? (on) : (off); }
			BUTTON_CHANGED(PSX_CROSS,  y, -128, 0);
			BUTTON_CHANGED(PSX_SQUARE, y,  127, 0);
			BUTTON_CHANGED(PSX_UP,     y, -128, 0);
			BUTTON_CHANGED(PSX_DOWN,   y,  127, 0);
			BUTTON_CHANGED(PSX_LEFT,   x, -128, 0);
			BUTTON_CHANGED(PSX_RIGHT,  x,  127, 0);
			BUTTON_CHANGED(PSX_L1,     b,    2, 0);
			BUTTON_CHANGED(PSX_R1,     b,    1, 0);
#undef BUTTON_CHANGED

			gameport_buttons = gameport_curr;
		}
#endif

		SendIO((struct IORequest *)gameport_io);
	}
}
#endif

// wcgsl detection, calibration, CURS_GetPosition
bool JOY_Read(int *xa, int *ya, int *xb, int *yb, uint *ba, uint *bb) {
    if (xa != NULL) *xa = 0;
    if (ya != NULL) *ya = 0;
    if (xb != NULL) *xb = 0;
    if (yb != NULL) *yb = 0;
    if (ba != NULL) *ba = 0;
    if (bb != NULL) *bb = 0;
            return FALSE;
}

/*
bool JOY_BIOSRead(int *xa, int *ya, int *xb, int *yb, uint *ba, uint *bb) {
    return FALSe;
}
*/

// -----------------------------
// High level functions.
// These perform averaging of values to avoid multitaskers' interference.

// game init
byte JOY_Init(bool bios, int nvalues) {
	atexit(JOY_End);
	psxport_init();
    if (LowLevelBase) {
		return 1;
	}
    return 0;
}

void JOY_End(void) {
	psxport_uninit();
}

// game calibration
bool JOY_Get(int *xa, int *ya, int *xb, int *yb, uint *ba, uint *bb) {
    int ax, ay, ab;
	ULONG portstate;

	portstate = ReadJoyPort(1);

	if (((portstate & JP_TYPE_MASK) == JP_TYPE_GAMECTLR) || ((portstate & JP_TYPE_MASK) == JP_TYPE_JOYSTK)) {
		// X
		if (portstate & JPF_JOY_LEFT) {
			ax = -1;
		} else if (portstate & JPF_JOY_RIGHT) {
			ax = 1;
		} else {
			ax = 0;
		}
		if (xa != NULL) *xa = ax;

		// Y
		if (portstate & JPF_JOY_UP) {
			ay = -1;
		} else if (portstate & JPF_JOY_DOWN) {
			ay = 1;
		} else {
			ay = 0;
		}
		if (ya != NULL) *ya = ay;

		// Button
		ab = 0;
		if (portstate & JPF_BUTTON_RED) {
			ab |= 1;
		}
		if (portstate & JPF_BUTTON_BLUE) {
			ab |= 2;
		}
		if (ba != NULL) *ba = ab;
	} else {
		if (xa != NULL) *xa = 0;
		if (ya != NULL) *ya = 0;
		if (ba != NULL) *ba = 0;
	}

	if (gameport_is_open) {
		psxport_read(xb, yb, bb);
	} else {
		if (xb != NULL) *xb = 0;
		if (yb != NULL) *yb = 0;
		if (bb != NULL) *bb = 0;
	}

    return 1;
}

// -----------------------
// Calibrated reading.

word JOY_CalAX[4] = {0, 1, 2, 3};
word JOY_CalAY[4] = {0, 1, 2, 3};
word JOY_CalBX[4] = {0, 1, 2, 3};
word JOY_CalBY[4] = {0, 1, 2, 3};

// game controls, menu
bool JOY_CalGet(int wx, int wy, int *xa, int *ya, int *xb, int *yb, uint *ba, uint *bb) {
    int ax, ay, bx, by;
    bool rez;

    rez = JOY_Get(&ax, &ay, &bx, &by, ba, bb);
    if (xa != NULL) *xa = ax * wx;
    if (ya != NULL) *ya = ay * wy;
    if (xb != NULL) *xb = (bx * wx) >> 7;
    if (yb != NULL) *yb = (by * wy) >> 7;

    return rez;
}

// --------------------------- JOYSTICK.C ------------------------------
