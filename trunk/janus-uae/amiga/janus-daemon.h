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

#ifndef __JANUS_DAEMON_H_
#define __JANUS_DAEMON_H_

#define AROSTRAPBASE 0xF0FF90

#define REG(reg,arg) arg __asm(#reg)

#define AD__MAXMEM 256
#define AD_SHUTDOWN
#define AD_SETUP 10
#define AD_GET_JOB  11
#define AD_JOB_DONE 0
#define AD_GET_JOB_RESIZE 1
#define AD_GET_JOB_LIST_WINDOWS 2
#define AD_GET_JOB_MESSAGES 3
#define AD_GET_JOB_SYNC_WINDOWS 4
#define AD_GET_JOB_REPORT_UAE_WINDOWS 5
#define AD_GET_JOB_REPORT_HOST_WINDOWS 6
#define AD_GET_JOB_MARK_WINDOW_DEAD 7
#define AD_GET_JOB_GET_MOUSE 8
#define AD_GET_JOB_SWITCH_UAE_WINDOW 9
#define AD_GET_JOB_ACTIVE_WINDOW 10
#define AD_GET_JOB_NEW_WINDOW 11

extern ULONG (*calltrap)(ULONG __asm("d0"), 
                         ULONG __asm("d1"), 
			 APTR  __asm("a0"));

/* sync-mouse.c */
BOOL init_sync_mouse();
void free_sync_mouse();
void sync_mouse();

/* patch.c */
void patch_functions();
void unpatch_functions();

/* sync-windows.c */
BOOL init_sync_windows();
void update_windows();
void report_uae_windows();
void report_host_windows();
void sync_windows();
void sync_active_window();


#endif
