/************************************************************************ 
 *
 * Janus-Daemon
 *
 * Copyright 2009-2010 Oliver Brunner - aros<at>oliver-brunner.de
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
#include <proto/layers.h>
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

#ifdef __AROS__
#include <aros/system.h>
#endif

//#define DEBUG 1
#include "janus-daemon.h"

int __nocommandline = 0; /*???*/

#ifdef __AROS__

/* those registers must be reset to original values */
#define PUSHSTACK     "movem.l %d2-%d7/%a2-%a4,-(%SP)\n"
#define POPSTACK      "movem.l (%SP)+,%d2-%d7/%a2-%a4\n"
#define PUSHFULLSTACK "movem.l %d0-%d7/%a0-%a6,-(%SP)\n"
#define POPFULLSTACK  "movem.l (%SP)+,%d0-%d7/%a0-%a6\n"

#else
char verstag[] = "\0$VER: janus-daemon 1.4 [AmigaOS]";

#define PUSHSTACK     "movem.l d2-d7/a2-a4,-(SP)\n"
#define POPSTACK      "movem.l (SP)+,d2-d7/a2-a4\n"
#define PUSHFULLSTACK "movem.l d0-d7/a0-a6,-(SP)\n"
#define POPFULLSTACK  "movem.l (SP)+,d0-d7/a0-a6\n"

#endif

#if 0
struct Window *old_MoveWindowInFront_Window,
              *old_MoveWindowInFront_BehindWindow;
ULONG          old_MoveWindowInFront_Counter;
#endif

ULONG          *cmdbuffer=NULL;

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

ULONG        intdata[2];

#ifndef __AROS__
struct Library *CyberGfxBase;
#endif

BOOL open_libs() {
  ENTER
#ifndef __AROS__
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
   if (!(LayersBase=(struct LayersBase *) OpenLibrary("layers.library",0))) {
     printf("unable to open layers.library\n");
     LEAVE
     return FALSE;
   }

   CyberGfxBase=OpenLibrary("cybergraphics.library", 0);
   DebOut("CyberGfxBase: %lx\n",CyberGfxBase);
#endif

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
UBYTE buffer[MAXPUBSCREENNAME];

ULONG *setup_command_mem=NULL;

int setup(struct Task *task, ULONG signal, ULONG stop) {

  ENTER

  DebOut("(%lx,%lx,%d)\n",(ULONG) task, signal, stop);

  if(!setup_command_mem) {
    setup_command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);
  }

  //DebOut("janusd: memory: %lx\n",(ULONG) command_mem);

  setup_command_mem[ 0]=(ULONG) task;
  setup_command_mem[ 4]=(ULONG) signal;
  setup_command_mem[ 8]=(ULONG) stop;

  /* tell j-uae, which guest OS is running 
   * 0: AmigaOS
   * 1: AROS
   */
#ifdef __AROS__
  setup_command_mem[12]=(ULONG) 1;
#else
  setup_command_mem[12]=(ULONG) stop;
#endif

  /* return value is too unrealiable! */
  calltrap (AD_SETUP, AD__MAXMEM, setup_command_mem);

  state=setup_command_mem[8];


  DebOut("result %d\n",(int) state);

  LEAVE

  return state;
}

#ifndef __AROS__
/* A1 contains the data pointer
 * you have to clear the Z flag on exit (moveq #0, D0)
 * you "should" return 0xdff000 in A0
 * 
 */
__asm__("_vert_int:\n"
	PUSHSTACK
	"move.l 4,A6\n"
	"move.l 4(A1),D0\n"
	"move.l (A1),A1\n"
	"jsr -324(A6)\n"
	POPSTACK
	"lea 0xdff000, A0\n"
	"moveq #0, D0\n"

        "rts\n");
/*
 * assembler functions need to be declared or used, before
 * you can reference them (?).
 */
void vert_int();

#else

AROS_UFP4(ULONG, vert_int,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6));

#endif
/* setup_vert_int 
 *
 * First, I sent all the Signals from the UAE thread using
 * uae_Signal. This works well, as long as AmigaOS is
 * healthy. As soon as it crashes, it might take down
 * the UAE thread doing uae_Signal :(. As signals
 * for janusd arrive quite often, this is not good.
 *
 * So we install our own INTB_VERTB.
 */
static int setup_vert_int(struct Task *task, ULONG signal) {
  struct Interrupt *vbint;

  DebOut("(%lx, %lx)", task, signal);

  if (!(vbint = AllocMem(sizeof(struct Interrupt), MEMF_PUBLIC|MEMF_CLEAR))) {
    return FALSE;
  }

  intdata[0]=(ULONG) task;
  intdata[1]=        signal;

  vbint->is_Node.ln_Type = NT_INTERRUPT;
  vbint->is_Node.ln_Pri = 1;
  vbint->is_Node.ln_Name = "JanusD vbint";
  vbint->is_Data = (APTR)&intdata;
#ifndef __AROS__
  vbint->is_Code = vert_int;
#else
  vbint->is_Code = (APTR) vert_int;
#endif


  DebOut("AddIntServer(.., %lx)\n", vbint);
  AddIntServer(INTB_VERTB, vbint); /* Do it! */
  DebOut("AddIntServer(.., %lx) done\n", vbint);

  return TRUE;
}

/***************************************
 * enable/disable uae main window
 ***************************************/
void switch_uae_window() {

  ENTER;

  DebOut("entered");

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
  ULONG        sigs;
#ifdef __AROS__
  struct ScreenNotifyMessage *notify_msg;
#endif

  ENTER

  DebOut("entered\n");

  DebOut("running (CTRL-C to go to normal mode, CTRL-D to rootless mode)..\n");

  sigs=SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_F;
#ifdef __AROS__
  sigs=sigs|notify_signal;
#endif

  done=FALSE;
  init=FALSE;
  while(!done) {
    DebOut("Wait() ..\n");
    newsignals=Wait(mysignal | sigs);
    set=setup(mytask, mysignal, 0);
    if(newsignals & mysignal) {
      if(set) {
        /* we are active */

        if(!init) {
          /* disabled -> enabled */
          init=TRUE;
          DebOut("update screens ..\n");
          update_screens(); /* report all open screens once, 
                             * updates again at every openwindow patch
                  			     * call
                  			     */
          DebOut("screens updated\n");

          DebOut("update windows ..\n");
          update_windows(); /* report all open windows once,
			                       * new windows will be handled by the patches
			                       */
          DebOut("windows updated\n");
        }

        update_top_screen();
        sync_windows();
        report_uae_windows();
        report_host_windows(); /* bad! */
        sync_active_window();
        forward_messages();
      }
      sync_mouse();
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
        DebOut("janusd: tell uae, that we received a SIGBREAKF_CTRL_D\n");
        setup(mytask, mysignal, 2); /* we are back again */
      }
      else {
        DebOut("we are already active\n");
      }

    }
    if(set && (newsignals & SIGBREAKF_CTRL_F)) {
      /* force a manual update of all windows */
      DebOut("complete update of 68k windows ..\n");
      update_windows(); /* report all open windows once,
            * new windows will be handled by the patches
            */
      DebOut("windows updated\n");
    }
#ifdef __AROS__
    if(newsignals & notify_signal) {
      DebOut("notify_signal received\n");

      while((notify_msg = (struct ScreenNotifyMessage *) GetMsg (notify_port))) {
        
        handle_notify_msg(notify_msg->snm_Class, notify_msg->snm_Object);

        ReplyMsg ((struct Message*) notify_msg);
      }

    }
#endif

  }

  /* never arrive here */

  DebOut("try to sleep ..\n");

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
  DebOut("started (%d)\n",argc);

  if(!open_libs()) {
    exit(1);
  }

  /* Just open the default public screen. Makes life easier and
   * gets AROS guests booting without problems.
   * AmigaOS my just work by chance, as there the booting
   * is faster and the workbench screen might be up
   * early enough. Not nice, but no harm done either.
   */
  {
    struct Screen *s;
    s=LockPubScreen(NULL);
    UnlockPubScreen(NULL, s);
  }

  /* try to get a signal */
  mysignal_bit=AllocSignal(-1);
  if(mysignal_bit == -1) {
    printf("no signal..!\n");
    DebOut("no signal..!\n");
    LEAVE
    exit(1);
  }
  mysignal=1L << mysignal_bit;

  mytask=FindTask(NULL);
  DebOut("task: %lx\n",mytask);

  if(!init_sync_mouse()) {
    DebOut("ERROR: init_sync_mouse failed\n");
    printf("ERROR: init_sync_mouse failed\n");
    LEAVE
    exit(1);
  }

  init_sync_windows(); /* never fails, does nothing critical at all */
  init_sync_screens(); /* never fails, does nothing critical at all */

  setup(mytask, mysignal, 0); /* init everything for the patches */
  patch_functions();

  setup_vert_int(mytask, mysignal);


  SetTaskPri(mytask, 55);

  while(1) {
    runme(); /* as we patched the system, we will run (sleep) forever */
  }

  /* can't be reached.. */
  /*remove_vert_int()*/
  unpatch_functions();
  FreeSignal(mysignal_bit);
  free_sync_mouse();
  if(setup_command_mem) {
    FreeVec(setup_command_mem);
  }

  DebOut("exit\n");

  LEAVE

  exit (0);
}

#ifdef __AROS__
#undef SysBase

/* intdata[0]=(ULONG) task; */
/* intdata[1]=        signal; */

AROS_UFH4(ULONG, vert_int,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(ULONG *, intdata, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    Signal((struct Task *) intdata[0], intdata[1]);

    return 0;

    AROS_USERFUNC_EXIT
}

#endif
