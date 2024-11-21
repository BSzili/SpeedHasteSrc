#include <llipx.h>

    // Returns ipxentry. if zero, ipx is not installed
dword IPX_Init(void) {
        return 0;
}

void IPX_End(void) {
}

IPX_PAddress IPX_GetAddress(IPX_PAddress b) {
        return NULL;
}

word IPX_GetMaxPacketSize(void) {
        return 0;
}

    // first argument:
    //      socket longevity flag
    //          00h the socket will remain open until a Close Socket call or the
    //              application terminates
    //          01h the socket will remain open until a Close Socket call
    // second argument:
    //      address to the socket value (little-endian word)
    // returns:
    //      >= 0 Success
    //          the socket value will reflect the open socket (little-endian word)
    //      -1 Socket Already Open
    //      -2 Socket Table Is Full
int IPX_OpenSocket(byte longevity, word *sock) {
        return -3;
}

void IPX_CloseSocket(word s) {
}

    // returns >= 0 Success, -1 -> listening socket does not exist
int IPX_ListenForPacket(IPX_Tecb *ecb) {
        return -2;
}

void IPX_SendPacket(IPX_Tecb *ecb) {
}

void IPX_RelinquishControl(void) {
}

// ------------------------------------
// Helper functions for allocating ECBs from DOS real mode memory.

IPX_Tecb *IPX_NewECB(int n, int extra) {
        return NULL;
}

void IPX_FreeECB(IPX_Tecb *ecb) {
}

// ----------------------------------------------
// Higher level stuff.

IPX_PPacket IPX_AllocPackets(int n, int bufsize) {
        return NULL;
}

void IPX_FreePackets(IPX_PPacket p) {
}

    // Fill up appropiate fields. If src == NULL will use own memory area.
void IPX_InitOutPacket(IPX_PPacket p,
                       IPX_PAddress src, IPX_PAddress dest, int sock) {
}

void IPX_InitInPacket(IPX_PPacket p, int sock) {
}


// ----------------------------- LLIPX.C ---------------------------

word IPX_FlipWord(word a) { return a; }
