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
 * $Id$
 *
 ************************************************************************/

#ifndef __LAUNCH_DAEMON_H_
#define __LAUNCH_DAEMON_H_

#define AD_LAUNCH_SETUP    20
#define AD_LAUNCH_JOB      21

#define LD_TEST             0
#define LD_GET_JOB          1

#define AROSTRAPBASE 0xF0FF90

/* debug.c */
void PrintOut(const char *file, unsigned int line, const char *func, const char *format, ...);

#define REG(reg,arg) arg __asm(#reg)

extern ULONG (*calltrap)(ULONG __asm("d0"), 
                         ULONG __asm("d1"), 
			 APTR  __asm("a0"));

#endif
