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
#include <string.h>

#include <devices/clipboard.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <exec/nodes.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <workbench/startup.h>
#include <dos/dostags.h>

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

#include "include/wbstart.h"
#include "include/inline/wbstart.h"

#include "janus-daemon.h"
#include "launch-daemon.h"

struct Library *WBStartBase=NULL;
struct Library *DOSBase    =NULL;

char verstag[] = "\0$VER: launch-daemon 0.3";
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

   if(!(DOSBase=OpenLibrary("dos.library",36L))) {
     printf("unable to open dos.library!?\n");
     return FALSE;
   }

   if (!(WBStartBase=OpenLibrary("wbstart.library", 2L))) {
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

/****************************************************************
 * start_it
 *
 * start filename in path with args
 *
 * args can be NULL, if there are no parameters
 ****************************************************************/
static LONG start_it(char *path, char *filename, struct WBArg *args) {
  LONG rc;
  ULONG nr_args;

  DebOut("launchd: start_it(%s, %s, %lx)\n", path, filename, args);

  /* count arguments */
  nr_args=0;
  if(args!=NULL) {
    while((args[nr_args].wa_Name) || (args[nr_args].wa_Lock)) {
      nr_args++;
    }
  }
#if 0
  if(args && args[0]) {
    while(args[nr_args]) {
      nr_args++;
    }
  }
#endif
  DebOut("launchd: #args: %d\n", nr_args);

  if(nr_args) {
    rc = WBStartTags(WBStart_DirectoryName,  (ULONG) path,
              WBStart_Name,          (ULONG) filename,
             WBStart_ArgumentCount,  nr_args,
          WBStart_ArgumentList,   (ULONG) args,
            TAG_DONE);
  }
  else {
    rc = WBStartTags(WBStart_DirectoryName, (ULONG) path,
              WBStart_Name,          (ULONG) filename,
            TAG_DONE);
  }

  if(rc != RETURN_OK) {
    DebOut("launchd: unable to start it: rc=%d\n", rc);
  }

  return rc;
}


/*
struct WBArg {
  BPTR  wa_Lock; => a lock descriptor 
  BYTE *wa_Name; => a string relative to that lock 
};
*/


static BYTE *str_dup_pool(void *pool, char *in) {
  char *result;

  DebOut("launchd: str_dup_pool(.., %s)\n", in);

  result=(char *) AllocPooled(pool, strlen(in)+1);
  strcpy(result, in);

  return (BYTE *) result;
}

static char *get_filename(char *in) {
  char *sep;

  DebOut("get_filename(%s)\n", in);

  sep=PathPart(in);

  DebOut("launchd: filename: %s\n", sep+1);

  return sep+1;
}

/* WARNING: this destroys in !!*/
static char *get_path(void *pool, char *in) {
  char *sep;
  char *res;
  char restore;

  DebOut("get_path(%s)\n", in);

  sep=PathPart(in);
  restore=sep[0];
  sep[0]=(char) 0;

  res=AllocPooled(pool, strlen(in)+1);
  strcpy(res, in);

  sep[0]=restore;

  DebOut("launchd: path: %s\n", res);

  return res;
}

/*
 * create an array of WBArg for the wbstart call.
 * we get all strings referenced in ULONG *in
 *
 * we use a pool here, to avoid complicate FreeVecs
 *
 * in looks like that:
 *
 * 0 : ignored here
 * 4 : ignored here
 * 8 : ignored here
 * 12: (ULONG) offset 1
 * 16: (ULONG) offset 2
 * ...
 * 20: NULL
 * offset 1: string 1
 * offset 2: string 2
 */
static struct WBArg *create_wbargs(void *pool, ULONG *in) {
  ULONG nr;
  ULONG i;
  ULONG t;
  char *strings;
  struct WBArg *args;
  BPTR  lock;
  char *path;
  ULONG *ref;

  DebOut("launchd: create_wbargs\n");

  /* here is the beginning of the offsets */
  //ref=in+(3*sizeof (ULONG *));
  ref=&in[3];

  nr=0;
  while(ref[nr]) {
    DebOut("launchd: ref[%d]: %d (%s)\n", nr, ref[nr], ref[nr]);
    nr++;
  }

  if(!nr) {
    DebOut("launchd: no args\n");
    return NULL;
  }

  DebOut("launchd: nr: %d\n", nr);

  args=AllocPooled(pool, sizeof(struct WBArg) * (nr+1));
  strings=(char *) in;

  t=0;
  for(i=0; i<nr; i++) {
    path=get_path(pool, strings + ref[i]);
    lock=Lock(path, ACCESS_READ);
    if(lock) {
      /* attention: get_path destroys input strings, so use get_filename first! */
      args[t].wa_Name=str_dup_pool(pool, get_filename(strings + ref[i]) );
      args[t].wa_Lock=lock;
      DebOut("launchd: args[%d]: Lock %lx, Name %s\n", t, args[t].wa_Lock, args[t].wa_Name);
      t++;
    }
    else {
      DebOut("launchd: WARNING: could not lock path #%d: %s)\n", i, path);
    }
  }

  /* terminate it, not neccessary, but feels better */
  args[t].wa_Name=NULL;
  args[t].wa_Lock=NULL;

  return args;
}

static void handle_launch_signal(void) {
  ULONG *command_mem;
  char  *command_string;
  char  *path;
  char  *fullpath;
  char  *filename;
  struct WBArg *wbargs=NULL;
  void  *pool;
  BOOL   done;
  ULONG  result;

  C_ENTER

  DebOut("launchd: handle_launch_signal()\n");

  command_mem=AllocVec(8192, MEMF_CLEAR);
  if(!command_mem) {
    C_LEAVE
    return;
  }

  pool=CreatePool(MEMF_CLEAR, 256, 256);

  done=FALSE;
  DebOut("launchd: handle_launch_signal->while..\n");
  while(!done) {
    DebOut("launchd: handle_launch_signal->while(!done)\n");

    calltrap (AD_LAUNCH_JOB, LD_GET_JOB, command_mem);

    if(command_mem[0]==0) {
      DebOut("launchd: no commands waiting for us any more ..\n");
      done=TRUE;
    }
    else {
      if(command_mem[0]==1) {

  /* WB == 1 */

  command_string=(char *) command_mem;
  path          =command_string + command_mem[1];
  filename      =command_string + command_mem[2];

  DebOut("launchd: Start WB program \n", filename);

  if(command_mem[3]) {
    wbargs      =create_wbargs(pool, command_mem);
  }
  else {
    DebOut("launchd: we have no arguments\n");
  }

  start_it(path, filename, wbargs);
  /* TODO: unlock all :( */
      }
      else {
  /* CLI == 2 */
  /* we get everything in path: path,filename and arguments */

  command_string=(char *) command_mem;
  fullpath      =         command_string + command_mem[1];
#if 0
  filename      =         command_string + command_mem[2];
#endif

  DebOut("launchd: fullpath: >%s<\n", fullpath);
#if 0
  DebOut("launchd: cmd:  >%s<\n", filename);
  sprintf(cli_cmd, "%s/%s\n", path, filename);
  DebOut("cli_cmd: %s\n", cli_cmd);
#endif

  result=SystemTags(fullpath,
            SYS_Asynch, TRUE,
            SYS_Input,  Open("CON://200/100/RunAmigaOs/CLOSE/AUTO/WAIT", MODE_OLDFILE),
            SYS_Output, NULL,
            TAG_DONE);

  DebOut("result: %d\n", result);

      }
    }
  }

  DebOut("launchd: handle_launch_signal->FreeVec\n");
  FreeVec(command_mem);

  if(pool) {
    DeletePool(pool);
  }

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
