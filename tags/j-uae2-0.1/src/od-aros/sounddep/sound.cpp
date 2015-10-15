/*
* UAE - The Un*x Amiga Emulator
*
* Win32 sound interface (DirectSound)
*
* Copyright 1997 Mathias Ortmann
* Copyright 1997-2001 Brian King
* Copyright 2000-2002 Bernd Roesch
* Copyright 2002-2003 Toni Wilen
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "memory.h"
#include "sound.h"

#define SND_MAX_BUFFER2 524288
#define SND_MAX_BUFFER 8192

uae_u16 *paula_sndbufpt;
int paula_sndbufsize;
uae_u16 paula_sndbuffer[SND_MAX_BUFFER];

int sounddrivermask;
