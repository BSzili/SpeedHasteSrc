#include <llkey.h>

#include <proto/lowlevel.h>
#include <proto/dos.h>

#include <clib/debug_protos.h>
#include <SDI_compiler.h>

volatile byte LLK_Keys[256];
volatile byte LLK_NumKeys; // unused / LLK_kbhit -> LLK_PressAnyKey
volatile byte LLK_SpacePressed; // unused
volatile byte LLK_LastScan;
bool LLK_ChainChange; // unused
bool LLK_DoChain; // unused
bool LLK_Autorepeat; // TODO

//char __stdiowin[]="CON:////Circuit Racer/CLOSE/WAIT";
const char *__stdiowin = "CON:////Circuit Racer/CLOSE/WAIT";

static APTR intHandle;

static const byte KeyMappings[128] = {
	kAPOSTROPHE,
	k1,
	k2,
	k3,
	k4,
	k5,
	k6,
	k7,
	k8,
	k9,
	k0,
	kMINUS,
	kEQUAL,
	kBACKSLASH,
	0,
	kKEYPAD0,
	kQ,
	kW,
	kE,
	kR,
	kT,
	kY,
	kU,
	kI,
	kO,
	kP,
	kLBRACKET,
	kRBRACKET,
	0,
	kKEYPAD1,
	kKEYPAD2,
	kKEYPAD3,
	kA,
	kS,
	kD,
	kF,
	kG,
	kH,
	kJ,
	kK,
	kL,
	kSEMICOLON,
	kTICK,
	0, // international key next to Enter
	0,
	kKEYPAD4,
	kKEYPAD5,
	kKEYPAD6,
	0, // international key between the left Shift
	kZ,
	kX,
	kC,
	kV,
	kB,
	kN,
	kM,
	kCOMMA,
	kPERIOD,
	kSLASH,
	0,
	kKEYPADDEL,
	kKEYPAD7,
	kKEYPAD8,
	kKEYPAD9,
	kSPACE,
	kBACKSPACE,
	kTAB,
	kKEYPADENTER,
	kENTER,
	kESC,
	kDEL,
	0,
	0,
	0,
	kKEYPADMINUS,
	0,
	kUARROW,
	kDARROW,
	kRARROW,
	kLARROW,
	kF1,
	kF2,
	kF3,
	kF4,
	kF5,
	kF6,
	kF7,
	kF8,
	kF9,
	kF10,
	kNUMLOCK,
	kSCROLLLOCK,
	kKEYPADSLASH,
	kKEYPADSTAR,
	kKEYPADPLUS,
	kPAUSE, // Help key
	kLEFTSHIFT,
	kRIGHTSHIFT,
	kCAPSLOCK,
	kLEFTCTRL,
	kLEFTALT,
	kRIGHTALT,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

/*
	The routine is called from within an interrupt, so normal
	restrictions apply. The routine must preserve the following
	registers: A2, A3, A4, A7, D2-D7. Other registers are
	scratch, except for D0, which MUST BE SET TO 0 upon
	exit. On entry to the routine, A1 holds 'intData' and A5
	holds 'intRoutine', and D0 contains the rawkey code read
	from the keyboard.
*/

static ULONG LLK_NewInt9(REG(d0,ULONG key)) {
	//ULONG key = GetKey(); // TODO D0
	//UWORD qualifier = (key >> 16);
	UWORD code = (key & 0x7F);
	UWORD press = !(key & 0x80);
	byte scan = KeyMappings[code];
//kprintf("%s key %lx code %lx press %lx scan %lx\n", __FUNCTION__, key, code, press, scan);
	if (press) {
		LLK_LastScan = scan;
	}
	LLK_Keys[scan] = press;

	return 0;
}

void LLK_Init(void)
{
	//kprintf("%s\n", __FUNCTION__);
	SetMode(Input(), 1);
	intHandle = AddKBInt((APTR)LLK_NewInt9, NULL);
}

void LLK_End(void)
{
	//kprintf("%s\n", __FUNCTION__);
	if (intHandle != NULL) {
		RemKBInt(intHandle);
		intHandle = NULL;
	}
	SetMode(Input(), 0);
}
