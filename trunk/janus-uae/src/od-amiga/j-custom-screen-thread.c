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
 ************************************************************************/

#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <clib/intuition_protos.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "td-amigaos/thread.h"
#include "hotkeys.h"

#include "j.h"
#include "memory.h"

/***********************************************************
 * handle ScreenNotifyMessages
 *
 * AROS does not send a SDEPTH_TOFRONT/SDEPTH_TOBACK to our
 * screen always. If our screen get to front with a 
 * ScreenToFront call, then we get a message. If another
 * screen gets a ScreenToFront, we *don't* get a 
 * SDEPTH_TOBACK. And vice versa.
 *
 * So we try to catch a correct tofront with all
 * eventual possibilities..
 ***********************************************************/
static void handle_msg(JanusScreen *jscreen, 
                       ULONG notify_class, ULONG notify_code, ULONG notify_screen,
		       struct Process *thread) {

  BOOL tofront=FALSE;
  ULONG i;

  uae_no_display_update=TRUE;

  JWLOG("aros_cscr_thread[%lx]: notify_screen %lx, notify_class %lx, notify_code %lx\n", thread, notify_screen, notify_class, notify_code);

  if((jscreen->arosscreen == (struct Screen *) notify_screen) && (notify_code & SDEPTH_TOFRONT)) {
    tofront=TRUE;
    JWLOG("aros_cscr_thread[%lx]: SDEPTH_TOFRONT for our screen %lx\n", thread, notify_screen);
  }
  if((jscreen->arosscreen == (struct Screen *) notify_screen) && (notify_code & SDEPTH_TOBACK)) {
    tofront=FALSE;
    JWLOG("aros_cscr_thread[%lx]: SDEPTH_TOBACK for our screen\n");
  }
  if((jscreen->arosscreen != (struct Screen *) notify_screen) && (notify_code & SDEPTH_TOBACK)) {
    /* wait until the old screen is not the first screen any more */
    i=10;
    while((IntuitionBase->FirstScreen == (struct Screen *) notify_screen) && i--) {
      Delay(2);
    }
    /* we have to check, if we are the first now */
    if(IntuitionBase->FirstScreen == jscreen->arosscreen) {
      tofront=TRUE;
      JWLOG("aros_cscr_thread[%lx]: SDEPTH_TOBACK to foreign screen, but we are first now\n");
    }
    else {
      /* another one became front screen, so we don't care */
      JWLOG("aros_cscr_thread[%lx]: SDEPTH_TOBACK to foreign screen, but we still are not first\n");
      uae_no_display_update=FALSE;
      reset_drawing();
      return;
    }
  }
  if((jscreen->arosscreen != (struct Screen *) notify_screen) && (notify_code & SDEPTH_TOFRONT)) {
    JWLOG("aros_cscr_thread[%lx]: SDEPTH_TOFRONT to foreign screen, but we were not first\n");
    if(janus_active_screen != janus_active_screen) {
      uae_no_display_update=FALSE;
      reset_drawing();
      return;
    }
    tofront=FALSE;
    JWLOG("aros_cscr_thread[%lx]: SDEPTH_TOFRONT to foreign screen, so we are going back\n");
  }


  /* SDEPTH_TOFRONT/BACK is handled analog to IDCMP_ACTIVEWINDOW/INACTIVE */
  switch(tofront) {
    case TRUE:
      JWLOG("aros_cscr_thread[%lx]: SDEPTH_TOFRONT\n", thread);
      JWLOG("aros_cscr_thread[%lx]: wait for sem_janus_active_custom_screen\n", thread);
      ObtainSemaphore(&sem_janus_active_custom_screen);
      janus_active_screen=jscreen;
      JWLOG("aros_cscr_thread[%lx]: we (jscreen %lx) are active front screen now\n", thread, janus_active_screen);

      S  = jscreen->arosscreen;
      CM = jscreen->arosscreen->ViewPort.ColorMap;
      RP = &jscreen->arosscreen->RastPort;
      W  = jscreen->arosscreen->FirstWindow;

      ReleaseSemaphore(&sem_janus_active_custom_screen);

      //ActivateWindow(jscreen->arosscreen->FirstWindow); /* might not be necessary */
      i=20;
      while((aos3_first_screen != jscreen->aos3screen) && i--) {
	Delay(5);
	JWLOG("waiting for aos3_first_screen %lx == jscreen->aos3screen %lx (#%d)\n", aos3_first_screen, 
	                                                                              jscreen->aos3screen, i);
      }

      if(!i) {
	JWLOG("WARNING: unable to wait for aos3 %lx screen to be the first %lx\n", jscreen->aos3screen, 
	                                                                           aos3_first_screen);
      }

      /* TODO !!*/
      uae_main_window_closed=FALSE;

      break;

    case FALSE:
      JWLOG("aros_cscr_thread[%lx]: SDEPTH_TOBACK\n", thread);
      JWLOG("aros_cscr_thread[%lx]: wait for sem_janus_active_custom_screen\n", thread);

      Delay(10);
      ObtainSemaphore(&sem_janus_active_custom_screen);

      /* no other janus custom screen claimed to be active */
      if(janus_active_screen == jscreen) {
	janus_active_screen=NULL;
	uae_main_window_closed=TRUE;
	JWLOG("aros_cscr_thread[%lx]: we (jscreen %lx) are not active any more\n", thread, janus_active_screen);
      }

      ReleaseSemaphore(&sem_janus_active_custom_screen);
      break;
  }

  uae_no_display_update=FALSE;
  reset_drawing();

}

void handle_input(struct Window *win, JanusWin *jwin, ULONG class, UWORD code, int qualifier, struct Process *thread) ;

/***********************************************************
 * event handler for custom screen
 ***********************************************************/  
static void handle_custom_events_S(JanusScreen *jscreen, struct Process *thread) {

  struct Screen              *screen;
  ULONG                       signals;
  ULONG                       notify_signal;
  BOOL                        done;
  struct MsgPort             *port;
  JanusWin                    dummywin;
  IPTR                        notify;

  struct ScreenNotifyMessage *notify_msg;
  ULONG                       notify_class, notify_code, notify_object, notify_userdata;

  struct IntuiMessage        *intui_msg;
  UWORD                       intui_code, intui_qualifier;
  ULONG                       intui_class;

  screen=jscreen->arosscreen;
  JWLOG("screen: %lx\n", screen);

  port=CreateMsgPort();

  if(!port) {
    JWLOG("ERROR: no port!\n", port);
    return;
  }
  JWLOG("port: %lx\n", port);

  notify=StartScreenNotifyTags(SNA_MsgPort,  port,
			    SNA_UserData, screen,
			    SNA_SigTask,  thread,
			    SNA_Notify,   SNOTIFY_SCREENDEPTH /*| SNOTIFY_WAIT_REPLY*/,
			    TAG_END); 

  if(!notify) {
    JWLOG("ERROR: unable to StartScreenNotifyTagList!\n");
    return;
  }

  notify_signal=1L << port->mp_SigBit;

  JWLOG("jscreen->arosscreen: %lx\n", jscreen->arosscreen);

  done=FALSE;
  while(!done) {
    JWLOG("aros_cscr_thread[%lx]: wait for signal\n", thread);

    signals = Wait(notify_signal | SIGBREAKF_CTRL_C);
    JWLOG("aros_cscr_thread[%lx]: signal reveived\n", thread);

    if(signals & SIGBREAKF_CTRL_C) {
      JWLOG("aros_cscr_thread[%lx]: SIGBREAKF_CTRL_C received\n", thread);
      done=TRUE;
      break;
    } else 
    if(!done && (signals & notify_signal)) {
      JWLOG("aros_cscr_thread[%lx]: notify_signal received\n", thread);

      while((notify_msg = (struct ScreenNotifyMessage *) GetMsg (port))) {
	notify_class     = notify_msg->snm_Class;
	notify_code      = notify_msg->snm_Code;
	notify_object    = notify_msg->snm_Object;
	notify_userdata  = notify_msg->snm_UserData;

	ReplyMsg ((struct Message*) notify_msg); 

	JWLOG("aros_cscr_thread[%lx]: notify_class %lx (SNOTIFY_SCREENDEPTH %lx)\n", thread,
	       notify_class, SNOTIFY_SCREENDEPTH);
	JWLOG("aros_cscr_thread[%lx]: notify_object %lx screen %lx jscreen->arosscreen: %lx\n", thread,
	       notify_object, screen, jscreen->arosscreen);

//	if(notify_object == (ULONG) screen) {
//	  JWLOG("aros_cscr_thread[%lx]: own notify_object %lx\n", thread, notify_object);
	  /* this message is for us */
	  switch (notify_class) {
	      case SNOTIFY_SCREENDEPTH:
	    	handle_msg(jscreen, notify_class, notify_code, notify_object, thread);
  	      break;
  	  }
//	}
//	else {
//	  JWLOG("aros_cscr_thread[%lx]: foreign notify_object %lx\n", thread, notify_object);
//	}
      }
    }

  } /* while(done) */

  while(!EndScreenNotify(notify)) {
    /* might be in use */
    sleep(1);
  }
  notify=(IPTR) NULL;

  DeleteMsgPort(port);
  port=NULL;

  /* necessary !? */
  //appw_events();
}

/************************************************************************
 * new_aros_custom_screen for an aos3screen
 *
 * open new one
 *
 * This function must not return, until both screen and window are
 * opened. Wait for screen and window task to complete init
 ************************************************************************/
static struct Screen *new_aros_custom_screen(JanusScreen *jscreen, 
                                             uaecptr aos3screen,
				  	     struct Process *thread) {
  JanusWin *jwin;
  UWORD depth;
  ULONG mode;
  UWORD width, height;
  ULONG newwidth, newheight;
  const UBYTE preferred_depth[] = {8, 15, 16, 24, 32, 0};
  ULONG i;
  ULONG error;
  ULONG maxdelay;

  JWLOG("entered(jscreen %lx, aos3screen %lx)\n", jscreen, aos3screen);

  /* hide all dirty effects.. */
  uae_no_display_update=TRUE;

#if 0
  //width=720;
  //height=568;
  //JWLOG("w: %d h: %d\n", width, height);
#endif
  width =jscreen->maxwidth;
  height=jscreen->maxheight;

  JWLOG("w: %d h: %d\n", width, height);
 
  currprefs.gfx_width_win=width;
  currprefs.gfx_height =  height;
  currprefs.amiga_screen_type=0 /*UAESCREENTYPE_CUSTOM*/;

  /* this would be the right approach, but getting this pointer stuff
   * working in 68k adress space is a pain. 
   * screen->vp->RasInfo->BitMap->Depth ... */

  /* open a screen, which matches best */
  /* TODO: care for lores etc here, too! */
  i=0;
  mode=INVALID_ID;
  newwidth =width;
  newheight=height;
  while(mode==(ULONG) INVALID_ID && preferred_depth[i]) {
    mode=find_rtg_mode(&newwidth, &newheight, preferred_depth[i]);
    depth=preferred_depth[i];
    i++;
  }

  if(mode == (ULONG) INVALID_ID) {
    JWLOG("ERROR: could not find modeid !?!!\n");
    uae_no_display_update=FALSE;
    return NULL;
  }

  JWLOG("found modeid: %lx\n", mode);
  JWLOG("found w: %d h: %d\n", newwidth, newheight);

  /* If the screen is larger than requested, center UAE's display */
  XOffset=0;
  YOffset=0;
  if (newwidth > (ULONG) width) {
    XOffset = (newwidth - width) / 2;
  }
  if (newheight > (ULONG) height) {
    YOffset = (newheight - height) / 2;
  }
  JWLOG("XOffset, YOffset: %d, %d\n", XOffset, YOffset);

  /* TODO set depth! */
  jscreen->arosscreen=OpenScreenTags(NULL,
                                     //SA_Type,      PUBLICSCREEN,
				     //SA_PubName,   "FOO",
                                     SA_Width,     newwidth,
				     SA_Height,    newheight,
				     SA_Depth,     depth,
				     SA_DisplayID, mode,
				     SA_Behind,    FALSE,
				     SA_ShowTitle, FALSE,
				     SA_Quiet,     TRUE,
				     SA_ErrorCode, (ULONG)&error,
				     TAG_DONE);

  if(!jscreen->arosscreen) {
    JWLOG("ERROR: could not open screen !? (Error: %ld\n",error);
    gui_message ("Cannot open custom screen (Error: %ld)\n", error);
    uae_no_display_update=FALSE;
    return NULL;
  }

  JWLOG("new aros custom screen: %lx (%d x %d)\n",jscreen->arosscreen, 
                                                  jscreen->arosscreen->Width, 
                                                  jscreen->arosscreen->Height);

  /* add the new window to the janus_window list as a custom window 
   *
   * then start a thread for it
   */
  ObtainSemaphore(&sem_janus_screen_list);
  ObtainSemaphore(&sem_janus_window_list);

  jwin=(JanusWin *) AllocVec(sizeof(JanusWin),MEMF_CLEAR);
  JWLOG("new jwin %lx for janus custom screen %lx\n", jwin, jscreen);
  if(!jwin) {
    CloseScreen(jscreen->arosscreen);
    ReleaseSemaphore(&sem_janus_window_list);
    ReleaseSemaphore(&sem_janus_screen_list);
    gui_message ("out of memory !?");
    uae_no_display_update=FALSE;
    return NULL;
  }

  jwin->custom =TRUE;
  jwin->aroswin=NULL;
  jwin->jscreen=jscreen;
  jwin->aos3win=NULL;
  jwin->mempool=CreatePool(MEMF_CLEAR|MEMF_SEM_PROTECTED, 0xC000, 0x8000);
  janus_windows=g_slist_append(janus_windows,jwin);

  JWLOG("appended jwin %lx to janus_windows\n", jwin);

  ReleaseSemaphore(&sem_janus_window_list);
  ReleaseSemaphore(&sem_janus_screen_list);

  aros_win_start_thread(jwin);

  /* we need to wait, until our window is open! */
  maxdelay=100;
  while(!jscreen->arosscreen->FirstWindow && maxdelay--) {
    JWLOG("aros_cscr_thread[%lx]: wait until window is open (%d tries left ..)\n", 
          thread, maxdelay);
    Delay(5);
  }

  if(!maxdelay) {
    JWLOG("WARNING: could not wait until window is open..!!\n");
  }

  /* set our screen active */
  ObtainSemaphore(&sem_janus_active_custom_screen);
  janus_active_screen=jscreen;
  JWLOG("aros_cscr_thread[%lx]: we (jscreen %lx) are open and active now\n", thread, janus_active_screen);
  ReleaseSemaphore(&sem_janus_active_custom_screen);

  /* TODO !!*/
  /* this delay avoids drawings of wrong screen contents around the real amigaos screen
   * contents. Not nice, but works so far.
   */
  Delay(10);

  /* now update display again */
  uae_main_window_closed=FALSE;
  uae_no_display_update=FALSE;

  reset_drawing(); /* flush full screen, so that any potential gfx glitches are gone */

  //j_stop_window_update=FALSE;
  return jscreen->arosscreen;
}

/***********************************************************
 * every AOS3 custom screen gets its own process
 ***********************************************************/  
static void aros_custom_screen_thread (void) {

  struct Process *thread = (struct Process *) FindTask (NULL);
  GSList         *list_screen=NULL;
  JanusScreen    *jscr=NULL;
  GSList         *list_win=NULL;
  JanusWin       *jwin=NULL;
  BOOL            closed;

  /* There's a time to live .. */

  JWLOG("aros_cscr_thread[%lx]: thread running \n",thread);

  /* search for ourself */
  ObtainSemaphore(&sem_janus_screen_list);
  list_screen=g_slist_find_custom(janus_screens, 
	 		     	  (gconstpointer) thread,
			     	  &aos3_screen_process_compare);

  ReleaseSemaphore(&sem_janus_screen_list);

  if(!list_screen) {
    JWLOG("aros_cscr_thread[%lx]: ERROR: screen of this thread not found in screen list !?\n",thread);
    goto EXIT; 
  }

  jscr=(JanusScreen *) list_screen->data;
  JWLOG("aros_scr_thread[%lx]: win: %lx \n",thread,jscr);


  /* open a new screen for us*/
  jscr->arosscreen=new_aros_custom_screen(jscr, (uaecptr) jscr->aos3screen, thread);

  if(!jscr->arosscreen) {
    goto EXIT;
  }

  handle_custom_events_S(jscr, thread);

  JWLOG("aros_cscr_thread[%lx]: exit clean..\n", thread);

  /* ... and a time to die. */

EXIT:
  JWLOG("aros_cscr_thread[%lx]: EXIT\n",thread);

  ObtainSemaphore(&sem_janus_screen_list);

  ObtainSemaphore(&sem_janus_active_custom_screen);
  if(janus_active_screen == jscr) {
    JWLOG("aros_cscr_thread[%lx]: restore S to %lx, W to %lx, CM to %lx and RP to %lx\n",thread,
				  original_S, original_W, original_CM, original_RP);
    S= original_S;
    W= original_W;
    CM=original_CM;
    RP=original_RP;

    janus_active_screen=NULL;
    JWLOG("aros_cscr_thread[%lx]: set janus_active_screen=NULL\n", thread);
  }
  else {
    JWLOG("aros_cscr_thread[%lx]: another jscr (%lx) is already janus_active_screen\n", thread, janus_active_screen);
  }

  ReleaseSemaphore(&sem_janus_active_custom_screen);

  if(jscr->arosscreen->FirstWindow) {

    /* our custom screen has only one window, so signal the task to close it */
    ObtainSemaphore(&sem_janus_window_list);
    list_win= g_slist_find_custom(janus_windows, 
                                         (gconstpointer) jscr->arosscreen->FirstWindow,
					 &aros_window_compare);
    if(list_win) {
      jwin=(JanusWin *) list_win->data;
      JWLOG("Signal jwin %lx task %lx SIGBREAKF_CTRL_C..\n", jwin, jwin->task);
      Signal(jwin->task, SIGBREAKF_CTRL_C);
    }
    else {
      JWLOG("ERROR! could not find aros window %lx in janus_windows !!?\n", jscr->arosscreen->FirstWindow);
    }
    ReleaseSemaphore(&sem_janus_window_list);

    /* wait until it is closed */
    closed=FALSE;
    while(!closed) {
      ObtainSemaphore(&sem_janus_window_list);
      list_win= g_slist_find_custom(janus_windows, 
                                         (gconstpointer) jscr->arosscreen->FirstWindow,
					 &aros_window_compare);
      ReleaseSemaphore(&sem_janus_window_list);
      if(!list_win) {
	closed=TRUE;
      }
      else {
	JWLOG("wait until window task is dead\n");
	Delay(10); /* do not busy wait */
      }
    }

    JWLOG("aros_cscr_thread[%lx]: window of screen %lx is closed\n",thread, jscr->arosscreen);
  }

  if(jscr->arosscreen) {
    JWLOG("aros_cscr_thread[%lx]: close aros screen %lx\n",thread, jscr->arosscreen);
    CloseScreen(jscr->arosscreen);
    jscr->arosscreen=NULL;
  }

  if(jscr->name) {
    FreeVec(jscr->name);
    jscr->name=NULL;
  }

  if(list_screen) {
    JWLOG("aros_cscr_thread[%lx]: remove %lx from janus_screens\n",thread, list_screen);
    janus_screens=g_slist_delete_link(janus_screens, list_screen);
    list_screen=NULL;
  }

  if(jscr) {
    JWLOG("aros_cscr_thread[%lx]: FreeVec(%lx)\n",thread, jscr);
    FreeVec(jscr);
  }

  uae_main_window_closed=TRUE;

  ReleaseSemaphore(&sem_janus_screen_list);

  JWLOG("aros_cscr_thread[%lx]: dies..\n", thread);
}

int aros_custom_screen_start_thread (JanusScreen *screen) {

    JWLOG("aros_screen_start_thread(%lx)\n",screen);

    screen->name=AllocVec(8+strlen(CUSTOM_SCREEN_TASK_PREFIX_NAME)+1,MEMF_CLEAR);

    sprintf(screen->name,"%s%lx", CUSTOM_SCREEN_TASK_PREFIX_NAME, screen->aos3screen);

    ObtainSemaphore(&aos3_thread_start);
    screen->task = (struct Task *)
	    myCreateNewProcTags ( NP_Output, Output (),
				  NP_Input, Input (),
				  NP_Name, (ULONG) screen->name,
				  NP_CloseOutput, FALSE,
				  NP_CloseInput, FALSE,
				  NP_StackSize, 4096,
				  NP_Priority, 0,
				  NP_Entry, (ULONG) aros_custom_screen_thread,
				  TAG_DONE);

    ReleaseSemaphore(&aos3_thread_start);

    JWLOG("screen thread %lx created\n",screen->task);

    return screen->task != 0;
}

#if 0
extern void uae_set_thread_priority (int pri)
{
    SetTaskPri (FindTask (NULL), pri);
}
#endif

