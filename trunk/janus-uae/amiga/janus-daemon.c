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

/*
 * first try to integrate UAE in AROS
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <exec/devices.h>
#include <exec/interrupts.h>
#include <exec/nodes.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
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

int __nocommandline = 0; /*???*/

char verstag[] = "\0$VER: janus-daemon 0.4";
#if 0
struct Window *old_MoveWindowInFront_Window,
              *old_MoveWindowInFront_BehindWindow;
ULONG          old_MoveWindowInFront_Counter;
#endif

LONG          *cmdbuffer=NULL;

/* state:
 *  0 not yet initialized / sleep until we go to coherency
 *  2 play the game
 */
ULONG state=0;

/* enable/disable screen dragging in amigaOS */
ULONG patch_draggable=0;

BYTE         mysignal_bit;
ULONG        mysignal;
struct Task *mytask = NULL;

struct Library *CyberGfxBase;

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
  ENTER
   if (!(IntuitionBase=(struct IntuitionBase *) OpenLibrary("intuition.library",39))) {
     printf("unable to open intuition.library\n");
     LEAVE
     return FALSE;
   }
   if (!(UtilityBase=(struct UtilityBase *) OpenLibrary("utility.library",39))) {
     printf("unable to open utility.library\n");
     LEAVE
     return FALSE;
   }

   CyberGfxBase=OpenLibrary("cybergraphics.library", 0);
   DebOut("CyberGfxBase: %lx\n",CyberGfxBase);

   LEAVE
   return TRUE;
}

/****************************************************
 * register us!
 * as long as setup is not called, janus-uae behaves 
 * just like a normal uae
 *
 * if called with stop=0, we want to sleep
 *
 * if result is TRUE, we need to run (coherent mode)
 * if result is FALSE, we need to sleep (classic)
 ****************************************************/
int setup(struct Task *task, ULONG signal, ULONG stop) {
  ULONG *command_mem;

  ENTER

  DebOut("janusd: setup(%lx,%lx,%d)\n",(ULONG) task, signal, stop);

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);

  //DebOut("janusd: memory: %lx\n",(ULONG) command_mem);

  command_mem[0]=(ULONG) task;
  command_mem[4]=(ULONG) signal;
  command_mem[8]=(ULONG) stop;

  state = calltrap (AD_SETUP, AD__MAXMEM, command_mem);

  state=command_mem[8];

  FreeVec(command_mem);

  DebOut("janusd: setup done(): result %d\n",(int) state);

  LEAVE

  return state;
}

/***************************************
 * enable/disable uae main window
 ***************************************/
void switch_uae_window() {

  ENTER;

  DebOut("janusd: switch_uae_window entered");

  if(!cmdbuffer) {
    cmdbuffer=AllocVec(16,MEMF_CLEAR);
  }

  if(cmdbuffer) {
    cmdbuffer[0]=0;
    calltrap (AD_GET_JOB, AD_GET_JOB_SWITCH_UAE_WINDOW, cmdbuffer);
  }

  LEAVE
}

void activate_uae_window(int status) {

  ENTER

  if(!cmdbuffer) {
    cmdbuffer=AllocVec(16,MEMF_CLEAR);
  }

  if(cmdbuffer) {
    cmdbuffer[0]=1;
    cmdbuffer[1]=status;
    calltrap (AD_GET_JOB, AD_GET_JOB_SWITCH_UAE_WINDOW, cmdbuffer);
  }

  LEAVE
}

static void runme() {
  ULONG        newsignals;
  ULONG       *command_mem;;
  BOOL         done;
  BOOL         init;
  BOOL         set;

  #if 0
  while(!setup(mytask, mysignal, 0)) {
    DebOut("janusd: let's sleep ..\n");
    sleep(10); /* somebody will wake us up anyways */
  }
#endif

#if 0
  /*============*/
  ULONG *commandmem;
  ULONG modeID;
  struct Screen *screen;
  WORD x,y;

  DebOut("FOO: startme\n");


done=FALSE;
  while(!done) {
    DebOut("FOO: calltrap..\n");
    commandmem=AllocVec(AD__MAXMEM,MEMF_CLEAR);
    calltrap (AD_GET_JOB, AD_TEST, commandmem);

    DebOut("FOO: janus-daemon:      %3d,%3d\n", commandmem[0], commandmem[1]);
    DebOut("FOO: amigaos screen dx  %3d\n",IntuitionBase->FirstScreen->ViewPort.RasInfo->RxOffset);
    screen=IntuitionBase->FirstScreen;
    if(screen) {
      modeID=screen->ViewPort.Modes;

      x=(WORD) commandmem[0];
      y=(WORD) commandmem[1];

      if(modeID & SUPERHIRES) {
	x=x*2;
	DebOut("==>SUPER_KEY\n");
      }
      else if(modeID & HIRES) {
	DebOut("==>HIRES_KEY\n");
      }
      else { 
	DebOut("==>LORES_KEY\n");
	x=x/2;
	y=y/2;
      }

      SetMouse(screen, x, y, 0);
    }
    FreeVec(commandmem);

    sleep(1);
  }

  /*============*/
#endif
  DebOut("janusd: runme entered\n");

  ENTER

  DebOut("janusd: now we want to do something usefull ..\n");

  DebOut("janusd: running (CTRL-C to go to normal mode, CTRL-D to rootless mode)..\n");

  done=FALSE;
  init=FALSE;
  while(!done) {
    DebOut("janusd: Wait() ..\n");
    newsignals=Wait(mysignal | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
    set=setup(mytask, mysignal, 0);
    if(newsignals & mysignal) {
      if(set) {
	/* we are active */

	if(!init) {
	  /* disabled -> enabled */
	  init=TRUE;
	  update_screens(); /* report all open screens once, 
			     * updates again a every openwindow patch
			     * call
			     */
	  DebOut("janusd: screens updated\n");

	  update_windows(); /* report all open windows once,
			     * new windows will be handled by the patches
			     */
	  DebOut("janusd: windows updated\n");
	}

	update_top_screen();
	sync_windows();
	report_uae_windows();
	report_host_windows();
	sync_active_window();
	forward_messages();
      }
      sync_mouse();
    }

    if((newsignals & SIGBREAKF_CTRL_C) ||
      (!set && (newsignals & mysignal))) {
      DebOut("janusd: !set || got SIGBREAKF_CTRL_C..\n");
      if(init) {
	DebOut("janusd: tell uae, that we received a SIGBREAKF_CTRL_C\n");
	init=FALSE;
	/* cose all windows */
      	command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);
	calltrap (AD_GET_JOB, AD_GET_JOB_LIST_WINDOWS, command_mem); /* this is a NOOP!!!*/
	FreeVec(command_mem);
	setup(mytask, mysignal, 1); /* we are tired */
      }
      else {
	DebOut("janusd: we are already inactive\n");
      }
    }

    if(newsignals & SIGBREAKF_CTRL_D) {
      DebOut("janusd: got SIGBREAKF_CTRL_D..\n");
//      switch_uae_window();
      if(!init) {
	DebOut("janusd: tell uae, that we received a SIGBREAKF_CTRL_D\n");
	setup(mytask, mysignal, 2); /* we are back again */
      }
      else {
	DebOut("janusd: we are already active\n");
      }

    }
  }

  /* never arrive here */


  DebOut("janusd: try to sleep ..\n");

  /* enable uae window again */
  activate_uae_window(1);

  /* cose all windows */
  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);
  calltrap (AD_GET_JOB, AD_GET_JOB_LIST_WINDOWS, command_mem);
  FreeVec(command_mem);

  LEAVE
}


/* error handling still sux (free resources TODO)*/

int main (int argc, char **argv) {

  ENTER
  DebOut("janusd: started (%d)\n",argc);

  if(!open_libs()) {
    exit(1);
  }

  if(!init_sync_mouse()) {
    DebOut("janusd: ERROR: init_sync_mouse failed\n");
    printf("ERROR: init_sync_mouse failed\n");
    LEAVE
    exit(1);
  }

  init_sync_windows(); /* never fails */
  init_sync_screens(); /* never fails */

  /* try te get a signal */
  mysignal_bit=AllocSignal(-1);
  if(mysignal_bit == -1) {
    printf("no signal..!\n");
    DebOut("janusd: no signal..!\n");
    LEAVE
    exit(1);
  }
  mysignal=1L << mysignal_bit;

  mytask=FindTask(NULL);
  DebOut("janusd: task: %lx\n",mytask);

  SetTaskPri(mytask, 55);

  setup(mytask, mysignal, 0); /* init everything for the patches */

  patch_functions();

  while(1) {
    runme(); /* as we patched the system, we will run (sleep) forever */
  }

  /* can't be reached.. */
  unpatch_functions();
  FreeSignal(mysignal_bit);
  free_sync_mouse();

  DebOut("janusd: exit\n");

  LEAVE

  exit (0);
}
