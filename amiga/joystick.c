// --------------------------- JOYSTICK.C ------------------------------
// For use with WATCOM 9.5 + DOS4GW
// (C) Copyright 1993/4 by Jare & JCAB of Iguana.

#include <joystick.h>

//#include <string.h>
#include <proto/lowlevel.h>


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
    if (LowLevelBase) {
		return 1;
	}
    return 0;
}

//void JOY_End(void);

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

		// Unused
		if (xb != NULL) *xb = 0;
		if (yb != NULL) *yb = 0;
		if (bb != NULL) *bb = 0;
	} else {
		if (xa != NULL) *xa = 0;
		if (ya != NULL) *ya = 0;
		if (xb != NULL) *xb = 0;
		if (yb != NULL) *yb = 0;
		if (ba != NULL) *ba = 0;
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
    if (xb != NULL) *xb = bx * wx;
    if (yb != NULL) *yb = by * wy;

    return rez;
}

// --------------------------- JOYSTICK.C ------------------------------
