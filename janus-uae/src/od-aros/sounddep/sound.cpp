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

uae_u16 *paula_sndbufpt;
