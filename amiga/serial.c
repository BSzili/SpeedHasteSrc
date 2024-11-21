#include <serial.h>

#include <clib/debug_protos.h>

void SER_InitComInfo(SER_PComPort p, int com, int port, int irq) {
}

bool SER_InitComPort(SER_PComPort p) {
        return FALSE;
}

void SER_InitComInterrupts(SER_PComPort p, void __interrupt (__far *isr)(void)) {
}

void SER_EndComPort(SER_PComPort p) {
}

// ------------------------------------------------------
// --------------- Cool stream-based functions.

    // Init structure, hook to interrupts and setup port interrupts.
void SER_InitStream(SER_PStream s, void __interrupt (__far *isr)(void)) {
}

    // Add byte to stream buffer.
void SER_WriteStreamChar(SER_PStream s, byte c) {
}

    // Get byte from stream buffer.
int  SER_ReadStreamChar(SER_PStream s) {
    return 0;
}

    // Unget a read stream char.
void SER_UngetStreamChar(SER_PStream s, int c) {
}

void SER_WriteStream(SER_PStream s, const byte *c, int len) {
}

int SER_ReadStream(SER_PStream s, byte *c, int max) {
    return 0;
}

// ----------- Block functions.

void SER_InitBlockRead(SER_PStream s) {
}

void SER_WriteBlock(SER_PStream s, byte *c, int len) {
}

int SER_ReadBlock(SER_PStream s, byte *c, int max) {
    return 0;
}

    // Start transmission of buffer in stream.
void SER_StreamTransmit(SER_PStream s) {
}

    // Most of the ISR is here.
void SER_StreamHandle(SER_PStream s) {
}

// -------------------------------------
// Modem stuff. Establish connection thru a phone line.

    // -1 if ESC, else SERMS_MATCH if 'data' received, else SERMS_xxxx
int SER_WaitModem(SER_PStream s, const char *data) {
    return -1;
}

    // Should return SERMS_OK
int SER_InitModem(SER_PStream s, SER_PModemCfg cfg) {
    return -1;
}

    // Should return SERMS_CONNECT
int SER_Dial(SER_PStream s, SER_PModemCfg cfg, const char *number) {
    return -1;
}

    // Should return SERMS_CONNECT
int SER_Answer(SER_PStream s, SER_PModemCfg cfg) {
    return -1;
}

    // Should return SERMS_OK
int SER_Hangup(SER_PStream s, SER_PModemCfg cfg) {
    return -1;
}

// ------------------------------------------------------
// --------------- Scrap stuff to ease use.

SER_TStream SER_ScrapStream;

void __interrupt __far __loadds SER_ScrapISR(void) {
    SER_StreamHandle(&SER_ScrapStream);
}

// --------------------------- SERIAL.C ------------------------------
