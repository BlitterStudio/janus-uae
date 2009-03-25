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

/*
 * first try to integrate UAE in AROS
 *
 * Copyright 2008 o1i
 *
 * we are unning as a high priority daemon to fetch the
 * commands the aos3 daemon runing on our host os sent
 * us and execute them. We also have to return the results.
 *
 * steps (TODO: outdated!!):
 *
 * 1. setup and register daemon in uae
 * 2. sleep until signal from uae (interrupt caused by uae)
 * 3. fetch command
 * 4. perform necessary syscalls etc.
 * 5. store results
 * 6. notify aos3 daemon, that we are done
 * 7. if another command is waiting, goto 3
 * 8. goto 2
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <proto/exec.h>
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

//#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>

#include "janus-daemon.h"

int __nocommandline = 0; /*???*/

char verstag[] = "\0$VER: janus-daemon 0.2";
#if 0
struct Window *old_MoveWindowInFront_Window,
              *old_MoveWindowInFront_BehindWindow;
ULONG          old_MoveWindowInFront_Counter;
#endif

LONG          *cmdbuffer=NULL;


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
}

/****************************************************
 * register us!
 * as long as setup is not called, janus-uae behaves 
 * just like a normal uae
 ****************************************************/
int setup(struct Task *task, ULONG signal) {
  ULONG  result;
  ULONG *command_mem;

  printf("setup(%lx,%lx)\n",task,signal);

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);

  printf("memory: %lx\n",command_mem);

  command_mem[0]=(ULONG) task;
  command_mem[4]=(ULONG) signal;

  result = calltrap (AD_SETUP, AD__MAXMEM, command_mem);

  FreeVec(command_mem);

  printf("setup done(): result %d\n",result);

  return result;
}

/*****************************************
 * check, if new IDCMP messages for 
 * amigaOS windows are waiting and send
 * them to the right message ports.
 *
 * NOT USED ATM!
 *****************************************/
void forward_messages() {
  ULONG *command_mem;
  UBYTE done;
  struct IntuiMessage *msg;

  //printf("forward_messages()\n");

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);

  done=0;
  while(!done) {

    calltrap (AD_GET_JOB, AD_GET_JOB_MESSAGES, command_mem);
    if(!command_mem[0]) {
      done=1;
      //printf("forward_messages: nothing to do..\n");
    }
    else {
      printf("got message, what to do now..?\n");
    }
  }

  FreeVec(command_mem);
}

/***************************************
 * enable/disable uae main window
 ***************************************/
void switch_uae_window() {

  if(!cmdbuffer) {
    cmdbuffer=AllocVec(16,MEMF_CLEAR);
  }

  if(cmdbuffer) {
    cmdbuffer[0]=0;
    calltrap (AD_GET_JOB, AD_GET_JOB_SWITCH_UAE_WINDOW, cmdbuffer);
  }
}

void activate_uae_window(int status) {

  if(!cmdbuffer) {
    cmdbuffer=AllocVec(16,MEMF_CLEAR);
  }

  if(cmdbuffer) {
    cmdbuffer[0]=1;
    cmdbuffer[1]=status;
    calltrap (AD_GET_JOB, AD_GET_JOB_SWITCH_UAE_WINDOW, cmdbuffer);
  }
}

/* error handling still sux (free resources TODO)*/

int main (int argc, char **argv) {

  struct Task *mytask = NULL;
  BYTE         mysignal_bit;
  ULONG        mysignal;
  ULONG        newsignals;
  ULONG        *command_mem;;
  BOOL         done;

  printf("aros-daemon...\n");

  if(!open_libs()) {
    exit(1);
  }

  if(!init_sync_mouse()) {
    printf("ERROR: init_sync_mouse failed\n");
    exit(1);
  }

  init_sync_windows(); /* never fails */

  /* try te get a signal */
  mysignal_bit=AllocSignal(-1);
  if(mysignal_bit == -1) {
    printf("no signal..!\n");
    exit(1);
  }
  mysignal=1L << mysignal_bit;

  mytask=FindTask(NULL);
  /* we want to be fast ?
   *SetTaskPri(mytask, 126);
   */

  setup(mytask,mysignal);

  patch_functions();
  update_windows(); /* report all open windows once,
                     * new windows wil be handled by the patches
		     */

  printf("running (CTRL-C to quit, CTRL-D to stop uae gfx update)..\n");

  done=FALSE;
  while(!done) {
    newsignals=Wait(mysignal | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
    if(newsignals & mysignal) {
      sync_windows();
      report_uae_windows();
      report_host_windows();
      sync_mouse();
      sync_active_window();
      //forward_messages();
    }
    if(newsignals & SIGBREAKF_CTRL_C) {
      printf("got SIGBREAKF_CTRL_C..\n");
      done=TRUE;
    }
    if(newsignals & SIGBREAKF_CTRL_D) {
      printf("got SIGBREAKF_CTRL_D..\n");
      switch_uae_window();
    }

  }

  /* enable uae window again */
  activate_uae_window(1);

  /* cose all windows */
  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);
  calltrap (AD_GET_JOB, AD_GET_JOB_LIST_WINDOWS, command_mem);
  FreeVec(command_mem);

  unpatch_functions();

  FreeSignal(mysignal_bit);

  free_sync_mouse();

  exit (0);
}
