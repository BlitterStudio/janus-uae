#include <exec/types.h>
#include "UAE-Control.h"

#ifdef SASC
/* SAS/C */
static int (*calltrap)(...) = (int (*)(...))0xF0FF60;
#define CAST
#else
/* GCC */
static int (*calltrap)(ULONG __asm("a0"),
                  ULONG __asm("a1"),
                  ULONG __asm("a2")) = (APTR) 0xF0FF60;
#define CAST (ULONG)
#endif

int GetVersion(void)
{
    return calltrap (0, 0, 0);
}
int GetUaeConfig(struct UAE_CONFIG *a)
{
    return calltrap (1, CAST a, 0);
}
int SetUaeConfig(struct UAE_CONFIG *a)
{
    return calltrap (2, CAST a, 0);
}
int HardReset(void)
{
    return calltrap (3, 0, 0);
}
int Reset(void)
{
    return calltrap (4, 0, 0);
}
int EjectDisk(ULONG drive)
{
    return calltrap (5, CAST "", drive);
}
int InsertDisk(UBYTE *name, ULONG drive)
{
    return calltrap (5, CAST name, drive);
}
int EnableSound(void)
{
    return calltrap (6, 2, 0);
}
int DisableSound(void)
{
    return calltrap (6, 1, 0);
}
int EnableJoystick(void)
{
    return calltrap (7, 1, 0);
}
int DisableJoystick(void)
{
    return calltrap (7, 0, 0);
}
int SetFrameRate(ULONG rate)
{
    return calltrap (8, rate, 0);
}
int ChgCMemSize(ULONG mem)
{
    return calltrap (9, mem, 0);
}
int ChgSMemSize(ULONG mem)
{
    return calltrap (10, mem, 0);
}
int ChgFMemSize(ULONG mem)
{
    return calltrap (11, mem, 0);
}
int ChangeLanguage(ULONG lang)
{
    return calltrap (12, lang, 0);
}
int ExitEmu(void)
{
    return calltrap (13, 0, 0);
}
int GetDisk(ULONG drive, UBYTE *name)
{
    return calltrap (14, CAST drive, CAST name);
}
int DebugFunc(void)
{
    return calltrap (15, 0, 0);
}
