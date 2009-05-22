/************************************************************************ 
 *
 * Janus-Daemon
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
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

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>

#include <proto/exec.h>

#include "janus-daemon.h"

//#define DUMPWIN

extern struct IntuitionBase* IntuitionBase;

struct Window *old_MoveWindowInFront_Window,
              *old_MoveWindowInFront_BehindWindow;
ULONG          old_MoveWindowInFront_Counter;

/* ATTENTION: janus-daemon.c does not check for errors, so if you 
 * generate an error here, check it there!
 */
BOOL init_sync_windows() {
  old_MoveWindowInFront_Window=NULL;
  old_MoveWindowInFront_BehindWindow=NULL;
  old_MoveWindowInFront_Counter=0;

  return TRUE;
}

/****************************************************
 * update the window list, so that new aros windows
 * and threads are created for new amigaOS windows
 *
 * we just return a list of all windows on any
 * public screen. We are not interested in custom
 * screens here. The uae side can find out, on which
 * screen the window is.
 ****************************************************/
void update_windows() {
  ULONG *command_mem;
  int    i;
  struct Window        *w;
  struct List          *public_screen_list;
  struct PubScreenNode *public_screen_node;

  //printf("update_windows()\n");
 
  public_screen_list = (struct List *) LockPubScreenList();
  public_screen_node = (struct PubScreenNode *) public_screen_list->lh_Head;
  if(!public_screen_node) {
    printf("no public_screen_node!?\n"); /* TODO */
    DebOut("no public_screen_node!?\n"); /* TODO */
    UnlockPubScreenList();
    return;
  }

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);

  i=0;
  while (public_screen_node && public_screen_node->psn_Screen) {
    w=public_screen_node->psn_Screen->FirstWindow;
    while(w) {
      DebOut("update_windows: add window #%d: %lx (screen %lx) %s\n",i,w,public_screen_node->psn_Screen,w->Title);
      command_mem[i++]=(ULONG) w;
      w=w->NextWindow;
    }
    public_screen_node=(struct PubScreenNode *)
		       public_screen_node->psn_Node.ln_Succ;
  }

  UnlockPubScreenList();

  calltrap (AD_GET_JOB, AD_GET_JOB_LIST_WINDOWS, command_mem);
  FreeVec(command_mem);

}


#ifdef DUMPWIN
/* debug helpers */
void dump_host_windows(ULONG *window) {
  int i=0;

  DebOut("dump_host_windows\n");
  while(window[i] || i<5) {
    DebOut("  %d: %lx\n",i,window[i]);
    i++;
  }
}

void dump_uae_windows(struct Screen *screen) {
  struct Layer *layer;
  int i=0;

  DebOut("dump_uae_windows\n");

  /* get frontmost layer */
  layer=screen->FirstWindow->WLayer;
  while(layer->front) {
    layer=layer->front;
  }

  if(!layer) 
    return;

  while(layer) {
    if(layer->Window) {
      DebOut("  %d: %lx\n", i, layer->Window, ((struct Window *)layer->Window)->Title);
      i++;
    }
    layer = layer -> back;
  }
}

ULONG need_to_sort(ULONG *host_window, struct Layer *layer) {

  if(!host_window[0] && !layer) {
    return FALSE;
  }

  if(!host_window[0]) { /* all host windows are in the right order */
    return FALSE;
  }

  if(host_window[0]) {
    DebOut("need_to_sort: orphan host windows..\n");
    return FALSE;
  }

  if(!layer->Window) {
    return need_to_sort(host_window, layer->back); /* skip */
  }

  if((ULONG) layer->Window != host_window[0]) {
    return TRUE;
  }

  return need_to_sort(host_window+4, layer->back);
}
#endif


/*****************************************
 * report_uae_windows
 *
 * - sync window order (top to bottom)
 * - sync window sizes
 *****************************************/

/* fill in actual windows with size etc 
 * command mem fields (LONG):
 * 0: window pointer (LONG)
 * 1: LeftEdge, TopEdge (WORD,WORD)
 * 2: Width, Height (WORD, WORD)
 * 3: reserved NULL (LONG)
 * 4: reserved NULL (LONG)
 * 5: next window starts here
 *
 * All values are without window borders.
 * */

void report_uae_windows() {
  struct Screen *screen;
  struct Window *win;
  ULONG         *command_mem;
  ULONG          i;

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);
  if(!command_mem) {
    return;
  }

  screen=(struct Screen *) LockPubScreen(NULL);

  if(!screen) {
    printf("report_uae_windows: no screen!?\n");
    DebOut("ERROR: report_uae_windows: no screen!?\n");
    FreeVec(command_mem);
    return;
  }

  win=screen->FirstWindow;

  i=0;
  while(win) {
    //printf("add window #%d: %lx\n",i,w);
    command_mem[i  ]=(ULONG) win;
    command_mem[i+1]=(ULONG) ((win->LeftEdge 
                               + win->BorderLeft) 
			       * 0x10000 +
                              (win->TopEdge + 
			       win->BorderTop));
    command_mem[i+2]=(ULONG) ((win->Width -
                               win->BorderLeft -
			       win->BorderRight )
                              * 0x10000 + 
			      (win->Height - 
			       win->BorderTop -
			       win->BorderBottom));
    command_mem[i+3]=NULL;
    command_mem[i+4]=NULL;

    win=win->NextWindow;
    i=i+5;
  }

  calltrap (AD_GET_JOB, AD_GET_JOB_REPORT_UAE_WINDOWS, command_mem);

  FreeVec(command_mem);
}


/* fill in actual windows with size etc 
 *
 * command mem fields (LONG):
 * 0: window pointer (LONG)
 * 1: LeftEdge, TopEdge (WORD,WORD)
 * 2: Width, Height (WORD, WORD)
 * 3: reserved NULL (LONG)
 * 4: reserved NULL (LONG)
 * 5: next window starts here
 *
 * All values are without window borders.
 * */

static UWORD get_hi(ULONG l) {
  ULONG t;
  t=l&0xFFFF0000;
  t=t/0x10000;
  return (UWORD) t;
}

static UWORD get_lo(ULONG l) {
  return (UWORD) (l&0xFFFF);
}

void report_host_windows() {
  struct Window *win;
  ULONG         *command_mem;
  ULONG          i;

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);
  if(!command_mem) {
    return;
  }

  calltrap (AD_GET_JOB, AD_GET_JOB_REPORT_HOST_WINDOWS, command_mem);

  i=0;
  while(command_mem[i]) {
    win=(struct Window *)command_mem[i];
    DebOut("resize window %lx (%s)\n",(ULONG) win,win->Title);

    DebOut("  x/y: %d x %d\n",get_hi(command_mem[i+1]),get_lo(command_mem[i+1]));
    DebOut("  w/h: %d x %d\n",get_hi(command_mem[i+2]),get_lo(command_mem[i+2]));

    //printf("  (long x/y: %lx)\n",command_mem[i+1]);
    //printf("  (long w/y: %lx)\n",command_mem[i+2]);
    /* TODO(?) assert_window(win);..*/
    ChangeWindowBox(win,
                    get_hi(command_mem[i+1]) - win->BorderLeft,
                    get_lo(command_mem[i+1]) - win->BorderTop,
		    get_hi(command_mem[i+2]) +
		      win->BorderLeft + win->BorderRight,
		    get_lo(command_mem[i+2]) +
		      win->BorderTop + win->BorderBottom);

    i=i+5;
  }

  /* get all changed windows */

#if 0
  screen=(struct Screen *) LockPubScreen(NULL);

  if(!screen) {
    printf("report_uae_windows: no screen!?\n");
    FreeVec(command_mem);
    return;
  }

  win=screen->FirstWindow;

  i=0;
  while(win) {
    //printf("add window #%d: %lx\n",i,w);
    command_mem[i  ]=(ULONG) win;
    command_mem[i+1]=(ULONG) ((win->LeftEdge 
                               + win->BorderLeft) 
			       * 0x10000 +
                              (win->TopEdge + 
			       win->BorderTop));
    command_mem[i+2]=(ULONG) ((win->Width -
                               win->BorderLeft -
			       win->BorderRight )
                              * 0x10000 + 
			      (win->Height - 
			       win->BorderTop -
			       win->BorderBottom));
    command_mem[i+3]=NULL;
    command_mem[i+4]=NULL;

    win=win->NextWindow;
    i=i+5;
  }
#endif


  FreeVec(command_mem);
}

/* my_MoveWindowInFrontOf(Window, BehindWindow)
 *   Window =  window to re-position in front of another window
 *   BehindWindow =  window to re-position in front of
 *
 * As MoveWindowInFrontOf does not move the window
 * at once and we might call my_MoveWindowInFrontOf for
 * one window more than once (multiple layers), we
 * just do a real MoveWindowInFront, if one of
 * the parameters is new to us.
 */

static void my_MoveWindowInFrontOf(struct Window *window, 
                                   struct Window *behindWindow) {

  //printf("my_MoveWindowInFrontOf enteren\n");
  if(window == old_MoveWindowInFront_Window &&
     behindWindow == old_MoveWindowInFront_BehindWindow &&
     old_MoveWindowInFront_Counter < 99) {
    /* nothing to do */
    old_MoveWindowInFront_Counter++;
    //printf("my_MoveWindowInFrontOf left here\n");
    return;
  }

  if(old_MoveWindowInFront_Counter) {
    DebOut("MoveWindowInFrontOf(%lx,%lx) ignored %d times\n", 
           (ULONG) old_MoveWindowInFront_Window, 
	   (ULONG) old_MoveWindowInFront_BehindWindow,
	   (int) old_MoveWindowInFront_Counter); 
    old_MoveWindowInFront_Counter=0;
  }

  old_MoveWindowInFront_Window=window;
  old_MoveWindowInFront_BehindWindow=behindWindow;
  
  DebOut("MoveWindowInFrontOf(%lx - %s, %lx - %s)\n", (ULONG) window,       window->Title,
                                                      (ULONG) behindWindow, behindWindow->Title); 
  MoveWindowInFrontOf(window,behindWindow);
  //printf("my_MoveWindowInFrontOf left\n");
}

/*****************************************
 * sync_windows
 *
 * sync window order (top to bottom)
 *
 * LockIBase was no good idea here.
 *
 * DMRequesters add a second layer with
 * the same window linked, so we need
 * to filter out double window pointer
 * here. Otherwise we get flapping windows
 *****************************************/
void sync_windows() {
  ULONG *command_mem;
  struct Layer *layer;
  struct Screen *screen;
  struct Window *win;
  ULONG  i=0;
  ULONG  done;

  //printf("sync_windows\n");


  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);
  if(!command_mem) {
    return;
  }

  /* get the AROS window order */
  calltrap (AD_GET_JOB, AD_GET_JOB_SYNC_WINDOWS, command_mem);

  if(!command_mem[0]) {
    /* no window, might be.. */
    FreeVec(command_mem);
    return;
  }

  win=(struct Window *) command_mem[0];
  screen=win->WScreen;

  if(!screen) {
    /* no screen !? */
    printf("ERROR: window %lx has no screen !?\n",(ULONG) win);
    DebOut("ERROR: window %lx has no screen !?\n",(ULONG) win);
    FreeVec(command_mem);
    return;
  }

  /* check screen of window 0 */
  if(screen != IntuitionBase->FirstScreen) {
    ScreenToFront(screen);
  }

#if 0
  screen=(struct Screen *) LockPubScreen(NULL);
  if(!screen) {
    printf("sync_windows: screen==NULL?!\n");
    FreeVec(command_mem);
    return;
  }
  UnlockPubScreen(NULL,screen); /* TODO: is it safe already? */
#endif

#ifdef DUMPWIN
  dump_uae_windows(screen);
  dump_host_windows(command_mem);
#endif

  /* get frontmost layer */
  layer=screen->FirstWindow->WLayer;
  while(layer->front) {
    layer=layer->front;
  }

  if(!layer) {
    printf("layer==NULL!?\n");
    DebOut("ERROR: layer==NULL!?\n");
    FreeVec(command_mem);
    return;
  }

  /*
   * We just swap *one* window, as it is not swapped at once
   * anyways. Next time, if it was swapped, we care for the
   * next.
   * my_MoveWindowInFrontOf ignores dupes (for some time).
   */
  i=0;
  /* my_MoveWindowInFrontOf(Window, BehindWindow)
   *   Window =  window to re-position in front of another window
   *   BehindWindow =  window to re-position in front of
   */
  done=FALSE;
  while(!done && layer && command_mem[i]) {
    //printf("  while(i: %d,%lx,%lx)\n",i,layer->back,command_mem[i]);
    if(layer -> Window) { /* Layer has a window */
      if(command_mem[i] != (ULONG) layer->Window &&
	 command_mem[i] /* && layer->Window */) {
	/* move command_mem[i] in front of this layer */
	my_MoveWindowInFrontOf((struct Window *)command_mem[i], 
			       layer->Window);
	done=TRUE;
      };
      i++;
      while(layer->back && (layer->Window == layer->back->Window)) {
	DebOut("layers with same window ignored (%lx %s)\n", layer->Window, ((struct Window *)layer->Window)->Title);
	layer = layer -> back;
      }
    }
    layer = layer -> back;
  }

  FreeVec(command_mem);
}

void sync_active_window() {
  struct Window *win;
  win=(struct Window *) calltrap (AD_GET_JOB, 
                                  AD_GET_JOB_ACTIVE_WINDOW, NULL);

  ActivateWindow(win);
}

