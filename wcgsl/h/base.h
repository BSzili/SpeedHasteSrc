// --------------------------- BASE.H ------------------------------
// For use with with WATCOM 9.5 + DOS4GW
// (C) Copyright 1993/4 by Jare & JCAB of Iguana-VangeliSTeam.

/*
 * Base definitions and expected things like SREGS, NULL, etc.
 */

#ifndef _BASE_H_
#define _BASE_H_

#ifdef __AMIGA__
#define __stdcall
#undef __interrupt
#define __interrupt
#undef __far
#define __far
#define far
#define __loadds
#define strcmpi strcasecmp
#else
#include <i86.h>
#endif
#include <stdlib.h>
#include <string.h>

#pragma library (local);


typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;
typedef   signed char  sbyte;
typedef   signed short sword;
typedef   signed long  sdword;

typedef unsigned char  uint8;
typedef   signed char  sint8;
typedef unsigned short uint16;
typedef   signed short sint16;
typedef unsigned long  uint32;
typedef   signed long  sint32;

typedef uint8 bool;

typedef signed   int   sint;
typedef unsigned int   uint;
typedef signed   long  slong;
typedef unsigned long  ulong;

#ifdef TRUE
#undef TRUE
#undef FALSE
#endif

enum {
    TRUE  = 1,
    FALSE = 0
};

#ifndef NULL
#define NULL (0L)
#endif

#define PUBLIC  extern
#define PRIVATE static

#define PUBLICFUNC __stdcall    // Defines a library function.
#define PUBLICDATA __stdcall    // Defines a library variable.

//#define PUBLICFUNC
#define LOCALFUNC

#define FAR

#ifndef swap
#define swap(a,b) ((a)^=(b),(b)^=(a),(a)^=(b))
#endif

#define swap16(w) ((w) = ((w) >> 8) | ((w) << 8))

#ifdef __AMIGA__
//#ifdef NDEBUG
#define outpw(_port, _data) do {} while(0)
#define outp(_port, _data) do {} while(0)
#define inp(_port) do {} while(0)
/*#else
#define outpw(_port, _data) kprintf("%s:%ld outpw(%lx,%lu)\n", __FUNCTION__, __LINE__, (_port), (_data))
#define outp(_port, _data) kprintf("%s:%ld outp(%lx,%lu)\n", __FUNCTION__, __LINE__, (_port), (_data))
#define inp(_port) kprintf("%s:%ld inp(%lx)\n", __FUNCTION__, __LINE__, (_port))
#endif*/
#else
extern unsigned inp(unsigned __port);
extern unsigned inpw(unsigned __port);
extern unsigned outp(unsigned __port, unsigned __value);
extern unsigned outpw(unsigned __port,unsigned __value);
#endif

#define inb(a)    inp(a)
#define outb(a,b) outp(a,b)
#define inw(a)    inpw(a)
#define outw(a,b) outpw(a,b)

#pragma intrinsic(inp,inpw,outp,outpw)

PUBLIC struct SREGS sregs;
PUBLIC union REGS inregs, outregs;

PUBLIC int    ArgC;
PUBLIC char **ArgV;

#define NEW(a)     ((void*)(((a)>0)?malloc((a)):NULL))
#define DISPOSE(a) (((a)!=NULL)?(void)(free((a)),(a)=NULL):(void)0)

PUBLIC void BASE_Require(const char *, const char *file, int line);
PUBLIC void BASE_Abort(const char *str, ...);

PUBLIC int BASE_CheckArg(const char *parm);

#ifdef __AMIGA__

#include <assert.h>
#define breakpoint()
#ifndef NDEBUG
#define REQUIRE assert
#else
#define REQUIRE(a) (a)
#endif

#else

#ifndef NDEBUG
void breakpoint(void);
#pragma aux breakpoint = "INT 3";
#define REQUIRE(a) ((a)?(void)0:(void)(breakpoint(),BASE_Require("\n" #a "\n", __FILE__, __LINE__)))
#define assert(a) REQUIRE(a)
#else
void breakpoint(void);
#pragma aux breakpoint = "INT 3";
#define REQUIRE(a) ((a)?(void)0:(void)(breakpoint(),BASE_Require("", __FILE__, __LINE__)))
//#define breakpoint()
#define assert(a)
#endif

#endif

#define SIZEARRAY(a) (sizeof(a)/sizeof(*(a)))

extern  sint32 FP0Div(sint32 a, sint32 b);
#define FP0Div(a,b) ((a)/(b))
extern  sint32 FP0Mult(sint32 a, sint32 b);
#define FP0Mult(a,b) ((a)*(b))

extern sint32 FP8Div(sint32 a, sint32 b);
#pragma aux   FP8Div modify nomemory [EDX] parm [EAX] [ECX] value [EAX] = \
                    "MOV    EDX,EAX" \
                    "SAR    EDX,31"  \
                    "SHLD EDX,EAX,8" \
                    "SHL  EAX,8"     \
                    "IDIV ECX"

extern sint32 FP8Mult(sint32 a, sint32 b);
#pragma aux   FP8Mult modify nomemory parm [EAX] [EDX] value [EAX] = \
                    "IMUL EDX"        \
                    "SHRD EAX,EDX,8"

#ifdef __AMIGA__
static inline sint32 FP16Div(sint32 a, sint32 b)
{
#if defined(__mc68060__)
	return ((sint32)(((double)(a) / (double)(b)) * 65536.0));
#else
	return ((sint32)(((int64_t)(a) << 16) / (b)));
#endif
}
#else
extern sint32 FP16Div(sint32 a, sint32 b);
#pragma aux   FP16Div modify nomemory [EDX] parm [EAX] [ECX] value [EAX] = \
                    "MOV    EDX,EAX" \
                    "SAR    EDX,31"  \
                    "SHLD EDX,EAX,16" \
                    "SHL  EAX,16"     \
                    "IDIV ECX"
#endif

#ifdef __AMIGA__
static inline sint32 FP16Mult(sint32 a, sint32 b)
{
#if defined(__mc68060__)
	return ((sint32)(((double)(a) * (double)(b)) / 65536.0));
#else
	return ((sint32)(((int64_t)(a) * (int64_t)(b)) >> 16));
#endif
}
#else
extern sint32 FP16Mult(sint32 a, sint32 b);
#pragma aux   FP16Mult modify nomemory parm [EAX] [EDX] value [EAX] = \
                    "IMUL EDX"        \
                    "SHRD EAX,EDX,16"
#endif

#ifdef __AMIGA__
static inline sint32 FP24Div(sint32 a, sint32 b)
{
#if defined(__mc68060__)
	return ((sint32)(((double)(a) / (double)(b)) * 16777216.0));
#else
	return ((sint32)(((int64_t)(a) << 24) / (b)));
#endif
}
#else
extern sint32 FP24Div(sint32 a, sint32 b);
#pragma aux   FP24Div modify nomemory [EDX] parm [EAX] [ECX] value [EAX] = \
                    "MOV    EDX,EAX" \
                    "SAR    EDX,31"  \
                    "SHLD EDX,EAX,24" \
                    "SHL  EAX,24"     \
                    "IDIV ECX"
#endif

#ifdef __AMIGA__
static inline sint32 FP24Mult(sint32 a, sint32 b)
{
#if defined(__mc68060__)
	return ((sint32)(((double)(a) * (double)(b)) / 16777216.0));
#else
	return ((sint32)(((int64_t)(a) * (int64_t)(b)) >> 24));
#endif
}
#else
extern sint32 FP24Mult(sint32 a, sint32 b);
#pragma aux   FP24Mult modify nomemory parm [EAX] [EDX] value [EAX] = \
                    "IMUL EDX"        \
                    "SHRD EAX,EDX,24"
#endif

#ifdef __AMIGA__
static inline sint32 FP32Div(sint32 a, sint32 b)
{
#if defined(__mc68060__)
	return ((sint32)(((double)(a) / (double)(b)) * 4294967296.0));
#else
	return ((sint32)(((int64_t)(a) << 32) / (b)));
#endif
}
#else
extern sint32 FP32Div(sint32 a, sint32 b);
#pragma aux   FP32Div modify nomemory [EAX] parm [EDX] [ECX] value [EAX] = \
                    "XOR  EAX,EAX"    \
                    "IDIV ECX"
#endif

#ifdef __AMIGA__
static inline sint32 FP32Mult(sint32 a, sint32 b)
{
#if defined(__mc68060__)
	return ((sint32)(((double)(a) * (double)(b)) / 4294967296.0));
#else
	return ((sint32)(((int64_t)(a) * (int64_t)(b)) >> 32));
#endif
}
#else
extern sint32 FP32Mult(sint32 a, sint32 b);
#pragma aux   FP32Mult modify nomemory parm [EAX] [EDX] value [EDX] = \
                    "IMUL EDX"
#endif

#ifdef __AMIGA__
static inline sint32 FPnDiv(sint32 a, sint32 b, uint32 r) // TODO add special cases
{
#if defined(__mc68060__)
	return ((sint32)(((double)(a) / (double)(b)) * (double)(1 << (r))));
#else
	return ((sint32)(((int64_t)(a) << (r)) / (b)));
#endif
}
#else
extern sint32 FPnDiv(sint32 a, sint32 b, uint32 r);
#pragma aux   FPnDiv modify nomemory [EDX] parm [EAX] [EBX] [ECX] value [EAX] = \
                    "MOV    EDX,EAX" \
                    "SAR    EDX,31"  \
                    "SHLD EDX,EAX,CL" \
                    "SHL  EAX,CL"     \
                    "IDIV EBX"
#endif

#ifdef __AMIGA__
static inline sint32 FP16Pow2(sint32 a) { return FP16Mult((a),(a)); }
#else
extern sint32 FP16Pow2(sint32 a);
#pragma aux   FP16Pow2 modify nomemory [EDX] parm [EAX] value [EAX] = \
                    "IMUL EAX"        \
                    "SHRD EAX,EDX,16"
#endif

extern sint32 FP24Pow2(sint32 a);
#pragma aux   FP24Pow2 modify nomemory [EDX] parm [EAX] value [EAX] = \
                    "IMUL EAX"        \
                    "SHRD EAX,EDX,24"

extern sint32 FP32Pow2(sint32 a);
#pragma aux   FP32Pow2 modify nomemory parm [EAX] value [EDX] = \
                    "IMUL EAX"

/*
#pragma aux   FPnRoundDiv modify nomemory [EDX] parm [EAX] [EBX] [ECX] value [EAX] = \
                    "CDQ"             \
                    "SHLD EDX,EAX,CL" \
                    "SHL  EAX,CL"     \
                    "MOV  ECX,EBX"    \
                    "SAR  ECX,1"      \
                    "ADD  EAX,ECX"    \
                    "ADC  EDX,0"      \
                    "IDIV EBX"
*/

#ifdef __AMIGA__
static inline sint32 FPnMult(sint32 a, sint32 b, uint32 r)
{
#if defined(__mc68060__)
	return ((sint32)(((double)(a) * (double)(b)) / (double)(1 << (r))));
#else
	return ((sint32)(((int64_t)(a) * (int64_t)(b)) >> (r)));
#endif
}
#else
extern sint32 FPnMult(sint32 a, sint32 b, uint32 r);
#pragma aux   FPnMult modify nomemory parm [EAX] [EDX] [ECX] value [EAX] = \
                    "IMUL EDX"        \
                    "SHRD EAX,EDX,CL"
#endif

#ifdef __AMIGA__
static inline sint32 FPMultDiv(sint32 a, sint32 b, sint32 c)
{
#if defined(__mc68060__)
	return ((sint32)(((double)(a) * (double)(b)) / (double)(c)));
#else
	return ((sint32)(((int64_t)(a) * (int64_t)(b)) / (c)));
#endif
}
#else
extern sint32 FPMultDiv(sint32 a, sint32 b, sint32 c);
#pragma aux   FPMultDiv modify nomemory parm [EAX] [EDX] [EBX] value [EAX] = \
                    "IMUL EDX"        \
                    "IDIV EBX"
#endif

extern sint32 FPUMultDiv(sint32 a, sint32 b, sint32 c);
#pragma aux   FPUMultDiv modify nomemory parm [EAX] [EDX] [EBX] value [EAX] = \
                    "MUL EDX"        \
                    "DIV EBX"

#ifdef __AMIGA__
static inline uint32 FP16Inverse(uint32 a) { return FP16Div(1 << 16, a); }
#else
extern uint32 FP16Inverse(uint32 a);     // 1/a  all 16.16
#pragma aux FP16Inverse modify nomemory [EDX] parm [ECX] value [EAX] = \
        "XOR    EAX,EAX"    \
        "MOV    EDX,1"      \
        "DIV    ECX"
#endif

extern void RepStosd(void *d, dword c, uint32 a);
#ifdef __AMIGA__
#define RepStosd(d, c, a)          \
    do {                            \
        uint32_t *dest = (uint32_t *)(d); \
        size_t count = (a);         \
        while (count--) {           \
            *dest++ = (c);          \
        }                           \
    } while (0)
#else
#pragma aux RepStosd parm [EDI] [EAX] [ECX] = \
        "CLD" \
        "REP STOSD"
#endif

extern void MemSetD(void *d, dword c, uint32 a);
#ifdef __AMIGA__
#define MemSetD(d, c, a)                       \
    do {                                         \
        uint32_t *dest32 = (uint32_t *)(d);      \
        size_t count32 = (a) >> 2;                \
        while (count32--) {                      \
            *dest32++ = (c);                     \
        }                                        \
        uint8_t *dest8 = (uint8_t *)dest32;      \
        size_t remaining = (a) & 3;              \
        while (remaining--) {                    \
            *dest8++ = (uint8_t)(c);             \
        }                                        \
    } while (0)
#else
#pragma aux MemSetD parm [EDI] [EAX] [ECX] = \
        "PUSH ECX"   \
        "SHR  ECX,2" \
        "REP STOSD"  \
        "POP  ECX"   \
        "AND  ECX,3" \
        "REP STOSB"
#endif

extern void MemSetW(void *d, dword c, uint32 a);
#pragma aux MemSetW parm [EDI] [EAX] [ECX] = \
        "SHR ECX,1"     \
        "REP STOSW"     \
        "ADC ECX,ECX"   \
        "REP STOSB"

extern void MemSetB(void *d, dword c, uint32 a);
#pragma aux MemSetB parm [EDI] [EAX] [ECX] = \
        "REP STOSB"

extern void RepMovsb(void *d, const void *c, uint32 a);
#ifdef __AMIGA__
#define RepMovsb(d, c, a) memcpy(d, c, a)
#else
#pragma aux RepMovsb parm [EDI] [ESI] [ECX] = \
        "CLD"        \
        "PUSH ECX"   \
        "SHR  ECX,2" \
        "REP MOVSD"  \
        "POP  ECX"   \
        "AND  ECX,3" \
        "REP MOVSB"
#endif

extern void RepMovsd(void *d, const void *c, uint32 a);
#pragma aux RepMovsd parm [EDI] [ESI] [ECX] = \
        "CLD"        \
        "REP MOVSD"

// ---------------------- Returns (a>0 ? 1 : -1) ------------------------
#ifdef __AMIGA__
static inline sint32 Sgn(sint32 a) { return a>0 ? 1 : -1; }
#else
sint32 Sgn(sint32 a);
#pragma aux Sgn modify nomemory          \
        parm caller [EAX]                \
        value [EAX] =                    \
        "SAR EAX,31"                     \
        "SHL EAX,1"                      \
        "INC EAX"
#endif

// ---------------------- Returns (a>0 ? a : -a) ------------------------
#ifdef __AMIGA__
static inline sint32 Abs32(sint32 a) { return a>0 ? a : -a; }
#else
sint32 Abs32(sint32 a);
#pragma aux Abs32 modify nomemory        \
        parm caller [EBX]                \
        value [EAX] =                    \
        "MOV EAX,EBX"                    \
        "ADD EBX,EBX"                    \
        "SBB EBX,EBX"                    \
        "XOR EAX,EBX"                    \
        "SUB EAX,EBX"
#endif

// ---------------------- Returns a^2
#ifdef __AMIGA__
static inline sint32 Pow2(sint32 a) { return a * a; }
#else
sint32 Pow2(sint32 a);
#pragma aux Pow2 modify nomemory         \
        parm caller [EDX]                \
        value [EAX] =                    \
        "MOV EAX,EDX"                    \
        "IMUL EAX"
#endif

#ifdef __AMIGA__
static inline sint32 Pow3(sint32 a) { return a * a * a; }
#else
sint32 Pow3(sint32 a);
#pragma aux Pow3 modify nomemory         \
        parm caller [EDX]                \
        value [EAX] =                    \
        "MOV EAX,EDX"                    \
        "IMUL EAX,EAX"                   \
        "IMUL EDX"
#endif

// ---------------------- Endian stuff

extern dword BSwapDword(dword a);
#ifdef __AMIGA__
#define BSwapDword(a) (uint32_t)(((uint32_t)(a) & 0xff) << 24 | ((uint32_t)(a) & 0xff00) << 8 | \
                      ((uint32_t)(a) & 0xff0000) >> 8 | ((uint32_t)(a) & 0xff000000) >> 24)
#else
#pragma aux  BSwapDword modify nomemory parm [EAX] value [EAX] = \
    "XCHG   AL,AH"  \
    "ROR    EAX,16" \
    "XCHG   AL,AH"
#endif

extern dword BSwapWord(dword a);
#ifdef __AMIGA__
#define BSwapWord(a) (uint16_t)(((uint16_t)(a) & 0xff) << 8 | ((uint16_t)(a) & 0xff00) >> 8)
#else
#pragma aux  BSwapWord modify nomemory parm [EAX] value [EAX] = \
    "XCHG   AL,AH"
#endif

// ---------------------- Get Segment values

#ifndef __AMIGA__
dword _DS(void);
#pragma aux _DS modify nomemory value [EAX] = \
    "XOR EAX,EAX"   \
    "MOV AX,DS"

dword _CS(void);
#pragma aux _CS modify nomemory value [EAX] = \
    "XOR EAX,EAX"   \
    "MOV AX,CS"

dword _ES(void);
#pragma aux _ES modify nomemory value [EAX] = \
    "XOR EAX,EAX"   \
    "MOV AX,ES"

dword _FS(void);
#pragma aux _FS modify nomemory value [EAX] = \
    "XOR EAX,EAX"   \
    "MOV AX,FS"

dword _GS(void);
#pragma aux _GS modify nomemory value [EAX] = \
    "XOR EAX,EAX"   \
    "MOV AX,GS"

void SetES(dword sel);
#pragma aux SetES modify nomemory parm [EAX] = \
    "MOV ES,AX"

void SetFS(dword sel);
#pragma aux SetFS modify nomemory parm [EAX] = \
    "MOV FS,AX"

void SetGS(dword sel);
#pragma aux SetGS modify nomemory parm [EAX] = \
    "MOV GS,AX"
#endif

// ----------------------------------

extern void FinishProgram(void);

// ----------------------------------

PUBLIC dword RND_Seed1, RND_Seed2, RND_Seed3;

#ifdef __AMIGA__
// used to initialize the random seed
#include <time.h>
#define BIOS_Clock time(NULL)
#else
#define BIOS_Clock (((dword *)0x46C)[0])
#endif

#ifdef __AMIGA__
static inline void RND_Randomize(dword seed) {
	RND_Seed1 = (seed);
	RND_Seed2 = (RND_Seed1 >> 13) | (RND_Seed1 << (32 - 13));
	RND_Seed3 = (RND_Seed2 >> 9) | (RND_Seed2 << (32 - 9));
}
#else
PUBLIC void RND_Randomize(dword seed);
#pragma aux RND_Randomize parm [EAX] = \
    "    MOV     [RND_Seed1],EAX"  \
    "    ROR     EAX,13         "  \
    "    MOV     [RND_Seed2],EAX"  \
    "    ROR     EAX,9          "  \
    "    MOV     [RND_Seed3],EAX"
#endif

#ifdef __AMIGA__
static inline dword RND_GetNum(void) {
    dword eax = RND_Seed1 + 0x0B35FA137;
    dword ebx = RND_Seed2 + 0x0354C63F7;
    dword edx = RND_Seed3 + 0x0067B784B;

    eax = (eax << 2) | (eax >> (32 - 2));
    RND_Seed1 = eax;
    ebx += eax;
    ebx = (ebx >> 1) | (ebx << (32 - 1));
    RND_Seed2 = ebx;
    edx -= ebx;
    eax ^= edx;
    RND_Seed3 = edx;
    eax += ebx;

    return eax;
}
#else
PUBLIC dword RND_GetNum(void);
#pragma aux RND_GetNum modify nomemory [EBX EDX] value [EAX] = \
    "    MOV     EAX,[RND_Seed1]" \
    "    MOV     EBX,[RND_Seed2]" \
    "    MOV     EDX,[RND_Seed3]" \
    "    ADD     EAX,0x0B35FA137" \
    "    ADD     EBX,0x0354C63f7" \
    "    ADD     EDX,0x0067B784B" \
    "    ROL     EAX,2          " \
    "    MOV     [RND_Seed1],EAX" \
    "    ADD     EBX,EAX        " \
    "    ROR     EBX,1          " \
    "    MOV     [RND_Seed2],EBX" \
    "    SUB     EDX,EBX        " \
    "    XOR     EAX,EDX        " \
    "    MOV     [RND_Seed3],EDX" \
    "    ADD     EAX,EBX        "
#endif

#ifdef __AMIGA__
#ifdef __mc68060__
#define LOOP(i, n) sint16 (i) = (n); while ((i)--)
#else
#define LOOP(i, n) for (sint16 (i) = 0; (i) < (n); (i)++)
#endif
#endif

#endif

// --------------------------- BASE.H ------------------------------
