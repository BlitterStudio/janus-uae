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
 ***********************************************************/
static void handle_msg(JanusScreen *jscreen, 
                       ULONG class, ULONG code, ULONG userdata,
		       struct Process *thread) {

  JWLOG("aros_cscr_thread[%lx]: class %d, code %d, userdata %lx\n", thread, class, code, userdata);

  /* SDEPTH_TOFRONT/BACK is handled analog to IDCMP_ACTIVEWINDOW/INACTIVE */
  switch(code) {
    case SDEPTH_TOFRONT: 
      /* TODO !!*/
      uae_main_window_closed=FALSE;

      reset_drawing(); /* this will bring a custom screen, with keyboard working */

      JWLOG("aros_cscr_thread[%lx]: wait for sem_janus_active_custom_screen\n", thread);
      ObtainSemaphore(&sem_janus_active_custom_screen);
      janus_active_screen=jscreen;
      JWLOG("aros_cscr_thread[%lx]: we (jscreen %lx) are active front screen now\n", thread, janus_active_screen);
      ReleaseSemaphore(&sem_janus_active_custom_screen);


      ActivateWindow(jscreen->arosscreen->FirstWindow);

      break;

    case SDEPTH_TOBACK: 
      JWLOG("aros_cscr_thread[%lx]: wait for sem_janus_active_custom_screen\n", thread);

      Delay(10);
      ObtainSemaphore(&sem_janus_active_custom_screen);

      /* no other janus custom screen claimed to be active */
      if(janus_active_screen == jscreen) {
	janus_active_screen=NULL;
	uae_main_window_closed=TRUE;
	reset_drawing();
	JWLOG("aros_cscr_thread[%lx]: we (jscreen %lx) are not active any more\n", thread, janus_active_screen);
      }

      ReleaseSemaphore(&sem_janus_active_custom_screen);
      break;

  }
}

void handle_input(struct Window *win, JanusWin *jwin, ULONG class, UWORD code, int qualifier, struct Process *thread) ;

/***********************************************************
 * event handler for custom screen
 ***********************************************************/  
static void handle_custom_events_S(JanusScreen *jscreen, struct Process *thread) {

  struct Screen              *screen;
  ULONG                       signals;
  ULONG                       notify_signal;
  ULONG                       window_signal;
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
  JWLOG("notify: %lx\n", notify);

  notify_signal=1L << port->mp_SigBit;
  JWLOG("notify_signal: %d\n", notify_signal);
  JWLOG("jscreen->arosscreen: %lx\n", jscreen->arosscreen);
  JWLOG("jscreen->arosscreen->FirstWindow: %lx\n", jscreen->arosscreen->FirstWindow);
  window_signal=1L << jscreen->arosscreen->FirstWindow->UserPort->mp_SigBit;

  JWLOG("while..\n");
  done=FALSE;
  while(!done) {
    JWLOG("aros_cscr_thread[%lx]: wait for signal\n", thread);

    signals = Wait(notify_signal | window_signal | SIGBREAKF_CTRL_C);
    JWLOG("aros_cscr_thread[%lx]: signal reveived\n", thread);

    if(signals & SIGBREAKF_CTRL_C) {
      JWLOG("aros_cscr_thread[%lx]: SIGBREAKF_CTRL_C received\n", thread);
      done=TRUE;
      break;
    }

    /* notify */
    if(!done && (signals & notify_signal)) {

      while((notify_msg = (struct ScreenNotifyMessage *) GetMsg (port))) {
	notify_class     = notify_msg->snm_Class;
	notify_code      = notify_msg->snm_Code;
	notify_object    = notify_msg->snm_Object;
	notify_userdata  = notify_msg->snm_UserData;

	ReplyMsg ((struct Message*) notify_msg); 

	if(notify_object == (ULONG) screen) {
	  /* this message is for us */
	  switch (notify_class) {
	      case SNOTIFY_SCREENDEPTH:
	    	handle_msg(jscreen, notify_class, notify_code, notify_userdata, thread);
  	      break;
  	  }
	}
      }
    }

#if 0
    /* window IDCMP */
    if((!done) && (signals & window_signal)) {
      
      JWLOG("signals & window_signal ..\n");

      while(intui_msg = (struct IntuiMessage *) GetMsg (jscreen->arosscreen->FirstWindow->UserPort)) {
	intui_class     = intui_msg->Class;
	intui_code      = intui_msg->Code;
       	intui_qualifier = intui_msg->Qualifier;

	JWLOG("send IntuiMsg to handle_input..\n");

	handle_input(NULL, &dummywin, intui_class, intui_code, intui_qualifier, thread);

	ReplyMsg ((struct Message*) intui_msg); 
      }
    }
#endif

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

  width =get_word(aos3screen+12);
  height=get_word(aos3screen+14);
  JWLOG("w: %d h: %d\n", width, height);

#warning remove me <======================================================================
  width=720;
  height=568;
  JWLOG("w: %d h: %d\n", width, height);
#warning remove me <======================================================================
 
  currprefs.gfx_width_win=width;
  currprefs.gfx_height = height;
  currprefs.amiga_screen_type=0 /*UAESCREENTYPE_CUSTOM*/;

  /* this would be the right approach, but getting this pointer stuff
   * working in 68k adress space is a pain. 
   * screen->vp->RasInfo->BitMap->Depth ... */

  /* open a screen, which matches best */
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
    return NULL;
  }

  JWLOG("found modeid: %lx\n", mode);
  JWLOG("found w: %d h: %d\n", newwidth, newheight);

  /* If the screen is larger than requested, center UAE's display */
  XOffset=0;
  YOffset=0;
  if (newwidth > (ULONG)width) {
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
  uae_main_window_closed=FALSE;

  reset_drawing(); /* this will bring a custon screen, with keyboard working */

#if 0
  j_stop_window_update=TRUE;
  custom_screen_active=jscreen;

  gfx_set_picasso_state(0);
  reset_drawing();
  init_row_map();
  init_aspect_maps();
  notice_screen_contents_lost();
  inputdevice_acquire ();
  inputdevice_release_all_keys ();
  reset_hotkeys ();
  hide_pointer (W);
#endif

#if 0
  JWLOG("Line: %lx\n", Line);
  JWLOG("BitMap: %lx\n", BitMap);
  JWLOG("TempRPort: %lx\n", TempRPort);

  if(!Line || !BitMap || !TempRPort) {
    JWLOG("ERROR ERROR ERROR ERROR: NULL pointer in Line/BitMap/TempRPort!!\n");
    return FALSE; /* ? */
  }
#endif

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
  JanusScreen    *jwin=NULL;
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

  JWLOG("aros_cscr_thread[%lx]: restore S to %lx, W to %lx, CM to %lx and RP to %lx\n",thread,
                                original_S, original_W, original_CM, original_RP);
  S= original_S;
  W= original_W;
  CM=original_CM;
  RP=original_RP;

  ObtainSemaphore(&sem_janus_active_custom_screen);
  if(janus_active_screen == jscr) {
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

    JWLOG("aros_cscr_thread[%lx]: closed aros window %lx\n",thread, jscr->arosscreen->FirstWindow);
    /* restore pointer to original window, there still might be a race condition here ? */
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

