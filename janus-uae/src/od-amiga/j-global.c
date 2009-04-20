/************************************************************************ 
 *
 * j-global.c
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE.
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
 * along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
 *
 ************************************************************************/

#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>

#include "j.h"
#include "threaddep/thread.h"

/* we need that, to wake up the aos3 high pri daemon */
ULONG aos3_task=0;
ULONG aos3_task_signal=0;

/* access the JanusWin list */
struct SignalSemaphore sem_janus_window_list;

/* access the JanusScreen list */
struct SignalSemaphore sem_janus_screen_list;

/* wait until thread is allowed to start */
struct SignalSemaphore aos3_thread_start;

/* wait until thread is allowed to start */
struct SignalSemaphore sem_janus_active_win;

/* protect aos3_messages list */
struct SignalSemaphore janus_messages_access;

GSList *janus_windows =NULL;
GSList *janus_screens =NULL;
GSList *janus_messages=NULL;

JanusWin *janus_active_window=NULL;

BOOL uae_main_window_closed=FALSE;

/* if you set this to true, jwindow contents won't get updated any more */
BOOL j_stop_window_update=FALSE;

WORD menux=0, menuy=0; /* fake mouse coords for menu selection */
