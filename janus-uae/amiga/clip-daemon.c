/************************************************************************ 
 *
 * Clip-Daemon
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE
 *
 * Janus-UAE is free software: you can redistribute it and/or modify
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

/*
 * share the clipboard between aos3 and the host
 */

#include <stdio.h>
#include <stdlib.h>

#include <exec/devices.h>
#include <exec/interrupts.h>
#include <exec/nodes.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <proto/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/preferences.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <devices/timer.h>
#include <hardware/intbits.h>

#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/timer_protos.h>

#include <proto/dos.h>
#include <proto/timer.h>

#include "janus-daemon.h"
#include "clip-daemon.h"

char verstag[] = "\0$VER: clip-daemon 0.4";
LONG          *cmdbuffer=NULL;

BYTE         mysignal_bit;
ULONG        mysignal;
struct Task *mytask = NULL;

/*
 * d0 is the function to be called (AD_*)
 * d1 is the size of the memory supplied by a0
 * a0 memory, where to put out command in and 
 *    where we store the result
 */
ULONG (*calltrap)(ULONG __asm("d0"),
                  ULONG __asm("d1"),
		  APTR  __asm("a0")) = (APTR) AROSTRAPBASE;

BOOL open_libs() {
   if (!(IntuitionBase=(struct IntuitionBase *) OpenLibrary("intuition.library",39))) {
     printf("unable to open intuition.library\n");
     return FALSE;
   }
   return TRUE;
}

/****************************************************
 * register us!
 * as long as setup is not called, janus-uae behaves 
 * just like a normal uae
 *
 * if called with stop=0, we want to sleep
 *
 * if result is TRUE, we need to run (share clipboard)
 * if result is FALSE, we need to sleep 
 ****************************************************/
int setup(struct Task *task, ULONG signal, ULONG stop) {
  ULONG *command_mem;
  ULONG  state;

  DebOut("clipboard setup(%lx,%lx,%d)\n",(ULONG) task, signal, stop);

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);

  command_mem[0]=(ULONG) task;
  command_mem[4]=(ULONG) signal;
  command_mem[8]=(ULONG) stop;

  state = calltrap (AD_CLIP_SETUP, AD__MAXMEM, command_mem);

  state=command_mem[8];

  FreeVec(command_mem);

  DebOut("clipboard setup done(): result %d\n",(int) state);

  return state;
}

static void runme() {
  ULONG        newsignals;
  ULONG       *command_mem;;
  BOOL         done;
  BOOL         init;
  BOOL         set;

  DebOut("clipd running (CTRL-C to go to normal mode, CTRL-D to shared mode)..\n");

  done=FALSE;
  init=FALSE;
  while(!done) {
    newsignals=Wait(mysignal | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
    set=setup(mytask, mysignal, 0);
    if(set && (newsignals & mysignal)) {
      /* we are active */

      if(!init) {
	/* disabled -> enabled */
	init=TRUE;
#if 0
	update_screens(); /* report all open screens once, 
			   * updates again a every openwindow patch
			   * call
			   */
	DebOut("screens updated\n");

	update_windows(); /* report all open windows once,
			   * new windows will be handled by the patches
			   */
	DebOut("windows updated\n");
#endif
      }

#if 0
      sync_windows();
      report_uae_windows();
      report_host_windows();
      sync_mouse();
      sync_active_window();
      forward_messages();
#endif
    }

    if((newsignals & SIGBREAKF_CTRL_C) ||
      (!set && (newsignals & mysignal))) {
      DebOut("!set || got SIGBREAKF_CTRL_C..\n");
      if(init) {
	DebOut("tell uae, that we received a SIGBREAKF_CTRL_C\n");
	init=FALSE;
	/* cose all windows */
      	command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);
	calltrap (AD_GET_JOB, AD_GET_JOB_LIST_WINDOWS, command_mem); /* this is a NOOP!!!*/
	FreeVec(command_mem);
	setup(mytask, mysignal, 1); /* we are tired */
      }
      else {
	DebOut("we are already inactive\n");
      }
    }

    if(newsignals & SIGBREAKF_CTRL_D) {
      DebOut("got SIGBREAKF_CTRL_D..\n");
//      switch_uae_window();
      if(!init) {
	DebOut("tell uae, that we received a SIGBREAKF_CTRL_D\n");
	setup(mytask, mysignal, 2); /* we are back again */
      }
      else {
	DebOut("we are already active\n");
      }

    }
  }

  /* never arrive here */

  DebOut("try to sleep ..\n");

#if 0
  /* enable uae window again */
  activate_uae_window(1);

  /* cose all windows */
  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);
  calltrap (AD_GET_JOB, AD_GET_JOB_LIST_WINDOWS, command_mem);
  FreeVec(command_mem);
#endif

}


int main (int argc, char **argv) {

  DebOut("started\n");

  if(!open_libs()) {
    exit(1);
  }

#if 0
  if(!init_sync_mouse()) {
    DebOut("ERROR: init_sync_mouse failed\n");
    printf("ERROR: init_sync_mouse failed\n");
    exit(1);
  }

  init_sync_windows(); /* never fails */
  init_sync_screens(); /* never fails */
#endif

  /* try te get a signal */
  mysignal_bit=AllocSignal(-1);
  if(mysignal_bit == -1) {
    printf("no signal..!\n");
    DebOut("no signal..!\n");
    exit(1);
  }
  mysignal=1L << mysignal_bit;

  mytask=FindTask(NULL);
  DebOut("task: %lx\n",mytask);

  setup(mytask, mysignal, 0); /* init everything for the patches */

#if 0
  patch_functions();
#endif

  while(1) {
    runme(); /* as we patched the system, we will run (sleep) forever */
  }
}
