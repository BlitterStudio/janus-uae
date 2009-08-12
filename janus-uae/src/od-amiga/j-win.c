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

static void new_aos3window(ULONG aos3win);

/*********************************************************
 * ad_job_update_janus_windows(aos3 window array)
 *
 * for every window in the array call new_aos3window(win)
 *
 * this is called at janus-daemon startup once
 *
 *********************************************************/
uae_u32 ad_job_update_janus_windows(ULONG *m68k_results) {
  ULONG i;

  ENTER

  i=0;
  while(get_long_p(m68k_results+i)) {
    new_aos3window(get_long_p(m68k_results+i));
    i++;
  }

  LEAVE

  return TRUE;
}

/* not used ? */
#if 0
/**********************************************************
 * ad_job_update_janus_windows
 *
 * update aos3window list, so new windows are added
 * and closed windows are removed
 ***********************************************************/

/* input: array if aos3 windows, aos3window */
static int is_in_results(ULONG *m68k_results, gconstpointer w) {
  ULONG i=0;

  while(get_long_p(m68k_results+i)) {
    if(get_long_p(m68k_results+i)==(ULONG) w) {
      return TRUE;
    }
    i++;
  }
  return FALSE;
}
#endif

/********************************************************
 * new_aos3window(aos3 window pointer)
 *
 * Add an amigaos3 window to our janus window list
 * and start the thread, which will open the according
 * aros window.
 *
 * If the amigaos3 window is already in the
 * janus_windows list, do nothing.
 ********************************************************/
static void new_aos3window(ULONG aos3win) {
  JanusWin    *jwin;
  JanusScreen *jscreen;
  ULONG        aos3screen;
  GSList      *list_screen;

  ENTER

  JWLOG("new_aos3window(%lx)\n",aos3win);

  if(!aos3win) {
    /* OpenWindow failed !? */
    LEAVE
    return;
  }

  ObtainSemaphore(&sem_janus_screen_list);
  //JWLOG("ObtainSemaphore(&sem_janus_window_list)\n");
  ObtainSemaphore(&sem_janus_window_list);

  if(g_slist_find_custom(janus_windows, 
			  (gconstpointer) aos3win,
			  &aos3_window_compare)) {
    /* we already have this window */
    JWLOG("we already have aos3 window %lx\n", aos3win);
    //JWLOG("ReleaseSemaphore(&sem_janus_window_list)\n");
    ReleaseSemaphore(&sem_janus_window_list);
    ReleaseSemaphore(&sem_janus_screen_list);
    LEAVE
    return;
  }


  aos3screen=get_long(aos3win+46);
  JWLOG("search for aos3screen %lx\n",aos3screen);
  list_screen=g_slist_find_custom(janus_screens, 
	   		          (gconstpointer) aos3screen,
			          &aos3_screen_compare);

  if(!list_screen) {
    JWLOG("\n\nERROR (FIXME): This window %lx -> screen %lx does not exist!!\n\n",aos3win,aos3screen);
    /* this must not happen !! */
    //JWLOG("ReleaseSemaphore(&sem_janus_window_list)\n");
    ReleaseSemaphore(&sem_janus_window_list);
    ReleaseSemaphore(&sem_janus_screen_list);
    LEAVE
    return;
  }

  jscreen=(JanusScreen *) list_screen->data;

  /* new jwin */
  jwin=(JanusWin *) AllocVec(sizeof(JanusWin),MEMF_CLEAR);
  JWLOG("aos3window %lx as jwin %lx\n",
	   aos3win,
	   jwin);

  JWLOG("jwin %lx is on jscreen %lx\n",jwin,jscreen);

  jwin->aos3win=(gpointer) aos3win;
  jwin->jscreen=jscreen; 
  jwin->mempool=CreatePool(MEMF_CLEAR|MEMF_SEM_PROTECTED, 
                           0xC000, 0x8000); /* hmm..*/
  janus_windows=g_slist_append(janus_windows,jwin);

  if(aos3screen_is_custom(jscreen->aos3screen)) {
    jwin->custom=TRUE;
    JWLOG("aos3window %lx is on a custom aos3 screen (%lx)\n", jwin->aos3win, jscreen->aos3screen);
  }

  //JWLOG("ReleaseSemaphore(&sem_janus_window_list)\n");
  ReleaseSemaphore(&sem_janus_window_list);
  ReleaseSemaphore(&sem_janus_screen_list);

  aros_win_start_thread(jwin);

  LEAVE
}
/**********************************************************
 * ad_job_active_window
 *
 * return aos3 window, which should be active
 **********************************************************/
uae_u32 ad_job_active_window(ULONG *m68k_results) {
  uae_u32 win;

  ENTER

  ObtainSemaphore(&sem_janus_active_win);

  if(!janus_active_window) {
    win=0;
  }
  else {
    win=(uae_u32) janus_active_window->aos3win;
  }

  ReleaseSemaphore(&sem_janus_active_win);

  LEAVE

  return win;
}

/**********************************************************
 * ad_job_new_window
 *
 * janus-daemon openWindow patch tells us, that a new
 * window has been opened.
 *
 **********************************************************/
uae_u32 ad_job_new_window(ULONG aos3win) {
  uae_u32 win;

  ENTER

  JWLOG("ad_job_new_window(aos3win %lx)\n",aos3win);

  // ?? ObtainSemaphore(&sem_janus_active_win);
  new_aos3window(aos3win);
  // ?? ReleaseSemaphore(&sem_janus_active_win);

  LEAVE

  return TRUE;
}

/***************************************************
 * fix_orphan_windows
 *
 * we walk through our window list and see, if we
 * have any windows left on AROS, which are not
 * present in amigaOS any more. This can happen,
 * as for example workbench does not call any
 * close function for the workbench "About"
 * requester (?). Whoever did this in amigaOS 3
 * should be .. &§Q$ !!
 *
 * To kill an aros window, please have a look at
 * ad_job_mark_window_dead, there you can see,
 * what is necessary to kill a window. 
 *
 * m68k_results has an amigaOS3 window pointer at
 * every 5th position, see 
 * ad_job_report_host_windows
 ***************************************************/
static BOOL is_orphan_window(ULONG *m68k_results, ULONG win) {
  ULONG i;

  //ENTER

  i=0;
  while(get_long_p(m68k_results+i)) {
    if(win == get_long_p(m68k_results+i)) {
      return FALSE;
    }
    i=i+5;
  }

  //LEAVE
  return TRUE;
}

static void fix_orphan_windows(ULONG *m68k_results) {
  struct Window  *aroswin;
  JanusWin       *win;
  ULONG           aos3win;
  GSList         *list_win;

  ENTER

  //JWLOG("ObtainSemaphore(sem_janus_window_list)\n");
  ObtainSemaphore(&sem_janus_window_list);
  list_win=janus_windows;
  while(list_win) {
    win=(JanusWin *) list_win->data;
    aos3win=(ULONG) win->aos3win;
    aroswin=win->aroswin;
    if(is_orphan_window(m68k_results, aos3win) && !win->dead) {
      JWLOG("  found orphan window: januswin %lx (aos3win %lx aroswin %lx)\n", list_win, aos3win, aroswin);
      win->dead=TRUE;
      if(win->task) {
	Signal(win->task, SIGBREAKF_CTRL_C);
      }
    }
    list_win=g_slist_next(list_win);
  }

  //JWLOG("ReleaseSemaphore(sem_janus_window_list)\n");
  ReleaseSemaphore(&sem_janus_window_list);
  LEAVE
}

/***************************************************
 * ad_job_report_uae_windows
 *
 * we get a list of all janus windows with x/y w/h
 * we resize our windows, if size changed and we
 * store the new size
 *
 * ad_job_report_uae_windows gets active, if an aos3
 * window has been resized by its owner task in
 * aos3. It updates the according AROS window.
 ***************************************************/

uae_u32 ad_job_report_uae_windows(ULONG *m68k_results) {

  struct Layer  *layer;
  struct Window *window;
  struct Window *last_window;
  struct Screen *screen;
  GSList        *list_win;
  JanusWin       *win;
  ULONG          aos3win;
  ULONG          i;
  BOOL           need_resize;
  UWORD          raw_w, raw_h;
  UWORD          raw_x, raw_y;

  ENTER

  /* resize/move windows according to the supplied data:
   *
   * command mem fields (LONG):
   * 0: window pointer (LONG)
   * 1: LeftEdge, TopEdge (WORD,WORD) (raw values)
   * 2: Width, Height (WORD, WORD) (raw values)
   * 3: reserved NULL (LONG)
   * 4: reserved NULL (LONG)
   * 5: next window starts here
   */

  i=0;
  while(get_long_p(m68k_results+i)) {
    aos3win=(ULONG) get_long_p(m68k_results+i);
    list_win=g_slist_find_custom(janus_windows, 
			          (gconstpointer) aos3win,
			          &aos3_window_compare);

    /* quite some sanity checks here */
    if(!list_win) {
      JWLOG("no Janus window found for aos3win %lx !?\n",aos3win);
      goto NEXT;
    }

    win=list_win->data;
    if(!win) {
      JWLOG("list_win %lx has no data??\n", list_win);
      goto NEXT;
    }

    window=win->aroswin;
    if(!window) {
      JWLOG("no AROS window found for aos3win %lx\n",aos3win);
      goto NEXT;
    }

    JWLOG("check window %lx (%s)\n", window, window->Title);

    if(win->custom) {
      JWLOG("aos3window %lx is on a custom screen\n", aos3win);
      goto NEXT;
    }

    if(win->delay > 0) {
      win->delay--;
      goto NEXT;
    }

    if(!assert_window(window)) {
      JWLOG(" window assert failed\n");
      goto NEXT;
    }
   
    need_resize=FALSE;
    if((win->LeftEdge != get_hi_word(m68k_results+i+1)) ||
       (win->TopEdge  != get_lo_word(m68k_results+i+1))) {

      JWLOG(" window has beed moved\n");
      win->LeftEdge = get_hi_word(m68k_results+i+1); /* both are raw */
      win->TopEdge  = get_lo_word(m68k_results+i+1);

      need_resize=TRUE;
    }

    if((win->Width  != get_hi_word(m68k_results+i+2)) ||
       (win->Height != get_lo_word(m68k_results+i+2))) { 
      /* aos3 size changed -> we take the change */
      JWLOG(" window has been resized\n",window);
      JWLOG("  aos3win->Width:  %3d\n",get_hi_word(m68k_results+i+2));
      JWLOG("  win->Width:      %3d\n",win->Width);
      JWLOG("  aroswin->Width:  %3d (raw)\n",window->Width-
                                             window->BorderLeft -
					     window->BorderRight);
      JWLOG("\n");

      JWLOG("  aos3win->Height: %3d\n",get_lo_word(m68k_results+i+2));
      JWLOG("  win->Height:     %3d\n",win->Height);
      JWLOG("  aroswin->Height  %3d (raw)\n",window->Height -
                                             window->BorderTop -
					     window->BorderBottom);
      win->Width   =get_hi_word(m68k_results+i+2);
      win->Height  =get_lo_word(m68k_results+i+2);

      need_resize=TRUE;
    }

    if(need_resize) {
      JWLOG(" change window %lx\n",window);
      win->delay=WIN_DEFAULT_DELAY * 4;  /* delay is set to 0, if
					  * IDCMP_CHANGE is received
					  * and delay > WIN_DEFAULT_DELAY
					  */

      /* You can detect that this operation has completed by receiving
       * the IDCMP_CHANGEWINDOW IDCMP message  .. TODO?*/
      ChangeWindowBox(window,
		      win->LeftEdge - window->BorderLeft, 
		      win->TopEdge - window->BorderTop,
		      win->Width + 
		      window->BorderLeft + 
		      window->BorderRight +
		      win->plusx,
		      win->Height + 
		      window->BorderTop + 
		      window->BorderBottom +
		      win->plusy);
      JWLOG("  aroswin->Width:  %3d (new raw)\n",window->Width-
                                             window->BorderLeft -
					     window->BorderRight);
      JWLOG("  aroswin->Height  %3d (new raw)\n",window->Height -
                                             window->BorderTop -
					     window->BorderBottom);
      JWLOG("  win->LeftEdge: %d\n", win->LeftEdge);
      JWLOG("  window->BorderLeft: %d\n", window->BorderLeft);
      JWLOG("  win->LeftEdge - window->BorderLeft: %d\n", win->LeftEdge - window->BorderLeft);



    }
NEXT:
    i=i+5;
  }


  fix_orphan_windows(m68k_results);

  LEAVE

  return TRUE;
}

/***************************************************
 * ad_job_report_host_windows
 *
 * ad_job_report_host_windows gets active, if an aros
 * window has been resized by the aros user
 * It creates a list with all windows to be changed
 * in aos3.
 *
 * ad_job_report_host_windows gets called *after*
 * ad_job_report_uae_windows, so automatic changes
 * win over user changes.
 ***************************************************/
uae_u32 ad_job_report_host_windows(ULONG *m68k_results) {

  struct Layer  *layer;
  struct Window *window;
  struct Window *last_window;
  struct Screen *screen;
  GSList        *list_win;
  JanusWin       *win;
  ULONG          aos3win;
  ULONG          i;
  ULONG          l;
  UWORD          raw_w, raw_h;
  BOOL           need_resize;

  ENTER

  /* 
   * command mem fields (LONG):
   * 0: window pointer (LONG)
   * 1: LeftEdge, TopEdge (WORD,WORD)
   * 2: Width, Height (WORD, WORD)
   * 3: reserved NULL (LONG)
   * 4: reserved NULL (LONG)
   * 5: next window starts here
   */

  i=0;

  //JWLOG("ObtainSemaphore(sem_janus_window_list)\n");
  ObtainSemaphore(&sem_janus_window_list);
  list_win=janus_windows;
  while(list_win) {
    win=(JanusWin *) list_win->data;
    window=win->aroswin;

    if(!assert_window(window)){
      goto NEXT;
    }

    if(win->delay > 0) {
      win->delay--;
      goto NEXT;
    }

    need_resize=FALSE;

    JWLOG("check aros window %lx (%s)\n", window, window->Title);

    /* check if we have been moved */
    if((window->LeftEdge + window->BorderLeft != win->LeftEdge) ||
       (window->TopEdge  + window->BorderTop  != win->TopEdge)) {
      JWLOG("window %lx has been moved:\n",window);
      JWLOG(" window->LeftEdge: %d\n",window->LeftEdge);
      JWLOG("  + BorderLeft:    %d\n",window->LeftEdge + 
                                      window->BorderLeft);
      JWLOG(" os3win->LeftEdge: %d\n",win->LeftEdge);
      JWLOG(" window->TopEdge:  %d\n",window->TopEdge);
      JWLOG("  + BorderTop:     %d\n",window->TopEdge+window->BorderTop);
      JWLOG(" os3win->TopEdge:  %d\n",win->TopEdge);

      win->LeftEdge = window->LeftEdge + window->BorderLeft;
      win->TopEdge  = window->TopEdge  + window->BorderTop;

      JWLOG(" os3 win %lx should become: x:%3d y:%3d (raw)\n",
                                       win->aos3win,
                                       win->LeftEdge,
				       win->TopEdge);

      need_resize=TRUE;
    }

    /* check if we have been resized */
    raw_w=window->Width - 
          window->BorderLeft - window->BorderRight -
	  win->plusx;
    raw_h=window->Height -
          window->BorderTop - window->BorderBottom -
	  win->plusy;

    if (raw_w != win->Width || raw_h!=win->Height) {

      JWLOG("window %lx has been resized:\n",window);
      JWLOG(" window->Width x Height %3d x %3d (full)\n",window->Width, 
                                                  window->Height);
      JWLOG(" window->Width x Height %3d x %3d (raw)\n", raw_w, raw_h);

      JWLOG(" os3win->Width x Height %3d x %3d\n",win->Width,
					  	  win->Height);

      /* update our list*/
      win->Width  = raw_w;
      win->Height = raw_h;

      JWLOG(" os3 win %lx should become: %3d x %3d (raw)\n",
                                                      win->aos3win,
                                                      win->Width,
						      win->Height);
      need_resize=TRUE;
    }

    if(need_resize) {
      win->delay=WIN_DEFAULT_DELAY; 
      put_long_p(m68k_results+i,(ULONG) win->aos3win);

      /* put it in a ULONG, make sure, gcc does not 
       * optimize it too much */
      l=win->LeftEdge;
      l=(l*0x10000) + win->TopEdge;
      //JWLOG("report long x/y %lx\n",l);
      put_long_p(m68k_results+i+1,l);

      l=win->Width;
      l=l*0x10000 + win->Height;
      //JWLOG("report long w/h %lx\n",l);
      put_long_p(m68k_results+i+2, l);
      put_long_p(m68k_results+i+3, 0);
      put_long_p(m68k_results+i+4, 0);
      i=i+5;
    }
NEXT:
    list_win=g_slist_next(list_win);
  }

  //JWLOG("ReleaseSemaphore(sem_janus_window_list)\n");
  ReleaseSemaphore(&sem_janus_window_list);

  LEAVE

  return TRUE;
}

/***************************************************
 * ad_job_mark_window_dead
 *
 * this gets called by the patch to CloseWindow
 * just before the real CloseWindow gets called.
 * So from now on, we must not touch the window
 * contents/structues any more!
 * We cannot close that window at once, there
 * might be a race condition, which reopens
 * the window before it is really closed.
 * So we mark it DEAD!
 ***************************************************/
uae_u32 ad_job_mark_window_dead(ULONG aos_window) {
  GSList        *list_win;
  JanusWin      *jwin;
  uae_u32        result=TRUE;

  ENTER

  JWLOG("ad_job_mark_window_dead(%lx)\n",aos_window);

  //JWLOG("ObtainSemaphore(&sem_janus_window_list)\n");
  ObtainSemaphore(&sem_janus_window_list);

  list_win=g_slist_find_custom(janus_windows,
                               (gconstpointer) aos_window,
                               &aos3_window_compare);
  if(!list_win) {
    JWLOG("WARNING: ad_job_mark_window_dead could not find aos3 window %lx\n",
            aos_window);
    /* this is ok, orphan window check might have killed it already */
    result=TRUE;
  }
  else {
    jwin=(JanusWin *) list_win->data;
    if(jwin) {
      jwin->dead=TRUE;
      if(jwin->aroswin) {
	JWLOG("ad_job_mark_window_dead(%lx) (%s)\n", aos_window, jwin->aroswin->Title);
      }
      else {
	JWLOG("ad_job_mark_window_dead(%lx)\n", aos_window);
      }
      if(jwin->task) {
	Signal(jwin->task, SIGBREAKF_CTRL_C);
      }
    }
    else {
      JWLOG("ERROR: ad_job_mark_window_dead: list_win->data == NULL??\n");
      result=FALSE;
    }
  }

  //JWLOG("ReleaseSemaphore(&sem_janus_window_list)\n");
  ReleaseSemaphore(&sem_janus_window_list);

  LEAVE

  return result;
}


uae_u32 ad_job_switch_uae_window(ULONG *m68k_results) {

  ENTER

  if(!get_long_p(m68k_results)) { /* just xor status */
    if(uae_main_window_closed) {
      JWLOG("ad_job_switch_uae_window: open window\n");
      uae_main_window_closed=FALSE;
    }
    else {
      JWLOG("ad_job_switch_uae_window: close window\n");
      uae_main_window_closed=TRUE;
    }
  }
  else {
    if(get_long_p(m68k_results+1)) {
      uae_main_window_closed=FALSE; 
    }
    else {
      uae_main_window_closed=TRUE;
    }
  }

  LEAVE

  return TRUE;
}

/***************************************************
 * ad_job_sync_windows
 *
 * we have to return the order of all janus windows
 * from top window to bottom window
 ***************************************************/
uae_u32 ad_job_sync_windows(ULONG *m68k_results) {

  struct Layer  *layer;
  struct Window *window;
  struct Window *last_window;
  struct Screen *screen;
  GSList        *list_win;
  JanusWin      *win;
  ULONG          i;

  ENTER

  screen=IntuitionBase->FirstScreen;

  if(!screen) {
    JWLOG("ad_job_sync_windows: screen==NULL !?!\n");
    LEAVE
    return FALSE;
  }

  /* there might be a screen without window */
  if(!screen->FirstWindow) {
    JWLOG("screen %lx has no window\n",screen);
    LEAVE
    return FALSE;
  }

  //JWLOG("get frontmost layer..\n");
  /* get frontmost layer */
  layer=screen->FirstWindow->WLayer;
  while(layer->front) {
    layer=layer->front;
  }

  if(!layer) {
    JWLOG("ad_job_sync_windows: layer==NULL!?\n");
    LEAVE
    return FALSE;
  }

  //JWLOG("frontmost layer: %lx\n",layer);

  /* no need for sem_janus_window_list semaphore access here ?? */
  i=0;
  last_window=NULL; /* a window maybe part of more than one layer */
  //JWLOG("ObtainSemaphore(&sem_janus_window_list)\n");
  ObtainSemaphore(&sem_janus_window_list);
  while(layer) {
    /* if layer belongs to one of our windows */
    //JWLOG("ad_job_sync_windows: layer %lx\n",layer);
    if(layer->Window) {
      list_win= g_slist_find_custom(janus_windows,
                               (gconstpointer) layer->Window,
                               &aros_window_compare);

      if(list_win) {
	win=(JanusWin *) list_win->data;
	if(win->aos3win != last_window) {
	  last_window=win->aos3win;
	  //JWLOG("ad_job_sync_windows: win %lx added at %d\n",win->aos3win,i*4);
	  if(!win->custom && !win->dead) { /* hmmm..?*/
	    /* put_long cares for *4 ? */
	    put_long_p(m68k_results+i, (ULONG) win->aos3win); 
	  }
	  i++;
	}
      }
    }
    layer=layer->back;
  }
  //JWLOG("ReleaseSemaphore(&sem_janus_window_list)\n");
  ReleaseSemaphore(&sem_janus_window_list);

  LEAVE

  return TRUE;
}

/***********************************************************
 * close_all_janus_windows()
 *
 * send a CTRL-C to all aros janus tasks, so that they
 * close their window and free their resources.
 ***********************************************************/
void close_all_janus_windows() {
  GSList   *list_win;
  JanusWin *win;

  ENTER

  if(!aos3_task) {
    /* never registered, nothing to close */
    LEAVE
    return;
  }

  JWLOG("close_all_janus_windows\n");

  ObtainSemaphore(&sem_janus_window_list);
  list_win=janus_windows;
  while(list_win) {
    win=(JanusWin *) list_win->data;
    JWLOG("  send CLTR_C to task %lx\n",win->task);
    if(win->task) {
      Signal(win->task, SIGBREAKF_CTRL_C);
    }
    list_win=g_slist_next(list_win);
  }
  ReleaseSemaphore(&sem_janus_window_list);
  LEAVE
}


/* not used ATM ? */
#if 0
/***********************************************************
 * uae_main_window_close()
 *
 * - stop all writes to uae main window
 * - close uae main window
 ***********************************************************/
void uae_main_window_close() {

  JWLOG("uae_main_window_close");
  
  if(uae_main_window_closed) {
    JWLOG("window is already closed\n");
    return;
  }

  uae_main_window_closed=TRUE;

  return;
}

/***********************************************************
 * uae_main_window_open()
 *
 * - open uae main window again
 * - enable all writes to uae main window again
 ***********************************************************/
void uae_main_window_open() {

  JWLOG("uae_main_window_open");
  
  if(!uae_main_window_closed) {
    JWLOG("window is already open\n");
    return;
  }

  uae_main_window_closed=FALSE;

  return;
}
#endif


