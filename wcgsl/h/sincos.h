// --------------------------- SINCOS.H ------------------------------
// For use with Watcom C 9.0 and Tran's PMode.
// (C) Copyright 1993/4 by Jare & JCAB of Iguana-VangeliSTeam.
// Modified for use with WATCOM 9.5 + DOS4GW

/*
 * Init with InitSinCos() (should be a more regular name, yeah). Values
 * from Cos() and Sin() are:
 *      angle : 0-65536 circunference. 0 90 degrees (16384) points south.
 *              Mmmmhhh should be the other way, but who cares.
 *      return: between -1 and 1, in 2.30 fixed point format.
 *
 * FPMult() multiplies a number in 2.30 format by other number, and gives
 * the result in whichever format the other number is. Usually, you will
 * expect 5000 == FPMult(5000, Cos(0)).
 */

#ifndef _SINCOS_H_
#define _SINCOS_H_

#ifndef _BASE_H_
#include <base.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern long int _CosTbl[4096];

#ifdef __cplusplus
extern "C" {
#endif

extern long int Cos(word a);
extern long int Sin(word a);

#define Cos(a) (_CosTbl[((word)(a))>>4])
#define Sin(a) (Cos((a)+16384))

/*
#include <math.h>
#define Cos(a) ((sint32)(cos(((double)a)*3.1415926536/32768.0)*(double)(1 << 30)))
#define Sin(a) ((sint32)(-sin(((double)a)*3.1415926536/32768.0)*(double)(1 << 30)))
*/

#ifdef __AMIGA__
#if defined(__mc68060__)
static inline long int FPMult(long int edx, long int eax)
{
    long double ldtmp = (long double)eax * (long double)edx;
    eax = (ldtmp * (1.0 / 0x40000000));
    return eax;
}
#else
static inline long int FPMult(long int edx, long int eax)
{
    long long int lltmp = (long long int)eax * (long long int)edx;
    eax = (lltmp >> 30);
    return eax;
}
#endif
#else
extern long int FPMult(long int, long int);
#pragma aux FPMult modify nomemory parm caller [EDX] [EAX] value [EAX] = \
    "IMUL EDX" \
    "SHRD EAX,EDX,30"
#endif

#ifdef __AMIGA__
#if defined(__mc68060__)
static inline long int FPDiv(long int eax, long int ebx)
{
    long long int lltmp = (long long int)eax << 30;
    eax = lltmp / ebx;
    return eax;
}
#else
static inline long int FPDiv(long int eax, long int ebx)
{
    long double ldtmp = (long double)eax / (long double)ebx;
    eax = (ldtmp * 0x40000000);
    return eax;
}
#endif
#else
extern long int FPDiv(long int, long int);
#pragma aux FPDiv modify [EDX] nomemory parm caller [EAX] [EBX] value [EAX] = \
    "MOV EDX,EAX"   \
    "SAR EDX,2"     \
    "SHL EAX,30"    \
    "IDIV EBX"
#endif

// ---------------------- Returns a^2, being a in 2.30
#ifdef __AMIGA__
#define FPPow2(a) FPMult((a),(a))
#else
sint32 FPPow2(sint32 a);
#pragma aux FPPow2 modify nomemory [EDX] \
        parm caller [EAX]                \
        value [EAX] =                    \
        "IMUL EAX"                       \
        "SHRD EAX,EDX,30"
#endif

void InitSinCos(void);


#endif                                                                
// ----------------------- End of SINCOS.H ---------------------------
