#include <proto/lowlevel.h>
#include <clib/debug_protos.h>

#include <timer.h>

volatile dword TIMER_Clock;
void (*TIMER_HookFunction)(void);
dword TIMER_Speed;

static APTR g_timerIntHandle = NULL;

static ULONG TimerISR(void) {
    if (TIMER_HookFunction != NULL)
        TIMER_HookFunction();
    TIMER_Clock++;
    return 0;
}

void TIMER_Init(dword speed) {
    if (g_timerIntHandle == NULL) {//kprintf("%s(%ld)\n", __FUNCTION__, speed);
        TIMER_HookFunction = NULL;
        TIMER_Clock = 0;
        TIMER_Speed = 0x10000;
        g_timerIntHandle = AddTimerInt((APTR)TimerISR, NULL);
        TIMER_SetSpeed(speed);
    }
}

void TIMER_SetSpeed(dword speed) {
    if (g_timerIntHandle != NULL) {
        TIMER_Speed = speed;
        StartTimerInt(g_timerIntHandle, (1000 * 1000) / (1192755 / speed), TRUE);
    }
}

void TIMER_End(void) {
    if (g_timerIntHandle != NULL) {//kprintf("%s()\n", __FUNCTION__);
        StopTimerInt(g_timerIntHandle);
        RemTimerInt(g_timerIntHandle);
        g_timerIntHandle = NULL;
    }
}
