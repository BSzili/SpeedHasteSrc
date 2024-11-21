#include <dpmi.h>
#include <clib/debug_protos.h>

word PUBLICFUNC DPMI_NewSelector(void)
{
    return 1;
}

void PUBLICFUNC DPMI_FreeSelector(word sel)
{
}

void PUBLICFUNC DPMI_SetBaseAddress(word sel, dword linbase)
{
}

void PUBLICFUNC DPMI_SetLimit(word sel, dword limit)
{
}
