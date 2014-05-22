/************************************************************************ 
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE.
 *
 * Janus-UAE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Janus-UAE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id$
 *
 ************************************************************************
 *
 * A janus splash window is a small, own Zune Application, consisting 
 * of just one, simple window. It is no own executeable, but a j-uae
 * thread. 
 *
 * If there is a "janus.jpg" image, it is displayed in this window.
 * 
 * The text below the image can be controlled by a startup option for
 * j-uae (-splash_text).
 * 
 * The image is displayed, as long as the timeout is set (-splash_timeout)
 *
 * Both text and timeout can also be changed by a small m68k utility,
 * so you can show the user, at which step in the boot process he is at
 * the moment.
 * 
 * The window can be closed with "ESC" or the (invisible) close gadget.
 *
 ************************************************************************/

#include <exec/types.h>
#include <libraries/mui.h>
#include <dos/var.h>
#include <exec/libraries.h>
#include <workbench/startup.h>
#include <devices/timer.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/alib.h>
#include <proto/timer.h>
#include <aros/debug.h>

//#define JWTRACING_ENABLED 1
//#define JW_ENTER_ENABLED 1
#include "j.h"

struct Task *splash_window_task=NULL;

static void aros_splash_window_thread (void);
static struct timerequest *open_timer (void);
static void close_timer(struct timerequest *req);
static void set_timer(struct timerequest *req, ULONG seconds);
static void stop_timer(struct timerequest *req);

//static guint32 timer     =0;
static APTR    splash_win=NULL;
static APTR    splash_app=NULL;
static APTR    splash_txt=NULL;
static struct timerequest *splash_req=NULL;
static BOOL splash_timer_started=FALSE;

static char process_name[]="j-uae splash window";

void close_splash(void) {

  JWLOG("send SIGBREAKF_CTRL_C to splash task\n");

  if(!splash_window_task) {
    JWLOG("no splash_window_task running\n");
    return;
  }

  Signal(splash_window_task, SIGBREAKF_CTRL_C);

  while(splash_window_task || FindTask((STRPTR) process_name)) {
    JWLOG("waiting for splash thread to die..\n");
    Delay(50);
  }
}

void show_splash(void) {

  JWLOG("send SIGBREAKF_CTRL_D to splash task\n");

  if(!splash_window_task) {
    JWLOG("no splash_window_task running\n");
    return;
  }

  Signal(splash_window_task, SIGBREAKF_CTRL_D);
}

#if 0
static gint gtk_splash_timer (gpointer *task) {

  JWLOG("entered!\n");

  if(!splash_window_task) {
    JWLOG("no splash window\n");
    return FALSE;
  }

  JWLOG("close window!\n");

  close_splash();

  if(timer) {
    /* timer is automatically removed.. ? */
//    gtk_timeout_remove (timer);
    timer=0;
  }

  return FALSE;
}
#endif

void do_splash(APTR text, int time) {

  JWLOG("text %s, time %d\n", text, time);

  if(!splash_window_task || !splash_app) {
    JWLOG("splash window already closed\n");
    return;
  }

  if(!time || !text) {
    close_splash();
    return;
  }

  /* reset old timer */
  if(splash_req) {
    stop_timer(splash_req);
  }

  /* warning, race condition!!! */
  if(splash_win) {
    set(splash_txt, MUIA_Text_Contents, text);
  }

  /* set it again */
  if(splash_req) {
    set_timer(splash_req, time);
  }
  //timer = gtk_timeout_add (time*1000, (GtkFunction) gtk_splash_timer, (gpointer) splash_window_task);

}

static void set_timer(struct timerequest *req, ULONG seconds) {
  struct timeval timeval;

  if(req) {
    GetSysTime (&timeval);
    JWLOG("got systime seconds: %d\n", timeval.tv_secs);
    JWLOG("seconds: %d\n", seconds);


    req->tr_node.io_Command = TR_ADDREQUEST;
    req->tr_time.tv_secs  = timeval.tv_secs + seconds;
    req->tr_time.tv_micro = 0;
    splash_timer_started=TRUE;
    SendIO ((struct IORequest *)req);
    JWLOG("timer set!\n");
  }
}

static struct timerequest *open_timer (void) {
  struct MsgPort *port;
  struct timerequest *req;

  if (( port=CreateMsgPort () )) {
    if (( req=(struct timerequest *) CreateIORequest (port, sizeof(struct timerequest)) )) {
      //if(!OpenDevice ((STRPTR) "timer.device", UNIT_VBLANK, (struct IORequest *) req, 0)) {
      if(!OpenDevice ((STRPTR) "timer.device", UNIT_WAITUNTIL, (struct IORequest *) req, 0)) {
        JWLOG("opened timer.device: req=%lx\n", req);

        return req;
      }
      DeleteIORequest ((struct IORequest *)req);
    }
    DeleteMsgPort (port);
  }
  return NULL;
}

static void close_timer(struct timerequest *req) {
  struct MsgPort *port;

  JWLOG("req: %lx\n", req);

  if(req) {
    stop_timer(req);
    port=req->tr_node.io_Message.mn_ReplyPort; /* backup */
    CloseDevice ((struct IORequest *)req);
    JWLOG("port was: %lx\n", port);
    DeleteIORequest ((struct IORequest *)req);
    DeleteMsgPort (port);
  }
}

static void stop_timer(struct timerequest *req) {
  BOOL done=FALSE;
  LONG ret;

  JWLOG("req=%lx\n", req);
  JWLOG("splash_timer_started: %d\n", splash_timer_started);

  if (req) {
    if(splash_timer_started) {
      done=TRUE;
      //if (!CheckIO((struct IORequest *)req)) {
        JWLOG("Abort old timer ..\n");
        ret=AbortIO((struct IORequest *)req);
      //}
        JWLOG("ret: %d\n", ret);
        JWLOG("WaitIO ..\n");
      WaitIO((struct IORequest *)req);
    }
  }

  if(!done) {
    JWLOG("done nothing!\n");
  }
}

static void aros_splash_window_thread (void) {

  BPTR file;
  ULONG signals=0;
  ULONG timer_signal=0;

  if(( file=Open((STRPTR) "janus.png", MODE_OLDFILE) )) {
    Close(file);

    splash_app=ApplicationObject,
            MUIA_Application_UseCommodities, FALSE,
            SubWindow, splash_win=WindowObject,
              MUIA_Window_Borderless, TRUE,
              MUIA_Window_Width, 200,
              WindowContents, VGroup,
                MUIA_InnerTop,    40,
                MUIA_InnerBottom, 40,
                MUIA_InnerLeft,   40,
                MUIA_InnerRight,  40,
                Child, ImageObject,
                  MUIA_Image_Spec, "3:janus.png",
                  End,
                Child, TextObject,
                  MUIA_Text_Contents, "\n\33b\33cJanus-UAE\n\33n\33cVersion " PACKAGE_VERSION "\n",
                End,
                Child, splash_txt=TextObject,
                  MUIA_Text_PreParse, "\33c",
                  MUIA_Text_Contents, "starting...\n",
                End,
              End,
            End,
          End;
  }
  else {
    splash_app=ApplicationObject,
            MUIA_Application_UseCommodities, FALSE,
            SubWindow, splash_win=WindowObject,
              MUIA_Window_Borderless, TRUE,
              MUIA_Window_Width, 200,
              WindowContents, VGroup,
                MUIA_InnerTop,    40,
                MUIA_InnerBottom, 40,
                MUIA_InnerLeft,   40,
                MUIA_InnerRight,  40,
                Child, TextObject,
                  MUIA_Text_Contents, "\n\33b\33cJanus-UAE\n\33n\33cVersion " PACKAGE_VERSION "\n",
                End,
                Child, splash_txt=TextObject,
                  MUIA_Text_PreParse, "\33c",
                  MUIA_Text_Contents, "starting...\n",
                End,
              End,
            End,
          End;

  }

  if(!splash_app || !splash_win) {
    goto EXIT;
  }

  splash_req=open_timer();
  if(!splash_req) {
    goto EXIT;
  }

  timer_signal= 1L << splash_req->tr_node.io_Message.mn_ReplyPort->mp_SigBit;
  JWLOG("timer_signal: %lx\n", timer_signal);

  DoMethod(splash_win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            splash_app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

  while(DoMethod(splash_app, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit) {
    signals = Wait(signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | timer_signal);
    JWLOG("got signal: %lx\n", signals);
    if (signals & SIGBREAKF_CTRL_C) {
      break;
    }
    if (signals & SIGBREAKF_CTRL_D) {
      set(splash_win, MUIA_Window_Open, TRUE);
    }
    if (signals & timer_signal) {
      JWLOG("GOT TIMER EVENT!!\n");
      break;
    }


  }

  /* ... and a time to die. */
  JWLOG("closing splash window ..!\n");

EXIT:

  if(splash_req) {
    stop_timer(splash_req);
    close_timer(splash_req);
  }

  if(splash_win) {
    JWLOG("hide splash_win %lx\n", splash_win);
    set(splash_win, MUIA_Window_Open, FALSE);
    splash_win=NULL;

    JWLOG("close splash_app %lx\n", splash_app);
    MUI_DisposeObject(splash_app);
  }

  splash_app=NULL;
  splash_txt=NULL;

  JWLOG(" === splash window thread dies ====\n");

  /* we are going to die soon enough */
  splash_window_task=NULL;

  LEAVE
}


int aros_splash_start_thread (void) {

  ENTER

  JWLOG("starting thread ..\n");

  if(splash_window_task) {
    JWLOG("splash window thread already running\n");
    return TRUE;
  }

  splash_window_task = (struct Task *)
	  myCreateNewProcTags ( //NP_Output, Output (),
				//NP_Input, Input (),
				NP_Name, (ULONG) process_name,
				//NP_CloseOutput, FALSE,
				//NP_CloseInput, FALSE,
				NP_StackSize, 0x4000,
				NP_Priority, 0,
				NP_Entry, (ULONG) aros_splash_window_thread,
				TAG_DONE);


  if(!splash_window_task) {
    JWLOG("ERROR: could not create splash window thread\n");
    LEAVE
    return FALSE;
  }

  JWLOG("splash window thread created\n");

  LEAVE
  return TRUE;
}

