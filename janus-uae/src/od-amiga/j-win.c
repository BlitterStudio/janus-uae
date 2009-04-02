/************************************************************************ 
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE.
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
 * along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
 *
 ************************************************************************/

#include "j.h"

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

  i=0;
  while(get_long(m68k_results+i)) {
    new_aos3window(get_long(m68k_results+i));
    i++;
  }

  return TRUE;
}

/**********************************************************
 * ad_job_update_janus_windows
 *
 * update aos3window list, so new windows are added
 * and closed windows are removed
 ***********************************************************/

/* input: array if aos3 windows, aos3window */
static int is_in_results(LONG *m68k_results, gconstpointer w) {
  int i=0;

  while(get_long((uaecptr) m68k_results+i)) {
    if(get_long((uaecptr) m68k_results+i)==w) {
      return TRUE;
    }
    i++;
  }
  return FALSE;
}

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

  JWLOG("new_aos3window(%lx)\n",aos3win);

  if(!aos3win) {
    /* OpenWindow failed !? */
    return;
  }

  ObtainSemaphore(&sem_janus_screen_list);
  ObtainSemaphore(&sem_janus_window_list);

  if(g_slist_find_custom(janus_windows, 
			  (gconstpointer) aos3win,
			  &aos3_window_compare)) {
    /* we already have this window */
    return;
  }

  /* new jwin */
  jwin=(JanusWin *) AllocVec(sizeof(JanusWin),MEMF_CLEAR);
  JWLOG("aos3window %lx as jwin %lx\n",
	   aos3win,
	   jwin);

  aos3screen=get_long(aos3win+46);
  JWLOG("search for aos3screen %lx\n",aos3screen);
  list_screen=g_slist_find_custom(janus_screens, 
	   		          (gconstpointer) aos3screen,
			          &aos3_screen_compare);

  if(!list_screen) {
    JWLOG("\n\nERROR (FIXME): This window %lx -> screen %lx does not exist!!\n\n",aos3win,aos3screen);
    /* this must not happen !! */
    FreeVec(jwin);
    ReleaseSemaphore(&sem_janus_window_list);
    ReleaseSemaphore(&sem_janus_screen_list);
    return;
  }

  jscreen=(JanusScreen *) list_screen->data;

  JWLOG("jwin %lx is on jscreen %lx\n",jwin,jscreen);

  jwin->aos3win=aos3win;
  jwin->jscreen=jscreen; 
  janus_windows=g_slist_append(janus_windows,jwin);

  ReleaseSemaphore(&sem_janus_window_list);
  ReleaseSemaphore(&sem_janus_screen_list);

  aros_win_start_thread(jwin);
}
/**********************************************************
 * ad_job_active_window
 *
 * return aos3 window, which should be active
 **********************************************************/
uae_u32 ad_job_active_window(ULONG *m68k_results) {
  uae_u32 win;

  ObtainSemaphore(&sem_janus_active_win);

  if(!janus_active_window) {
    win=0;
  }
  else {
    win=janus_active_window->aos3win;
  }

  ReleaseSemaphore(&sem_janus_active_win);

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

  JWLOG("ad_job_new_window(aos3win %lx)\n",aos3win);

  // ?? ObtainSemaphore(&sem_janus_active_win);
  new_aos3window(aos3win);
  // ?? ReleaseSemaphore(&sem_janus_active_win);

  return TRUE;
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

  /* resize/move windows according to the supplied data:
   *
   * command mem fields (LONG):
   * 0: window pointer (LONG)
   * 1: LeftEdge, TopEdge (WORD,WORD)
   * 2: Width, Height (WORD, WORD)
   * 3: reserved NULL (LONG)
   * 4: reserved NULL (LONG)
   * 5: next window starts here
   */

  JWLOG("ad_job_report_uae_windows: entered\n");
  i=0;
  while(get_long(m68k_results+i)) {
    //JWLOG("ad_job_report_uae_windows: i=%d\n",i);
    aos3win=(ULONG) get_long(m68k_results+i);
    //JWLOG("ad_job_report_uae_windows: window=%lx\n",aos3win);
    list_win=g_slist_find_custom(janus_windows, 
			          (gconstpointer) aos3win,
			          &aos3_window_compare);
    //JWLOG("ad_job_report_uae_windows: list_win=%lx\n",list_win);
    if(list_win) {
      win=list_win->data;
      if(win->delay > 0) {
	win->delay--;
      }
      else {
	JWLOG("ad_job_report_uae_windows: win->delay=WIN_DEFAULT_DELAY\n");
	window=win->aroswin;
	if(assert_window(window)) {
	  //JWLOG("ad_job_report_uae_windows: aros window %lx is alive\n", window);
	  if((win->LeftEdge != get_hi_word(m68k_results+i+1)) ||
	     (win->TopEdge  != get_lo_word(m68k_results+i+1)) ||
	     (win->Width    != get_hi_word(m68k_results+i+2)) ||
	     (win->Height   != get_lo_word(m68k_results+i+2))) { 
	    /* aos3 size changed -> we take the change */
	    win->delay=WIN_DEFAULT_DELAY;
	    JWLOG("ad_job_report_uae_windows: window %lx size/pos changed\n",window);
	    win->LeftEdge=get_hi_word(m68k_results+i+1);
	    win->TopEdge =get_lo_word(m68k_results+i+1);
	    win->Width   =get_hi_word(m68k_results+i+2);
	    win->Height  =get_lo_word(m68k_results+i+2);
	    JWLOG("ad_job_report_uae_windows: resize window %lx\n",window);
	    JWLOG("ad_job_report_uae_windows: w/h: %dx%d\n",win->Width,win->Height);
	    JWLOG("ad_job_report_uae_windows: x/y: %dx%d\n",win->LeftEdge,win->TopEdge);
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
	  }
	}
      }
    }
    i=i+5;
  }

  JWLOG("ad_job_report_uae_windows: left\n");
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

  /* 
   * command mem fields (LONG):
   * 0: window pointer (LONG)
   * 1: LeftEdge, TopEdge (WORD,WORD)
   * 2: Width, Height (WORD, WORD)
   * 3: reserved NULL (LONG)
   * 4: reserved NULL (LONG)
   * 5: next window starts here
   */

  JWLOG("ad_job_report_host_windows: entered\n");
  i=0;

  ObtainSemaphore(&sem_janus_window_list);
  list_win=janus_windows;
  while(list_win) {
    win=(JanusWin *) list_win->data;
    if(win->delay > 0) {
      win->delay--;
    }
    else {

      window=win->aroswin;

      if(assert_window(window)) {
	/* check if we have been resized */
	if((window->LeftEdge + window->BorderLeft != win->LeftEdge) ||
	   (window->TopEdge  + window->BorderTop  != win->TopEdge)  ||
	   (window->Width    - 
	    window->BorderLeft -  window->BorderRight -
	    win->plusx
						  != win->Width)    ||
	   (window->Height   - 
	    window->BorderTop  - window->BorderBottom -
	    win->plusy
						  != win->Height)) {
	  JWLOG("ad_job_report_host_windows: win->delay=WIN_DEFAULT_DELAY\n");
	  win->delay=WIN_DEFAULT_DELAY; 


	  JWLOG("ad_job_report_host_windows: window %lx was resized\n",window);
	  JWLOG("ad_job_report_host_windows: full w/h %dx%d\n",window->Width,
							    window->Height);
	  JWLOG("ad_job_report_host_windows: full x/y %dx%d\n",window->LeftEdge,
							    window->TopEdge);

	  JWLOG("ad_job_report_host_windows: old w/h %dx%d\n",win->Width,
							    win->Height);
	  JWLOG("ad_job_report_host_windows: old x/y %dx%d\n",win->LeftEdge,
							    win->TopEdge);
	  /* update our list*/
	  win->LeftEdge = window->LeftEdge + window->BorderLeft;
	  win->TopEdge  = window->TopEdge  + window->BorderTop;
	  win->Width  = window->Width  - window->BorderLeft - window->BorderRight - win->plusx;
	  win->Height = window->Height - window->BorderTop - window->BorderBottom - win->plusy;

	  JWLOG("ad_job_report_host_windows: new w/h %dx%d\n",win->Width,
							    win->Height);
	  JWLOG("ad_job_report_host_windows: new x/y %dx%d\n",win->LeftEdge,
							    win->TopEdge);

	  put_long(m68k_results+i,win->aos3win);

	  /* put it in a ULONG, make sure, gcc does not optimize it too much */
	  l=win->LeftEdge;
	  l=l*0x10000 + win->TopEdge;
	  JWLOG("ad_job_report_host_windows: long x/y %lx\n",l);
	  put_long(m68k_results+i+1,l);

	  l=win->Width;
	  l=l*0x10000 + win->Height;
	  JWLOG("ad_job_report_host_windows: long w/h %lx\n",l);
	  put_long(m68k_results+i+2,l);
	  put_long(m68k_results+i+3,NULL);
	  put_long(m68k_results+i+4,NULL);
	  i=i+5;
	}
      }
    }
    list_win=g_slist_next(list_win);
  }
  ReleaseSemaphore(&sem_janus_window_list);

  JWLOG("ad_job_report_host_windows: left\n");
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

  JWLOG("ad_job_mark_window_dead(%lx)\n",aos_window);

  ObtainSemaphore(&sem_janus_window_list);

  list_win=g_slist_find_custom(janus_windows,
                               (gconstpointer) aos_window,
                               &aos3_window_compare);
  if(!list_win) {
    JWLOG("WARNING: ad_job_mark_window_dead could not find window %lx\n",
            aos_window);
    result=FALSE;
  }
  else {
    jwin=(JanusWin *) list_win->data;
    if(jwin) {
      jwin->dead=TRUE;
      JWLOG("ad_job_mark_window_dead(%lx)\n",aos_window);
      if(jwin->task) {
	Signal(jwin->task, SIGBREAKF_CTRL_C);
      }
    }
    else {
      JWLOG("ERROR: ad_job_mark_window_dead: list_win->data == NULL??\n");
      result=FALSE;
    }
  }

  ReleaseSemaphore(&sem_janus_window_list);
  return result;
}

uae_u32 ad_job_switch_uae_window(ULONG *m68k_results) {

  if(!get_long(m68k_results)) { /* just xor status */
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
    if(get_long(m68k_results+1)) {
      uae_main_window_closed=FALSE; 
    }
    else {
      uae_main_window_closed=TRUE;
    }
  }

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

  screen=IntuitionBase->FirstScreen;

  if(!screen) {
    JWLOG("ad_job_sync_windows: screen==NULL !?!\n");
    return FALSE;
  }

  /* there might be a screen without window */
  if(!screen->FirstWindow) {
    JWLOG("screen %lx has no window\n",screen);
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
    return FALSE;
  }

  //JWLOG("frontmost layer: %lx\n",layer);

  /* no need for sem_janus_window_list semaphore access here ?? */
  i=0;
  last_window=NULL; /* a window maybe part of more than one layer */
  while(layer) {
    /* if layer belongs to one of our windows */
    //JWLOG("ad_job_sync_windows: layer %lx\n",layer);
    if(layer->Window) {
      ObtainSemaphore(&sem_janus_window_list);
      list_win= g_slist_find_custom(janus_windows,
                               (gconstpointer) layer->Window,
                               &aros_window_compare);

      if(list_win) {
	win=(JanusWin *) list_win->data;
	if(win->aos3win != last_window) {
	  last_window=win->aos3win;
	  JWLOG("ad_job_sync_windows: win %lx added at %d\n",win->aos3win,i*4);
	  if(!win->dead) { /* hmmm..?*/
	    /* put_long cares for *4 ? */
	    put_long(m68k_results+i, win->aos3win); 
	  }
	  i++;
	}
      }
      ReleaseSemaphore(&sem_janus_window_list);
    }
    layer=layer->back;
  }

  //JWLOG("ad_job_sync_windows left\n");
  return TRUE;
}
