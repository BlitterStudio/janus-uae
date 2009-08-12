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

static void handle_custom_events_W(struct Window *aroswin, struct Process *thread) {

  struct IntuiMessage *msg;
  int                  dmx, dmy, mx, my, class, code, qualifier;
  ULONG                signals;
  BOOL                 done;

  JWLOG("aroswin: %lx\n", aroswin);

#if 0
  /* necessary !? */
  if(aos3_task && aos3_task_signal) {
    JWLOG("send signal to Wait of janusd (%lx)\n", aos3_task);
    uae_Signal(aos3_task, aos3_task_signal);
  }

  /* necessary !? */
  gui_handle_events();
#endif

  done=FALSE;
  while(!done) {
    JWLOG("aros_cscr_thread[%lx]: wait for signal\n", thread);
    //signals = Wait(1L << aroswin->UserPort->mp_SigBit | SIGBREAKF_CTRL_C);
    signals = Wait(SIGBREAKF_CTRL_C);
    JWLOG("aros_cscr_thread[%lx]: signal reveived\n", thread);

    if(signals & SIGBREAKF_CTRL_C) {
      JWLOG("aros_cscr_thread[%lx]: SIGBREAKF_CTRL_C received\n", thread);
      done=TRUE;
      break;
    }
#if 0

    while ((msg = (struct IntuiMessage*) GetMsg (aroswin->UserPort))) {
	class     = msg->Class;
	code      = msg->Code;
	dmx       = msg->MouseX;
	dmy       = msg->MouseY;
	mx        = msg->IDCMPWindow->MouseX; // Absolute pointer coordinates
	my        = msg->IDCMPWindow->MouseY; // relative to the window
	qualifier = msg->Qualifier;

	ReplyMsg ((struct Message*)msg); 

	switch (class) {

	    case IDCMP_REFRESHWINDOW:
#if 0
	    /* chunk2planar is slow so we define use_delta_buffer for all modes
	     * bitdepth <= 8: use_delta_buffer is 1
	     */
		if (use_delta_buffer) {
		    /* hack: this forces refresh */
		    uae_u8 *ptr = oldpixbuf;
		    int i, len = gfxvidinfo.width;
		    len *= gfxvidinfo.pixbytes;
		    for (i=0; i < currprefs.gfx_height_win; ++i) {
			ptr[00000] ^= 255;
			ptr[len-1] ^= 255;
			ptr += gfxvidinfo.rowbytes;
		    }
		}
#endif
		BeginRefresh (aroswin);
		flush_block (0, currprefs.gfx_height_win - 1);
		EndRefresh (aroswin, TRUE);
		break;

	    case IDCMP_RAWKEY: {
		int keycode = code & 127;
		int state   = code & 128 ? 0 : 1;
		int ievent;

		if ((qualifier & IEQUALIFIER_REPEAT) == 0) {
		    /* We just want key up/down events - not repeats */
		    if ((ievent = match_hotkey_sequence (keycode, state)))
			handle_hotkey_event (ievent, state);
		    else
			inputdevice_do_keyboard (keycode, state);
		}
		break;
	     }

	    case IDCMP_MOUSEMOVE:
		setmousestate (0, 0, dmx, 0);
		setmousestate (0, 1, dmy, 0);

#if 0
		if (usepub) {
		    POINTER_STATE new_state = get_pointer_state (W, mx, my);
		    if (new_state != pointer_state) {
			pointer_state = new_state;
			if (pointer_state == INSIDE_WINDOW)
			    hide_pointer (W);
			else
			    show_pointer (W);
		    }
		}
#endif
      	      break;

	    case IDCMP_MOUSEBUTTONS:
		if (code == SELECTDOWN) setmousebuttonstate (0, 0, 1);
		if (code == SELECTUP)   setmousebuttonstate (0, 0, 0);
		if (code == MIDDLEDOWN) setmousebuttonstate (0, 2, 1);
		if (code == MIDDLEUP)   setmousebuttonstate (0, 2, 0);
		if (code == MENUDOWN)   setmousebuttonstate (0, 1, 1);
		if (code == MENUUP)     setmousebuttonstate (0, 1, 0);
      	      break;

	    case IDCMP_ACTIVEWINDOW:
		/* When window regains focus (presumably after losing focus at some
		 * point) UAE needs to know any keys that have changed state in between.
		 * A simple fix is just to tell UAE that all keys have been released.
		 * This avoids keys appearing to be "stuck" down.
		 */
		JWLOG("IDCMP_ACTIVEWINDOW\n");
		inputdevice_acquire ();
		inputdevice_release_all_keys ();
		reset_hotkeys ();
		copy_clipboard_to_amigaos();
		break;

	    case IDCMP_INACTIVEWINDOW:
		JWLOG("IDCMP_INACTIVEWINDOW\n");
		JWLOG("IntuitionBase->ActiveWindow: %lx (%s)\n",IntuitionBase->ActiveWindow, IntuitionBase->ActiveWindow->Title);
		copy_clipboard_to_aros();
		inputdevice_unacquire ();
		break;

	    default:
		write_log ("Unknown event class: %x\n", class);
		JWLOG("Unknown event class: %x\n", class);
		break;
        }
    }
#endif
  }

  /* necessary !? */
  appw_events();
}

/***********************************************************
 * every AOS3 custom screen gets its own process
 ***********************************************************/  
static void aros_custom_screen_thread (void) {

  struct Process *thread = (struct Process *) FindTask (NULL);
  GSList         *list_screen=NULL;
  JanusScreen    *jscr=NULL;
  BOOL            done;
  ULONG           s;
  struct Window  *aroswin;

  /* There's a time to live .. */

  JWLOG("aros_cscr_thread[%lx]: thread running \n",thread);

  ObtainSemaphore(&sem_janus_screen_list);
  list_screen=g_slist_find_custom(janus_screens, 
	 		     	  (gconstpointer) thread,
			     	  &aos3_screen_process_compare);

  ReleaseSemaphore(&sem_janus_screen_list);
  JWLOG("aros_cscr_thread[%lx]: released sem_janus_window_list sem \n",thread);

  if(!list_screen) {
    JWLOG("aros_cscr_thread[%lx]: ERROR: screen of this thread not found in screen list !?\n",thread);
    goto EXIT; 
  }

  jscr=(JanusScreen *) list_screen->data;
  JWLOG("aros_scr_thread[%lx]: win: %lx \n",thread,jscr);

  JWLOG("aros_cscr_thread[%lx]: jscr->arosscreen: %lx\n", thread, jscr->arosscreen);
  if(!jscr->arosscreen) {
    JWLOG("aros_scr_thread[%lx]: ERROR: screen has to be already opened !!\n",thread);
    goto EXIT; 
  }

  /* our custom screen has only one window */
  aroswin=jscr->arosscreen->FirstWindow;
  if(!aroswin) {
    JWLOG("aros_cscr_thread[%lx]: ERROR: screen %lx has no window !!\n",thread, jscr->arosscreen);
  }
  JWLOG("aros_cscr_thread[%lx]: aroswin: %lx\n", thread, aroswin);

  handle_custom_events_W(aroswin, thread);

  JWLOG("aros_cscr_thread[%lx]: handle_custom_events_W returned\n", thread);

  /* ... and a time to die. */

EXIT:
  JWLOG("aros_cscr_thread[%lx]: EXIT\n");
  done=TRUE;  /* need something, if debug is off.. */
#if 0
  ObtainSemaphore(&sem_janus_window_list);

  if(aroswin) {
    JWLOG("aros_scr_thread[%lx]: close window %lx\n",thread,aroswin);
    CloseWindow(aroswin);
    jwin->aroswin=NULL;
  }

  if(list_win) {
    if(list_win->data) {
      name_mem=((JanusWin *)list_win->data)->name;
      JWLOG("aros_scr_thread[%lx]: FreeVec list_win->data \n",thread);
      FreeVec(list_win->data); 
      list_win->data=NULL;
    }
    JWLOG("aros_scr_thread[%lx]: g_slist_remove(%lx)\n",thread,list_win);
    //g_slist_remove(list_win); 
    janus_windows=g_slist_delete_link(janus_windows, list_win);
    list_win=NULL;
  }

  /* not nice to free our own name, but should not be a problem */
  if(name_mem) {
    JWLOG("aros_scr_thread[%lx]: FreeVec() >%s<\n", thread,
	    name_mem);
    FreeVec(name_mem);
  }

  ReleaseSemaphore(&sem_janus_window_list);
  JWLOG("aros_scr_thread[%lx]: dies..\n", thread);
#endif
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

