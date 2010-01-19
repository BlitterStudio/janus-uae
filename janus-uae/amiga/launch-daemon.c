/************************************************************************ 
 *
 * Launch-Daemon
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
 * start ao3 workbench executables from aros wanderer
 */

#include <stdio.h>
#include <stdlib.h>

#include <devices/clipboard.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <exec/nodes.h>
#include <exec/io.h>
#include <exec/memory.h>

#if 0
#include <intuition/intuitionbase.h>
#include <intuition/preferences.h>
#include <hardware/intbits.h>
#endif

#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#if 0
#include <clib/timer_protos.h>

#include <proto/dos.h>
#include <proto/timer.h>
#endif

#include <libraries/wbstart.h>
#include <inline/wbstart.h>

#include "janus-daemon.h"
#include "launch-daemon.h"

struct Library *WBStartBase;

char verstag[] = "\0$VER: launch-daemon 0.1";
LONG          *cmdbuffer=NULL;

/* signal sent by j-uae to tell us, we have to do something! */
BYTE         launch_signal_bit;
ULONG        launchsignal;

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

   if (!(WBStartBase=OpenLibrary("wbstart.library",2))) {
     printf("unable to open wbstart.library! Get it from aminet.\n");
     return FALSE;
   }

   return TRUE;
}

/****************************************************
 * register us!
 *
 * as long as setup is not called, janus-uae behaves 
 * just like a normal uae
 *
 * if called with stop=0, we want to sleep
 *
 * if result is TRUE, we need to run 
 * if result is FALSE, we need to sleep 
 ****************************************************/
int setup(struct Task *task, ULONG launch_signal, ULONG stop) {
  ULONG *command_mem;
  ULONG  state;

  C_ENTER

  DebOut("launchd: launcher setup(%lx,%lx,%lx,%d)\n",(ULONG) task, launch_signal, stop);

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);

  command_mem[ 0]=(ULONG) task;
  command_mem[ 4]=(ULONG) launch_signal;
  command_mem[ 8]=(ULONG) 0;
  command_mem[12]=(ULONG) stop;

  state = calltrap (AD_LAUNCH_SETUP, AD__MAXMEM, command_mem);

  state=command_mem[12];

  FreeVec(command_mem);

  DebOut("launchd: launcher setup done(): result %d\n",(int) state);

  C_LEAVE

  return state;
}

static LONG start_it(char *path, char *filename) {
  LONG rc;

  DebOut("launchd: start_it(%s, %s)\n", path, filename);

  rc = WBStartTags(WBStart_DirectoryName, (ULONG) path,
                   WBStart_Name,          (ULONG) filename,
		   TAG_DONE);

  if(rc != RETURN_OK) {
    DebOut("launchd: unable to start it: rc=%d\n", rc);
  }
  return rc;
}


static void handle_launch_signal(void) {
  ULONG *command_mem;
  char  *command_string;
  char  *path;
  char  *filename;

  C_ENTER

  DebOut("launchd: handle_launch_signal()\n");

  command_mem=AllocVec(4096, MEMF_CLEAR);
  if(!command_mem) {
    C_LEAVE
    return;
  }

  calltrap (AD_LAUNCH_JOB, LD_GET_JOB, command_mem);

  if(command_mem[0]==0) {
    DebOut("launchd: no commands waiting for us!?\n");
  }

  command_string=(char *) command_mem;
  path          =command_string + command_mem[1];
  filename      =command_string + command_mem[2];

  start_it(path, filename);

  C_LEAVE
}

static void runme(void) {
  ULONG        newsignals;
  BOOL         done;
  BOOL         init;
  BOOL         set=TRUE;

  C_ENTER
  DebOut("launchd: launchd running (CTRL-C to go to normal mode, CTRL-D to disabled mode)..\n");

  setup(mytask, launchsignal, 0);

  done=FALSE;
  init=FALSE;
  while(!done) {
    newsignals=Wait(launchsignal | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);

    /* test if we are still active */
    //TODO set=setup(mytask, to_aros_signal, to_amigaos_signal, 0);
    DebOut("launchd: set %d, signal %d\n", set, newsignals);

    if(set) {

      if(newsignals & launchsignal) {
	DebOut("launchd: launchsignal from UAE received\n");
	handle_launch_signal();
      }
    }

    if((newsignals & SIGBREAKF_CTRL_C) ||
      (!set && (newsignals & launchsignal))) {
      DebOut("launchd: !set || got SIGBREAKF_CTRL_C..\n");
      /* we are tired */
      //TODO set=setup(mytask, to_aros_signal, to_amigaos_signal, 1);
    }

    if(newsignals & SIGBREAKF_CTRL_D) {
      DebOut("launchd: got SIGBREAKF_CTRL_D..\n");
      /* tell uae, that we received a SIGBREAKF_CTRL_D */
      //TODO set=setup(mytask, to_aros_signal, to_amigaos_signal, 2);
    }
  }

  /* never arrive here */
  DebOut("launchd: try to sleep ..\n");
  C_LEAVE
}

int main (int argc, char **argv) {

  C_ENTER

  DebOut("launchd: started\n");

  if(!open_libs()) {
    C_LEAVE
    exit(1);
  }

  launch_signal_bit=AllocSignal(-1);
  if(launch_signal_bit == -1) {
    printf("no signal..!\n");
    DebOut("launchd: no signal..!\n");
    C_LEAVE
    exit(1);
  }
  launchsignal=1L << launch_signal_bit;

  mytask=FindTask(NULL);
  DebOut("launchd: task: %lx\n",mytask);

  while(1) {
    runme(); /* as we patched the system, we will run (sleep) forever */
  }

  C_LEAVE
}
