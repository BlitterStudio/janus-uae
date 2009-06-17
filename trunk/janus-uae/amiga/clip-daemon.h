/************************************************************************ 
 *
 * Janus-Daemon
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-Daemon.
 *
 * Janus-Daemon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Janus-Daemon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Janus-Daemon. If not, see <http://www.gnu.org/licenses/>.
 *
 ************************************************************************/

#ifndef __CLIP_DAEMON_H_
#define __CLIP_DAEMON_H_

#include <stdio.h>
//#include <proto/intuition.h>
//#include <proto/exec.h>
#include <clib/intuition_protos.h>

#define AD_CLIP_SETUP 15
#define AD_CLIP_JOB 16
#define AD_CLIP_TEST 0
#define JD_AMIGA_CHANGED      1
#define JD_AROS_CHANGED       2
#define JD_CLIP_COPY_TO_AROS  3

#define AROSTRAPBASE 0xF0FF90

#define REG(reg,arg) arg __asm(#reg)

extern ULONG (*calltrap)(ULONG __asm("d0"), 
                         ULONG __asm("d1"), 
			 APTR  __asm("a0"));

/* debug.c */
void DebOut(const char *format, ...);

#define CLIPDEV_NAME "clipboard.device"

#endif
