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
#include <clib/intuition_protos.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "td-amigaos/thread.h"
#include "hotkeys.h"

#include "j.h"
#include "memory.h"

/***********************************************************
 * event handler for custom screen windows
 *
 * based on handle_events in gfx-amigaos/ami-win.c
 ***********************************************************/  

static void handle_custom_events_W(struct Screen *screen, struct Process *thread) {

  ULONG                       signals;
  ULONG                       notify_signal;
  BOOL                        done;
  struct MsgPort             *port;
  struct ScreenNotifyMessage *msg;
  IPTR                        notify;

  JWLOG("screen: %lx\n", screen);

  port=CreateMsgPort();

  if(!port) {
    JWLOG("ERROR: no port!\n", port);
    return;
  }

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

  done=FALSE;
  while(!done) {
    JWLOG("aros_cscr_thread[%lx]: wait for signal\n", thread);

    signals = Wait(notify_signal | SIGBREAKF_CTRL_C);
    JWLOG("aros_cscr_thread[%lx]: signal reveived\n", thread);

    if(signals & SIGBREAKF_CTRL_C) {
      JWLOG("aros_cscr_thread[%lx]: SIGBREAKF_CTRL_C received\n", thread);
      done=TRUE;
      break;
    }

    if(signals & notify_signal) {

      while ((msg = (struct ScreenNotifyMessage *) GetMsg (port))) {
	JWLOG("SCREEN MESSAGE RECEIVED!!\n");
#if 0
	class     = msg->Class;
	code      = msg->Code;
	dmx       = msg->MouseX;
	dmy       = msg->MouseY;
	mx        = msg->IDCMPWindow->MouseX; // Absolute pointer coordinates
	my        = msg->IDCMPWindow->MouseY; // relative to the window
	qualifier = msg->Qualifier;

#endif
	ReplyMsg ((struct Message*)msg); 

#if 0
	switch (class) {

	    case IDCMP_REFRESHWINDOW:
	}
#endif
      }
    }
  }

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
 ************************************************************************/
static struct Screen *new_aros_custom_screen(JanusScreen *jscreen, 
                                             uaecptr aos3screen,
				  	     struct Process *thread) {
  UWORD depth;
  ULONG mode;
  UWORD width, height;
  ULONG newwidth, newheight;
  const UBYTE preferred_depth[] = {8, 15, 16, 24, 32, 0};
  ULONG i;
  ULONG error;
  static struct NewWindow NewWindowStructure = {
	0, 0, 800, 600, 0, 1,
	IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY /*| IDCMP_DISKINSERTED | IDCMP_DISKREMOVED*/
		| IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW | IDCMP_MOUSEMOVE | IDCMP_DELTAMOVE |
		IDCMP_REFRESHWINDOW,
	WFLG_SMART_REFRESH | WFLG_BACKDROP | WFLG_RMBTRAP | WFLG_NOCAREREFRESH
	 | WFLG_BORDERLESS | WFLG_ACTIVATE | WFLG_REPORTMOUSE,
	NULL, NULL, NULL, NULL, NULL, 5, 5, 800, 600,
	CUSTOMSCREEN
  };


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

  gfxvidinfo.height     =height;
  gfxvidinfo.width      =width;
  //currprefs.gfx_linedbl =0;
  //currprefs.gfx_lores   =1;
 
  S  = jscreen->arosscreen;
  CM = jscreen->arosscreen->ViewPort.ColorMap;
  RP = &jscreen->arosscreen->RastPort;

//  NewWindowStructure.Width  = jscreen->arosscreen->Width;
//  NewWindowStructure.Height = jscreen->arosscreen->Height;
  NewWindowStructure.Width  = newwidth;
  NewWindowStructure.Height = newheight;
  NewWindowStructure.Screen = jscreen->arosscreen;

  W = (void*)OpenWindow (&NewWindowStructure);
  if (!W) {
    gui_message ("Cannot open window on new custom screen !?");
    CloseScreen(jscreen->arosscreen);
    jscreen->arosscreen=NULL;
    return NULL;
  }

  JWLOG("new aros window on custom screen: %lx (%d x %d)\n", W, W->Width, W->Height);

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

  handle_custom_events_W(jscr->arosscreen, thread);

  JWLOG("aros_cscr_thread[%lx]: exit clean..\n", thread);

  /* ... and a time to die. */

EXIT:
  JWLOG("aros_cscr_thread[%lx]: EXIT\n",thread);

  ObtainSemaphore(&sem_janus_screen_list);

  if(jscr->arosscreen->FirstWindow) {
    /* our custom screen has only one window */
    JWLOG("aros_cscr_thread[%lx]: close aros window %lx\n",thread, jscr->arosscreen->FirstWindow);
    CloseWindow(jscr->arosscreen->FirstWindow);
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

