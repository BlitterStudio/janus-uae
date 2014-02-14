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
#include <proto/layers.h>
#include <proto/dos.h>

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

  ENTER

  old_MoveWindowInFront_Window=NULL;
  old_MoveWindowInFront_BehindWindow=NULL;
  old_MoveWindowInFront_Counter=0;

  LEAVE

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

  ENTER
 
  public_screen_list = (struct List *) LockPubScreenList();

  if(!public_screen_list) {
    DebOut("update_windows(): LockPubScreenList returned NULL !?\n"); 
    LEAVE
    return;
  }

  public_screen_node = (struct PubScreenNode *) public_screen_list->lh_Head;
  if(!public_screen_node) {
    printf("no public_screen_node!?\n"); /* TODO */
    DebOut("no public_screen_node!?\n"); /* TODO */
    UnlockPubScreenList();
    LEAVE
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

  ENTER
}


#ifdef DUMPWIN
/* debug helpers */
void dump_host_windows(ULONG *window) {
  int i=0;

  ENTER

  DebOut("dump_host_windows\n");
  while(window[i] || i<5) {
    DebOut("  %d: %lx\n",i,window[i]);
    i++;
  }

  LEAVE
}

void dump_uae_windows(struct Screen *screen) {
  struct Layer *layer;
  int i=0;

  ENTER
  DebOut("dump_uae_windows\n");

  /* get frontmost layer */
  layer=screen->FirstWindow->WLayer;
  while(layer->front) {
    layer=layer->front;
  }

  if(!layer) {
    LEAVE
    return;
  }

  while(layer) {
    if(layer->Window) {
      DebOut("  %d: %lx\n", i, layer->Window, ((struct Window *)layer->Window)->Title);
      i++;
    }
    layer = layer -> back;
  }

  LEAVE
}

ULONG need_to_sort(ULONG *host_window, struct Layer *layer) {

  ENTER

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

  LEAVE
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
 *
 * 0: screen
 * {
 * 0: window pointer (LONG)
 * 1: LeftEdge, TopEdge (WORD,WORD)
 * 2: Width, Height (WORD, WORD)
 * 3: reserved NULL (LONG)
 * 4: reserved NULL (LONG)
 * 5: next window starts here
 * }
 *
 * All values are without window borders.
 * */

void report_uae_windows() {
  struct Screen *screen;
  struct Window *win;
  ULONG         *command_mem;
  ULONG          i;
  char          *pubname;
  struct Screen *lock;

  ENTER

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);
  if(!command_mem) {
    LEAVE
    return;
  }

  screen=(struct Screen *) IntuitionBase->FirstScreen;

  DebOut("amigaos IntuitionBase->FirstScreen: %lx\n", screen);

  if(!screen) {
    printf("report_uae_windows: no screen!?\n");
    DebOut("ERROR: report_uae_windows: no screen!?\n");
    FreeVec(command_mem);
    LEAVE
    return;
  }

  /* Would be nice to lock the screen, but we would need to get the
   * screen name for that. Who invented this API !?
   */

  pubname=public_screen_name(screen);
  if(pubname) {
    lock=LockPubScreen((unsigned char *)pubname);
    DebOut("lock: %lx\n", lock);
  }
  win=screen->FirstWindow;

  /* we are reporting for this screen! */
  command_mem[0]=(ULONG) screen;

  i=1;
  while(win) {
    DebOut("add window #%d: %lx\n",i,win);
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
    command_mem[i+3]=0;
    command_mem[i+4]=0;

    win=win->NextWindow;
    i=i+5;
  }

  if(lock) {
    UnlockPubScreen(NULL, lock);
  }

  calltrap (AD_GET_JOB, AD_GET_JOB_REPORT_UAE_WINDOWS, command_mem);

  FreeVec(command_mem);

  LEAVE
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

/*
 * check, if at the given coordinates our window is visible 
 */
BOOL is_in_window(struct Window *win, UWORD x, UWORD y) {
  struct Layer_Info *li = &(win->WScreen->LayerInfo);
  struct Layer *layer;

  ENTER

  LockLayerInfo(li);
  layer = WhichLayer(li, x, y);
  UnlockLayerInfo(li);

  DebOut("layer: %lx\n", layer);
  if(layer == NULL) {
    LEAVE
    return FALSE;
  }
  
  if(layer->Window == win) {
    DebOut("layer->Window %lx == win %lx \n", layer->Window, win);
    LEAVE
    return TRUE;
  }

  DebOut("layer->Window %lx != win %lx \n", layer->Window, win);
  LEAVE
  return FALSE;
}

/*
 * Some AmigaOS windows don't like it, when foreigners call ChangeWindowBox on them.
 * FinalWriter Windows freeze, if you do a ChangeWindowBox and click inside.
 * I am not sure, why this happens. So we better try to move it around
 * with the mouse ;). This is only a problem, if the window is hidden, so that
 * either the dragbar or the size gadget is hidden. In this case, we have to
 * call ChangeWindowBox (and hope it has no harmful effects).
 *
 * I removed this hack again, as I think those crashes had other reasons.
 */
static void ChangeWindowBox_safe(struct Window *win, WORD left, WORD top, WORD width, WORD height) {

  ULONG lock;

#if 0
  WORD  dx, dy, dh, dw;
  UWORD gadget_x;
  UWORD gadget_y;
  BOOL  delay=FALSE;

  ENTER

  DebOut("win %lx, left %d, top %d, width %d, height %d\n", win, left, top, width, height);

  if(!window_exists(win)) {
    DebOut("win %lx does not exist !?\n", win);
    return;
  }

  dw=width  - win->Width;
  dh=height - win->Height;

  DebOut("dw: %d, dh: %d\n", dw, dh);

  if( (dw != 0) || (dh != 0) ) {
    gadget_x=win->LeftEdge + win->Width  - (win->BorderRight /2);
    gadget_y=win->TopEdge  + win->Height - (win->BorderBottom/2);

    DebOut("gadget_x: %d, gadget_y:%d\n", gadget_x, gadget_y);
    if(!is_in_window(win, gadget_x, gadget_y)) {
      DebOut("not inside window, sorry\n");
      goto ChangeWindowBox_with_API;
    }

    /* place mouse on size gadget and click */
    SetMouse(win->WScreen, gadget_x, gadget_y, IECODE_LBUTTON, TRUE, FALSE);

    /* move it to new size and release*/
    SetMouse(win->WScreen, gadget_x + dw, gadget_y + dh, IECODE_LBUTTON, FALSE, TRUE);

    delay=TRUE;
  }

  dx=left - win->LeftEdge;
  dy=top  - win->TopEdge;

  DebOut("dx: %d, dy: %d\n", dx, dy);

  if( (dx != 0) || (dy != 0) ) {
    /* this should hit the dragbar */
    gadget_x=win->LeftEdge + (win->Width     /2);
    gadget_y=win->TopEdge  + (win->BorderTop /2);

    DebOut("gadget_x: %d, gadget_y:%d\n", gadget_x, gadget_y);
    if(!is_in_window(win, gadget_x, gadget_y)) {
      DebOut("not inside window, sorry\n");
      goto ChangeWindowBox_with_API;
    }

    /* place mouse on dragbar and click*/
    SetMouse(win->WScreen, gadget_x, gadget_y, IECODE_LBUTTON, TRUE, FALSE);

    /* move it to new position and release*/
    SetMouse(win->WScreen, gadget_x + dx, gadget_y + dy, IECODE_LBUTTON, FALSE, TRUE);
  }

  LEAVE
  return;

ChangeWindowBox_with_API:
  DebOut("WARNING: calling intuition ChangeWindowBox(%lx, %d, %d, %d, %d)\n", win, left, top, width, height);
#endif

  ENTER

  DebOut("LockIBase()\n");
  lock=LockIBase(0);
  if(!assert_window(win)) {
    DebOut("window %lx does not exist anymore, ChangeWindowBox ignored\n", win);
    DebOut("UnlockIBase()\n");
    UnlockIBase(lock);
    LEAVE
    return;
  }

  DebOut("UnlockIBase()\n");
  UnlockIBase(lock);
  ChangeWindowBox(win, left, top, width, height);

  LEAVE
  return;
}

/* care for screens here ?? 
 * no, because the AROS side may only return windows on the top screen.
 */
void report_host_windows() {
  struct Window *win;
  ULONG         *command_mem;
  ULONG          i;
  WORD           left, top, width, height;

  ENTER

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);
  if(!command_mem) {
    LEAVE
    return;
  }

  calltrap (AD_GET_JOB, AD_GET_JOB_REPORT_HOST_WINDOWS, command_mem);

  i=0;
  while(command_mem[i] && (i*4 < AD__MAXMEM) ) {
    win=(struct Window *)command_mem[i];
    DebOut("report_host_windows(): resize window %lx (%s)\n",(ULONG) win,win->Title);

    DebOut("report_host_windows():   x/y: %d x %d\n",get_hi(command_mem[i+1]),get_lo(command_mem[i+1]));
    DebOut("report_host_windows():   w/h: %d x %d\n",get_hi(command_mem[i+2]),get_lo(command_mem[i+2]));

    left  =get_hi(command_mem[i+1]) - win->BorderLeft;
    top   =get_lo(command_mem[i+1]) - win->BorderTop;
    width =get_hi(command_mem[i+2]) + win->BorderLeft + win->BorderRight;
    height=get_lo(command_mem[i+2]) + win->BorderTop + win->BorderBottom;

    ChangeWindowBox_safe(win, left, top, width, height);

    i=i+5;
  }

  /* get all changed windows */

  LEAVE
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

  ULONG lock;

  ENTER
  //printf("my_MoveWindowInFrontOf enteren\n");
  if(window == old_MoveWindowInFront_Window &&
     behindWindow == old_MoveWindowInFront_BehindWindow &&
     old_MoveWindowInFront_Counter < 99) {
    /* nothing to do */
    old_MoveWindowInFront_Counter++;
    //printf("my_MoveWindowInFrontOf left here\n");
    LEAVE
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

  DebOut("LockIBase()\n");
  lock=LockIBase(0);
  
  /* we try to make sure, that nobody closes any of our windows inbetween .. */
  if(!assert_window(window) || !assert_window(behindWindow)) {
    DebOut("UnlockIBase()\n");
    UnlockIBase(lock);
    DebOut("window %lx or %lx is not available any more, MoveWindowInFrontOf ignored!\n", 
           window, behindWindow);
    goto EXIT;
  }

  /* hmm, we need to ensure, this window is not closed inbetween! 
   * But we are forced to release the lock on intuitionbase.
   *
   * We are running at a very hight priority, so we hope, nobody closes them inbetween! 
   */

  DebOut("UnlockIBase()\n");
  UnlockIBase(lock);
 
  MoveWindowInFrontOf(window,behindWindow);
  DebOut("did a MoveWindowInFrontOf(%lx - %s, %lx - %s)\n", (ULONG) window,       window->Title,
                                                            (ULONG) behindWindow, behindWindow->Title); 

EXIT:
  LEAVE

  return;
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

  ENTER

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);
  if(!command_mem) {
    LEAVE
    return;
  }

  /* get the AROS window order */
  calltrap (AD_GET_JOB, AD_GET_JOB_SYNC_WINDOWS, command_mem);

  if(!command_mem[0]) {
    /* no window, might be.. */
    FreeVec(command_mem);
    LEAVE
    return;
  }

  win=(struct Window *) command_mem[0];
  screen=win->WScreen;

  if(!screen) {
    /* no screen !? */
    printf("ERROR: window %lx has no screen !?\n",(ULONG) win);
    DebOut("ERROR: window %lx has no screen !?\n",(ULONG) win);
    FreeVec(command_mem);
    LEAVE
    return;
  }

  /* check screen of window 0 */
  if(screen != IntuitionBase->FirstScreen) {
    ScreenToFront(screen);
  }

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
    LEAVE
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
  LEAVE
}

void sync_active_window() {
  struct Window *win;
  ULONG          lock;

  ENTER

  win=(struct Window *) calltrap (AD_GET_JOB, 
                                  AD_GET_JOB_ACTIVE_WINDOW, NULL);

  DebOut("win: %lx\n", win);

  DebOut("LockIBase()\n");
  lock=LockIBase(0);

  if(!assert_window(win)) {
    DebOut("win %lx does not exist anymore\n", win);
    DebOut("UnlockIBase()\n");
    UnlockIBase(lock);
    LEAVE
    return;
  }

  DebOut("UnlockIBase()\n");
  UnlockIBase(lock);
  ActivateWindow(win);

  LEAVE
}

