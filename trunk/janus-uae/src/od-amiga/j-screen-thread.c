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

#include "j.h"
#include "memory.h"


#if 0
/* we don't know those values, but we always keep the last value here */
static UWORD estimated_border_top=25;
static UWORD estimated_border_bottom=25;
static UWORD estimated_border_left=25;
static UWORD estimated_border_right=25;

static void handle_msg(struct Window *win, ULONG class, UWORD code, int dmx, int dmy, WORD mx, WORD my, int qualifier, struct Process *thread, BOOL *done) {

  GSList *list_win;

  switch (class) {
      
    case IDCMP_CLOSEWINDOW:
    /* TODO: NO!!!!!!!!!!!!!!!! */
    /* fake IDCMP_CLOSEWINDOW to original aos3 window!! TODO!! */
	JWLOG("aros_scr_thread[%lx]: IDCMP_CLOSEWINDOW received\n", thread);
	break;

    case IDCMP_RAWKEY: {
	int keycode = code & 127;
	int state   = code & 128 ? 0 : 1;
	int ievent;

	if ((qualifier & IEQUALIFIER_REPEAT) == 0) {
	    /* We just want key up/down events - not repeats */
	    if ((ievent = match_hotkey_sequence (keycode, state)))
	      JWLOG("TODO: handle_hotkey_event\n");
		//handle_hotkey_event (ievent, state);
	    else

	      JWLOG("call inputdevice_do_keyboard(%d,%d)\n",keycode,state);
		inputdevice_do_keyboard (keycode, state);
	}
	break;
     }

#if 0
    case IDCMP_MOUSEMOVE:
	/* dmx and dmy are relative to our window */
	//setmousestate (0, 0, dmx, 0);
	//setmousestate (0, 1, dmy, 0);
	/* mouse nr, axis, value, absolute */
	setmousestate(0, 0, win->MouseX + win->LeftEdge, 1);
	setmousestate(0, 1, win->MouseY + win->TopEdge, 1);
#endif

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
	break;
#endif

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
	ObtainSemaphore(&sem_janus_active_win);
	if(!janus_active_window) {
	  inputdevice_acquire ();
	  inputdevice_release_all_keys ();
	  reset_hotkeys ();
	}
	ObtainSemaphore(&sem_janus_window_list);
	list_win=g_slist_find_custom(janus_windows,
                               (gconstpointer) win,
                               &aros_window_compare);
	ReleaseSemaphore(&sem_janus_window_list);
	janus_active_window=(JanusWin *) list_win->data;
	JWLOG("janus_active_window=%lx\n",list_win->data);
	ReleaseSemaphore(&sem_janus_active_win);

	break;

    /* there might be a race.. ? */
    case IDCMP_INACTIVEWINDOW: {
	JanusWin *old;
	sleep(1);
	ObtainSemaphore(&sem_janus_active_win);
	ObtainSemaphore(&sem_janus_window_list);
	list_win=g_slist_find_custom(janus_windows,
                               (gconstpointer) win,
                               &aros_window_compare);
	ReleaseSemaphore(&sem_janus_window_list);
	old=(JanusWin *) list_win->data;
	if(old == janus_active_window) {
	  janus_active_window=NULL;
	  JWLOG("janus_active_window=NULL\n");
	  inputdevice_unacquire ();
	}
	ReleaseSemaphore(&sem_janus_active_win);
	break;
    }

#if 0
    case IDCMP_DISKINSERTED:
	/*printf("diskinserted(%d)\n",code);*/
	break;

    case IDCMP_DISKREMOVED:
	/*printf("diskremoved(%d)\n",code);*/
	break;

    case IDCMP_INTUITICKS:
#ifdef __amigaos4__ 
	grabTicks--;
	if (grabTicks < 0) {
	    grabTicks = GRAB_TIMEOUT;
	    #ifdef __amigaos4__ 
		if (mouseGrabbed)
		    grab_pointer (W);
	    #endif
	}
#endif
	break;
    case IDCMP_NEWSIZE:
	do_inhibit_frame ((W->Flags & WFLG_ZOOMED) ? 1 : 0);
	break;

    case IDCMP_REFRESHWINDOW:
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
	BeginRefresh (W);
	flush_block (0, currprefs.gfx_height_win - 1);
	EndRefresh (W, TRUE);
	break;
#endif
    default:
	JWLOG("aros_scr_thread[%lx]: Unknown IDCMP class %lx received\n", thread, class);
	break;
  }
}
#endif


/************************************************************************
 * new_aros_pub_screen for an aos3screen
 *
 * check if there is already a pubscreen with this name
 * - if yes, return that one
 * - if not, open new one
 ************************************************************************/
static struct Screen *new_aros_pub_screen(JanusScreen *jscreen, 
                                          uaecptr aos3screen,
					  struct Process *thread,
					  BYTE signal) {
  ULONG width;
  ULONG height;
  ULONG depth;
  LONG mode  = INVALID_ID;
  ULONG i;
  ULONG err;
  struct Screen *arosscr;
  const UBYTE preferred_depth[] = {24, 32, 16, 15, 8};
  struct screen *pub;

  JWLOG("aros_scr_thread[%lx]: screen public name: >%s<\n",thread,
                                                           jscreen->pubname);

  width =get_word(aos3screen+12);
  height=get_word(aos3screen+14);
  depth =jscreen->depth;

  //JWLOG("we want %dx%d in %d bit\n",width,height,depth);

  mode=find_rtg_mode(&width, &height, depth);
  //JWLOG("screen mode with depth %d: %lx\n",depth,mode);

  if(mode == INVALID_ID) {
    //JWLOG("try different depth:\n");
    i=0;
    while( (i<sizeof(preferred_depth)) && (mode==INVALID_ID)) {
      depth = preferred_depth[i];
      mode = find_rtg_mode (&width, &height, depth);
      i++;
    }
    //JWLOG("screen mode with depth %d: %lx\n",depth,mode);
  }

  if(mode==INVALID_ID) {
    JWLOG("aros_scr_thread[%lx]: ERROR: unable to find a screen mode !?\n",
          thread);
    return NULL;
  }

  arosscr=OpenScreenTags(NULL,
		     SA_Type, PUBLICSCREEN,
		     SA_DisplayID, mode,
		     SA_Width, width, /* necessary ? */
		     SA_Height, height,
		     SA_Title, get_real_address(get_long(aos3screen+26)),
		     SA_PubName, jscreen->pubname,
		     SA_PubTask, thread,
		     SA_PubSig, signal,
		     SA_ErrorCode, &err,
		     TAG_DONE);

  jscreen->ownscreen=TRUE; /* TODO we have to close this one, how..? */

  return arosscr;
}

/***********************************************************
 * every AOS3 screen gets its own process
 ***********************************************************/  

static void aros_screen_thread (void) {

  struct Process *thread = (struct Process *) FindTask (NULL);
  GSList         *list_screen=NULL;
  JanusScreen    *jscr=NULL;
  BOOL            done;
  BYTE            signal;
  ULONG           s;
  uaecptr aos3screen;

  /* There's a time to live .. */

  JWLOG("aros_scr_thread[%lx]: thread running \n",thread);

  ObtainSemaphore(&sem_janus_screen_list);
  list_screen=g_slist_find_custom(janus_screens, 
	 		     	  (gconstpointer) thread,
			     	  &aos3_screen_process_compare);

  ReleaseSemaphore(&sem_janus_screen_list);
  JWLOG("aros_scr_thread[%lx]: released sem_janus_window_list sem \n",thread);

  if(!list_screen) {
    JWLOG("aros_scr_thread[%lx]: ERROR: screen of this thread not found in screen list !?\n",thread);
    goto EXIT; 
  }

  jscr=(JanusScreen *) list_screen->data;
  JWLOG("aros_scr_thread[%lx]: win: %lx \n",thread,jscr);

  signal=AllocSignal(-1);
  if(!signal) {
    JWLOG("aros_scr_thread[%lx]: unable to alloc signal\n");
    goto EXIT;
  }

  aos3screen=(uaecptr) jscr->aos3screen;

  jscr->arosscreen=new_aros_pub_screen(jscr, aos3screen, thread, signal);
  if(!jscr->arosscreen) {
    JWLOG("aros_scr_thread[%lx]: ERROR: could not open screen !!\n",thread);
    goto EXIT; 
  }

  while(!done) {
    s=Wait(1L << signal | SIGBREAKF_CTRL_C);
    done=TRUE;

    /* Ctrl-C */
    if(s & SIGBREAKF_CTRL_C) {
      JWLOG("aros_scr_thread[%lx]: SIGBREAKF_CTRL_C received\n", thread);
      //done=TRUE;
    }
  }

  /* ... and a time to die. */

EXIT:
  FreeSignal(signal);
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

int aros_screen_start_thread (JanusScreen *screen) {

    JWLOG("aros_screen_start_thread(%lx)\n",screen);

    screen->name=AllocVec(8+strlen(SCREEN_TASK_PREFIX_NAME)+1,MEMF_CLEAR);

    sprintf(screen->name,"%s%lx",SCREEN_TASK_PREFIX_NAME,screen->aos3screen);

    ObtainSemaphore(&aos3_thread_start);
    screen->task = (struct Task *)
	    myCreateNewProcTags ( NP_Output, Output (),
				  NP_Input, Input (),
				  NP_Name, (ULONG) screen->name,
				  NP_CloseOutput, FALSE,
				  NP_CloseInput, FALSE,
				  NP_StackSize, 4096,
				  NP_Priority, 0,
				  NP_Entry, (ULONG) aros_screen_thread,
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

