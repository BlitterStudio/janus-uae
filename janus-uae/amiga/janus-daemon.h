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

#include <stdio.h>
//#include <proto/intuition.h>
//#include <proto/exec.h>
#include <clib/intuition_protos.h>

//#define DEBUG 1
/* remember to enable m68k debugging in od-amiga/j-debug.c, too */
#if defined DEBUG
#define DebOut(...) PrintOut(__FILE__,__LINE__,__func__,__VA_ARGS__) 
#else
#define DebOut(...) 
#endif

#define AROSTRAPBASE 0xF0FF90

#define REG(reg,arg) arg __asm(#reg)

#define AD__MAXMEM 256

#define AD_SHUTDOWN 9
#define AD_SETUP 10
#define AD_GET_JOB  11
#define AD_CLIP_SETUP 15
#define AD_CLIP_JOB 16
#define AD_LAUNCH_SETUP    20
#define AD_LAUNCH_JOB      21

#define AD_TEST 0
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
#define AD_GET_JOB_LIST_SCREENS 12
#define AD_GET_JOB_OPEN_CUSTOM_SCREEN 13 /* not used ATM */
#define AD_GET_JOB_CLOSE_SCREEN       14 /* not used ATM */
#define AD_GET_JOB_TOP_SCREEN         15
#define AD_GET_JOB_SCREEN_DEPTH       16
#define AD_GET_JOB_MODIFY_IDCMP       17
#define AD_GET_JOB_UPDATE_GADGETS     18
#define AD_GET_JOB_SET_WINDOW_TITLES  19
#define AD_GET_JOB_WINDOW_LIMITS      20
#define AD_GET_JOB_SPLASH             21
#define AD_GET_JOB_HOST_DATA          22 /* resolution .. */
#define AD_GET_JOB_DEBUG             999

#define J_MSG_CLOSE                    1

extern ULONG state;

#ifndef __AROS__
extern ULONG (*calltrap)(ULONG __asm("d0"), 
                         ULONG __asm("d1"), 
			 APTR  __asm("a0"));
#else
ULONG calltrap(ULONG arg1, ULONG arg2, ULONG *arg3);
ULONG notify_signal;
struct MsgPort *notify_port;

void handle_notify_msg(ULONG notify_class, ULONG notify_object);

#endif


/* sync-mouse.c */
BOOL init_sync_mouse(void);
void free_sync_mouse(void);
void sync_mouse(void);
void SetMouse(struct Screen *screen, WORD x, WORD y, UWORD button, BOOL click, BOOL release);
BOOL is_cyber(struct Screen *screen);

/* patch.c */
#ifndef __AROS__
extern ULONG patch_draggable;
#endif
void patch_functions(void);
void unpatch_functions(void);

/* sync-windows.c */
BOOL init_sync_windows(void);
void update_windows(void);
BOOL init_sync_screens(void);
void update_screens(void);
void report_uae_windows(void);
void report_host_windows(void);
void sync_windows(void);
void sync_active_window(void);
void forward_messages(void);
void update_top_screen(void);

/* public_screen.c */
char *public_screen_name(struct Screen *scr); 

/* lock-window.c */

BOOL assert_window(struct Window *window);

/* debug.c */
void PrintOut(const char *file, unsigned int line, const char *func, const char *format, ...);

#define ENTER
#define LEAVE
#if 0
#define ENTER DebOut("janusd: %s:%s entered\n",__FILE__,__func__);
#define LEAVE DebOut("janusd: %s:%s left in line %d\n",__FILE__,__func__,__LINE__);
#endif

#if 0
#define C_ENTER DebOut("clipd: %s:%s entered\n",__FILE__,__func__);
#define C_LEAVE DebOut("clipd: %s:%s left in line %d\n",__FILE__,__func__,__LINE__);
#else
#define C_ENTER
#define C_LEAVE
#endif


#endif
