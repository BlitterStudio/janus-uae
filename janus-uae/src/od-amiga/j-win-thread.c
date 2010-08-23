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
 * $Id$
 *
 ************************************************************************/

#include <graphics/gfx.h>
#include <proto/layers.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>


#define JWTRACING_ENABLED 1
#include "j.h"
#include "memory.h"


static void handle_msg(struct Message *msg, struct Window *win, JanusWin *jwin, ULONG class, UWORD code, int dmx, int dmy, WORD mx, WORD my, int qualifier, struct Process *thread, ULONG secs, ULONG micros, BOOL *done);
static void dump_prop_gadget(struct Process *thread, ULONG gadget);

/* we don't know those values, but we always keep the last value here */
static UWORD estimated_border_top=25;
static UWORD estimated_border_bottom=25;
static UWORD estimated_border_left=25;
static UWORD estimated_border_right=25;

void setbuttonstateall (struct uae_input_device *id, struct uae_input_device2 *id2, int button, int state);

static void my_setmousebuttonstate(int mouse, int button, int state) {
  setbuttonstateall (&mice[mouse], &mice2[mouse], button, state);
}



/* we need to get a useable accurancy here */
static ULONG olisecs(ULONG s, ULONG m) {

  return s*100 + m/10000;

}

/***********************************************************
 * activate_ticks
 *
 * start automatic active refreshes
 ***********************************************************/
static void activate_ticks(JanusWin *jwin, ULONG speed) {

  /* some 10 per second arrive, refresh twice a second */
  jwin->intui_tickskip =10;    /* first refresh at once   */
  jwin->intui_tickcount=15;    /* stop after 15 refreshes */
  jwin->intui_tickspeed=speed; /* skip rate, 0 fastest   */
}

/**************************************************************************
 * force refresh of our contents from our amigaOS brother
 **************************************************************************/
static void refresh_content(JanusWin *jwin) {

  LONG start=0;
  LONG end=0;

  /* clip it */
  if(jwin->aroswin->TopEdge > 0) {
    start=jwin->aroswin->TopEdge;
  }

  end=jwin->aroswin->TopEdge + jwin->aroswin->Height + jwin->aroswin->BorderTop + jwin->aroswin->BorderBottom;
  if(end > jwin->aroswin->WScreen->Height) {
    end=jwin->aroswin->WScreen->Height-1;
  }

  /* use it */
  JWLOG("full refresh..\n");
  //DX_Invalidate(0, jwin->aroswin->WScreen->Height-1);
  DX_Invalidate(start, end);

#if 0

  JWLOG("refresh_content(%lx, first %d last %d)\n", jwin, start, end);
  clone_window(jwin->aos3win, jwin->aroswin, 0, 0xFFFFFF); /* clone window again */
#endif

}

/**************************************************************************
 * handle_input
 *
 * This handles all classes, that are also received from the window of
 * a custom screen. As the window of the custom screen is not a real
 * jwin, you have to supply a dummy jwin, which does not need to be
 * part of the window list. win is NULL, so TAKE CARE of such conditions!
 **************************************************************************/
static void handle_input(struct Window *win, JanusWin *jwin, ULONG class, UWORD code, int qualifier, struct Process *thread);
static void handle_input(struct Window *win, JanusWin *jwin, ULONG class, UWORD code, int qualifier, struct Process *thread) {

  UWORD selection;

  JWLOG("aros_win_thread[%lx]: handle_input(jwin %lx, aros win %lx, class %lx, code %d)\n", thread, 
                                                                                            jwin, win, 
											    class, code);

  switch (class) {
      
    case IDCMP_RAWKEY: {
	int keycode = code & 127;
	int state   = code & 128 ? 0 : 1;
	int ievent;

	if ((qualifier & IEQUALIFIER_REPEAT) == 0) {
	    /* We just want key up/down events - not repeats */
	    if ((ievent = match_hotkey_sequence (keycode, state)))
	      JWLOG("aros_win_thread[%lx]: TODO: handle_hotkey_event\n", thread);
		//handle_hotkey_event (ievent, state);
	    else

	      JWLOG("aros_win_thread[%lx]: call inputdevice_do_keyboard(%d,%d)\n",thread,keycode,state);
		inputdevice_do_keyboard (keycode, state);
	}
	break;
     }

    case IDCMP_MOUSEBUTTONS:

	if(IntuitionBase->ActiveWindow != win) {
	  /* it seems, that sometimes (after a menucancel?) we get clicks from
	   * other windows. Is this really correct AROS behaviour?
	   */
	  JWLOG("aros_win_thread[%lx]: IDCMP_MOUSEBUTTONS on foreign window !? (actwin%lx != mywin %lx): code %d\n", 
	         IntuitionBase->ActiveWindow, win);
	}
	else {
	  JWLOG("aros_win_thread[%lx]: IDCMP_MOUSEBUTTONS code %d\n", thread, code);
	  if (code == SELECTDOWN) setmousebuttonstate (0, 0, 1);
	  if (code == SELECTUP)   setmousebuttonstate (0, 0, 0);
	  if (code == MIDDLEDOWN) setmousebuttonstate (0, 2, 1);
	  if (code == MIDDLEUP)   setmousebuttonstate (0, 2, 0);
	  if (code == MENUDOWN)   setmousebuttonstate (0, 1, 1);
	  if (code == MENUUP)     setmousebuttonstate (0, 1, 0);
	}
	break;

    case IDCMP_ACTIVEWINDOW:
	/* When window regains focus (presumably after losing focus at some
	 * point) UAE needs to know any keys that have changed state in between.
	 * A simple fix is just to tell UAE that all keys have been released.
	 * This avoids keys appearing to be "stuck" down.
	 */

	if(win) {
	  JWLOG("aros_win_thread[%lx]: IDCMP_ACTIVEWINDOW(%lx, %s)\n", thread, win, win->Title);
	}
	inputdevice_acquire ();
	inputdevice_release_all_keys ();
	reset_hotkeys ();

	ObtainSemaphore(&sem_janus_active_win);
	if(!janus_active_window) {
	  copy_clipboard_to_amigaos();
	}
#if 0
	ObtainSemaphore(&sem_janus_window_list);
	list_win=g_slist_find_custom(janus_windows,
                               (gconstpointer) win,
                               &aros_window_compare);
	ReleaseSemaphore(&sem_janus_window_list);
	janus_active_window=(JanusWin *) list_win->data;
#endif 
	janus_active_window=jwin;
	JWLOG("aros_win_thread[%lx]: janus_active_window=%lx\n", thread, janus_active_window);
	ReleaseSemaphore(&sem_janus_active_win);

	break;

    /* there might be a race.. ? */
    case IDCMP_INACTIVEWINDOW: {
	JanusWin *old;

	if(win) {
	  JWLOG("aros_win_thread[%lx]: IDCMP_INACTIVEWINDOW(%lx, %s)\n", thread, win, win->Title);
	}
	inputdevice_unacquire ();

	Delay(10);
	ObtainSemaphore(&sem_janus_active_win);
#if 0
	ObtainSemaphore(&sem_janus_window_list);
	list_win=g_slist_find_custom(janus_windows,
                               (gconstpointer) win,
                               &aros_window_compare);
	ReleaseSemaphore(&sem_janus_window_list);
	old=(JanusWin *) list_win->data;
#endif
	old=jwin;
	if(old == janus_active_window) {
	  janus_active_window=NULL;
	  JWLOG("aros_win_thread[%lx]: janus_active_window=NULL\n", thread);
	  copy_clipboard_to_aros();
	}
	ReleaseSemaphore(&sem_janus_active_win);
	break;
    }
  }
  JWLOG("aros_win_thread[%lx]: handle_input(jwin %lx, ..) done\n", thread, jwin);
}

/********************************************************
 * xy_compare
 *
 * compare two JanusGadgets, so that they are sorted 
 * according to their x (and second y) coordinates
 ********************************************************/
static gint xy_compare(gconstpointer a, gconstpointer b) {

  if( ((JanusGadget *)a)->x < ((JanusGadget *)b)->x) {
    return -1;
  }
  if( ((JanusGadget *)a)->x > ((JanusGadget *)b)->x) {
    return 1;
  }

  /* x == x, sort y */
  if( ((JanusGadget *)a)->y < ((JanusGadget *)b)->y) {
    return -1;
  }
  if( ((JanusGadget *)a)->y > ((JanusGadget *)b)->y) {
    return 1;
  }

  /* x == x and y == y ?? */
  return 0;
}

/********************************************************
 * init_border_gadgets
 *
 * fill up/down/left/right arrow JanusGadgets
 ********************************************************/
static void init_border_gadgets(struct Process *thread, JanusWin *jwin) {
  ULONG gadget;
  ULONG specialinfo;
  UWORD gadget_type;
  UWORD gadget_flags;
  UWORD spezial_info_flags;
  WORD  x=0, y=0;
  JanusGadget *jgad;
  GList *aos3_gadget_list=NULL;

  gadget=get_long_p(jwin->aos3win + 62);

  while(gadget) {

    gadget_type =get_word(gadget + 16); 
    gadget_flags=get_word(gadget + 12);

    if( ( !(gadget_type  & GTYP_SYSGADGET) ) &&
	( !(gadget_flags & GACT_TOPBORDER ) ) &&
	( !(gadget_flags & GACT_LEFTBORDER ) ) &&
        (  (gadget_type    & GTYP_CUSTOMGADGET) ) 
      ) {

      x=get_word(gadget + 4);
      y=get_word(gadget + 6);

      JWLOG("aros_win_thread[%lx]: cust gadget %lx: x %d y %d\n", thread, gadget, x, y);


      jgad=(JanusGadget *) AllocPooled(jwin->mempool, sizeof(JanusGadget));
      jgad->x=x;
      jgad->y=y;
      jgad->aos3gadget=gadget;

      aos3_gadget_list=g_list_append(aos3_gadget_list, (gpointer) jgad);
    }
    else if( gadget_type    & GTYP_PROPGADGET ) {

      specialinfo=get_long(gadget + 34);

      if(specialinfo) {
      	spezial_info_flags=get_word(specialinfo);
  	x=get_word(gadget + 4);
   	y=get_word(gadget + 6);

	if(spezial_info_flags & FREEHORIZ) {
 
	  JWLOG("aros_win_thread[%lx]: FREEHORIZ prop gadget %lx: x %d y %d\n", thread, gadget, x, y);

	  jgad=(JanusGadget *) AllocPooled(jwin->mempool, sizeof(JanusGadget));
	  jgad->x=x;
	  jgad->y=y;
	  jgad->flags=get_word(specialinfo);
	  jgad->aos3gadget=gadget;
	  jwin->prop_left_right=jgad;
	}

	if(spezial_info_flags & FREEVERT) {
 
	  JWLOG("aros_win_thread[%lx]: FREEVERT prop gadget %lx: x %d y %d\n", thread, gadget, x, y);

	  jgad=(JanusGadget *) AllocPooled(jwin->mempool, sizeof(JanusGadget));
	  jgad->x=x;
	  jgad->y=y;
	  jgad->flags=get_word(specialinfo);
	  jgad->aos3gadget=gadget;
	  jwin->prop_up_down=jgad;
	}
      }
    }


    gadget=get_long(gadget); /* NextGadget */
  }

  aos3_gadget_list=g_list_sort(aos3_gadget_list, &xy_compare);

  if((g_list_length(aos3_gadget_list) == 4) || (g_list_length(aos3_gadget_list) == 2)) {
    /* otherwise don't even try */
    jwin->arrow_left  = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 0);
    jwin->arrow_right = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 1);

    if(jwin->arrow_left->y != jwin->arrow_right->y) {
      /* non-aligned, seems to be something different.. */
      jwin->arrow_left =NULL;
      jwin->arrow_right=NULL;
    }

    if(g_list_length(aos3_gadget_list) == 4) {
      jwin->arrow_up   = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 2);
      jwin->arrow_down = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 3);
    }
    else {
      jwin->arrow_up   = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 0);
      jwin->arrow_down = (JanusGadget *) g_list_nth_data(aos3_gadget_list, 1);
    }

    if(jwin->arrow_up->x != jwin->arrow_down->x) {
      /* non-aligned, seems to be something different.. */
      jwin->arrow_up  =NULL;
      jwin->arrow_down=NULL;
    }
  }

  g_list_free(aos3_gadget_list);
  /* there still might be some elements alloced, in case !=2 and !=4 for example.
   * but they are from our pool, so we save the trouble of freeing them.
   */

  return;
}

extern BOOL manual_mouse;
extern WORD manual_mouse_x;
extern WORD manual_mouse_y;

BOOL horiz_prop_active=FALSE;
BOOL vert_prop_active=FALSE;

void move_horiz_prop_gadget(struct Process *thread, JanusWin *jwin) {
  UWORD x;
  ULONG t;
  ULONG specialinfo;
  JanusGadget *jgad=NULL;

  jgad=jwin->prop_left_right; 
  specialinfo=get_long(jgad->aos3gadget + 34);

  /* place mouse on the horizontal amigaos proportional gadget */
  /* y middle of gadget: TopEdge + Height - BorderBottom/2 */
  manual_mouse_y=get_word(jwin->aos3win+6) + get_word(jwin->aos3win+10) - (get_byte(jwin->aos3win+57)/2);

  //manual_mouse_y=y + get_word(jgad->aos3gadget + 6) + (get_word(jgad->aos3gadget + 10)/2);

  JWLOG("aros_win_thread[%lx]:  manual_mouse_y %d\n", thread, manual_mouse_y);

  JWLOG("aros_win_thread[%lx]:  width %d\n", thread, get_word(jgad->aos3gadget +  8));
  JWLOG("aros_win_thread[%lx]:  horizpot %x\n", thread, get_word(specialinfo + 2));
  JWLOG("aros_win_thread[%lx]:  CWidth %d\n", thread, get_word(specialinfo + 10));
  JWLOG("aros_win_thread[%lx]:  LeftBorder %d\n", thread, get_word(specialinfo + 18));

  /* left edge + left border width of aos3 window */
  x=get_word(jwin->aos3win + 4) + get_byte(jwin->aos3win + 54);

  /* border width of gadget */
  x=x + get_word(specialinfo + 18);

  /* CWidth * HorizPot / MAX_POT */
  //t=get_word(specialinfo + 10) * get_word(specialinfo + 2) / MAXPOT;
  t=get_word(specialinfo + 10) * ((struct PropInfo *) jwin->gad[GAD_HORIZSCROLL]->SpecialInfo)->HorizPot / MAXPOT;

  x=x+(t/2);

  JWLOG("aros_win_thread[%lx]: GAD_HORIZSCROLL: x %d\n", thread, x);

  /* use a long here to avoid overflow problems */
  /* (width * horizpot) / 0xFFFF */

#if 0
  t=((get_word(jgad->aos3gadget +  8) * get_word(specialinfo + 2)) / 0xFFFF);
  /* those values are negative */
  JWLOG("aros_win_thread[%lx]: GAD_HORIZSCROLL: LeftEdge %x\n", thread, get_word(jgad->aos3gadget + 4));
  /* x + Gadget LeftEdge + t */
  manual_mouse_x=x + get_word(jgad->aos3gadget + 4) + (WORD) t;
#endif
  manual_mouse_x=x;
  /* debug! */

  JWLOG("aros_win_thread[%lx]: hit gadget %lx at %d x %d..\n", thread, jgad->aos3gadget, manual_mouse_x, manual_mouse_y);

  mice[0].enabled=FALSE; /* disable mouse emulation */
  manual_mouse=TRUE;
  while(manual_mouse) {
    /* wait until the mouse is, where it should be */
    Delay(1);
  }
  Delay(10);
}

void move_vert_prop_gadget(struct Process *thread, JanusWin *jwin) {
  UWORD y;
  ULONG t;
  ULONG rulerheight;
  ULONG freeheight;
  ULONG specialinfo;
  JanusGadget *jgad=NULL;

  jgad=jwin->prop_up_down; 
  specialinfo=get_long(jgad->aos3gadget + 34);

  /* place mouse on the vertical amigaos proportional gadget */
  /* x middle of gadget: LeftEdge + Width - BorderRight/2 */
  manual_mouse_x=get_word(jwin->aos3win+4) + get_word(jwin->aos3win+8) - (get_byte(jwin->aos3win+56)/2);

  JWLOG("aros_win_thread[%lx]:  manual_mouse_x %d\n", thread, manual_mouse_x);

  JWLOG("aros_win_thread[%lx]:  window top edge %d\n", thread, get_word(jwin->aos3win + 6));
  JWLOG("aros_win_thread[%lx]:  window top border height %d\n", thread, get_byte(jwin->aos3win + 55));
  JWLOG("aros_win_thread[%lx]:  gadget specialinfo CHight %d\n", thread, get_word(specialinfo + 12));
  JWLOG("aros_win_thread[%lx]:  aros VertPot %d\n", thread, ((struct PropInfo *) jwin->gad[GAD_VERTSCROLL]->SpecialInfo)->VertPot);

  /* top edge + top border height of aos3 window */
  y=get_word(jwin->aos3win + 6) + get_byte(jwin->aos3win + 55);

  /* size of ruler = 68k gadget height * VertBody / MAXBODY */
  rulerheight=get_word(specialinfo + 12) * get_word(specialinfo + 8) / MAXBODY;
  JWLOG("aros_win_thread[%lx]:  rulerheight %d\n", thread, rulerheight);

  /* topmost y clickpoint for us
   * assuming the slder gadget is in it's top position, we need to click in the middle of the
   * slider (rulerheight / 2). We also add the border of the Propgadget, as we don't want to
   * click on that either.
   */
  y=y + rulerheight/2 + get_word(specialinfo + 20) ;

  /* free space outside of ruler */
  freeheight=get_word(specialinfo + 12) - rulerheight;

  /* we now take the percentage of the freeheight, that corresponds to the actual prop value */
  t=(freeheight * ((struct PropInfo *) jwin->gad[GAD_VERTSCROLL]->SpecialInfo)->VertPot) / MAXPOT;
  JWLOG("aros_win_thread[%lx]:  t %d\n", thread, t);

  /* click point!
   * as we only use the freeheight value, the maximum we can reach is the middle of the
   * slider, if the slider is on the lowest position. This is exactly, what we want.
   */
  y=y + t; 

#if 0
  /* otherwise we will miss the gadget */
  if(y<2) {
    y=2;
  }
#endif

  JWLOG("aros_win_thread[%lx]: GAD_VERTSCROLL: y %d\n", thread, y);

  manual_mouse_y=y;

  JWLOG("aros_win_thread[%lx]: hit gadget %lx at %d x %d..\n", thread, jgad->aos3gadget, manual_mouse_x, manual_mouse_y);

  mice[0].enabled=FALSE; /* disable mouse emulation */
  manual_mouse=TRUE;
  while(manual_mouse) {
    /* wait until the mouse is, where it should be */
    Delay(1);
  }
  Delay(10);
}

/********************************************************
 * handle_gadget
 *
 * react on border gadget clicks
 ********************************************************/
static void handle_gadget(struct Process *thread, JanusWin *jwin, UWORD gadid) {
  UWORD x,y;
  ULONG t;
  ULONG specialinfo;
  JanusGadget *jgad=NULL;

  JWLOG("aros_win_thread[%lx]: jwin %lx, gadid %d\n", thread, jwin, gadid);

  switch (gadid) {
    case GAD_DOWNARROW:
    case GAD_UPARROW:
    case GAD_LEFTARROW:
    case GAD_RIGHTARROW:
      JWLOG("aros_win_thread[%lx]: GAD_DOWNARROW etc\n", thread);
      if(!jwin->arrow_up && !jwin->arrow_left) {
	init_border_gadgets(thread, jwin);
      }

      switch (gadid) {
	case GAD_DOWNARROW : jgad=jwin->arrow_down;  break;
	case GAD_UPARROW   : jgad=jwin->arrow_up;    break;
	case GAD_LEFTARROW : jgad=jwin->arrow_left;  break;
	case GAD_RIGHTARROW: jgad=jwin->arrow_right; break;
      }
      if(!jgad) {
	JWLOG("aros_win_thread[%lx]: jgad not matched??\n", thread);
	/* should not happen */
	break;
      }

      /* right border end of aos3 window */
      x=jwin->aroswin->LeftEdge + jwin->aroswin->Width - jwin->aroswin->BorderRight;
      /* those values are negative */
      manual_mouse_x=x + get_word(jgad->aos3gadget + 4) + (get_word(jgad->aos3gadget +  8)/2);

      y=jwin->aroswin->TopEdge + jwin->aroswin->Height - jwin->aroswin->BorderBottom;
      manual_mouse_y=y + get_word(jgad->aos3gadget + 6) + (get_word(jgad->aos3gadget + 10)/2);

      JWLOG("aros_win_thread[%lx]: hit gadget %lx at %d x %d..\n", thread, jgad->aos3gadget, manual_mouse_x, manual_mouse_y);

      mice[0].enabled=FALSE; /* disable mouse emulation */
      manual_mouse=TRUE;
      while(manual_mouse) {
	/* wait until the mouse is, where it should be */
	Delay(1);
      }
      Delay(10);

      my_setmousebuttonstate(0, 0, 1); /* click */


    break;

    case GAD_HORIZSCROLL:
      JWLOG("GAD_HORIZSCROLL!\n");
 
      move_horiz_prop_gadget(thread, jwin); 
      my_setmousebuttonstate(0, 0, 1); /* click */
      horiz_prop_active=TRUE;
      JWLOG("horiz_prop_active=TRUE\n");

    break;

    case GAD_VERTSCROLL:
      JWLOG("GAD_VERTSCROLL!\n");
 
      move_vert_prop_gadget(thread, jwin); 
      my_setmousebuttonstate(0, 0, 1); /* click */
      vert_prop_active=TRUE;
      JWLOG("vert_prop_active=TRUE\n");

    break;


    default:
      JWLOG("aros_win_thread[%lx]: WARNING: gadid %d is not handled (yet?)\n", thread, gadid);
    break;
  }

}

/* semaphore protect this ? */
BOOL  pointer_is_hidden=FALSE;
ULONG pointer_skip=0;

static void handle_msg(struct Message *msg, struct Window *win, JanusWin *jwin, ULONG class, UWORD code, int dmx, int dmy, WORD mx, WORD my, int qualifier, struct Process *thread, ULONG secs, ULONG micros, BOOL *done) {

  GSList   *list_win;
  JanusMsg *jmsg;
  UWORD     selection;
  ULONG     flags;
  UWORD     gadid;

  switch (class) {
    case IDCMP_RAWKEY:
    case IDCMP_MOUSEBUTTONS:
    case IDCMP_ACTIVEWINDOW:
    case IDCMP_INACTIVEWINDOW:
      handle_input(win, jwin, class, code, qualifier, thread);
      break;

    case IDCMP_GADGETDOWN:
      JWLOG("aros_win_thread[%lx]: IDCMP_GADGETDOWN\n", thread);
      gadid = ((struct Gadget *)((struct IntuiMessage *)msg)->IAddress)->GadgetID;
      handle_gadget(thread, jwin, gadid);
      break;

    case IDCMP_GADGETUP:
      /* avoid race conditions with handle_gadget ... 
       * TODO: find a better way for this?
       */
      while(manual_mouse) {
	Delay(1);
      }
      Delay(15);

      my_setmousebuttonstate(0, 0, 0); /* unclick */
      horiz_prop_active=FALSE;
      vert_prop_active=FALSE;
      JWLOG("prop_active=FALSE\n");
      mice[0].enabled=TRUE; /* enable mouse emulation */
      break;

    case IDCMP_CLOSEWINDOW:
      if(!jwin->custom) {
	/* fake IDCMP_CLOSEWINDOW to original aos3 window */
	JWLOG("aros_win_thread[%lx]: CLOSEWINDOW received for jwin %lx (%s)\n", 
		thread, jwin, win->Title);

	ObtainSemaphore(&janus_messages_access);
	/* this gets freed in ad_job_fetch_message! */
	jmsg=AllocVec(sizeof(JanusMsg), MEMF_CLEAR); 
	if(!jmsg) {
	  JWLOG("aros_win_thread[%lx]: ERROR: no memory (ignored message)\n", thread);
	  break;
	}
	jmsg->jwin=jwin;
	jmsg->type=J_MSG_CLOSE;
	janus_messages=g_slist_append(janus_messages, jmsg);
	ReleaseSemaphore(&janus_messages_access);

      }
      break;

    case IDCMP_CHANGEWINDOW:
      if(!jwin->custom) {

	/* ChangeWindowBox is not done at once, but deferred. You 
	 * can detect that the operation has completed by receiving 
	 * the IDCMP_CHANGEWINDOW IDCMP message
	 */
	JWLOG("aros_win_thread[%lx]: IDCMP_CHANGEWINDOW: received\n", thread);
	if(jwin->delay > WIN_DEFAULT_DELAY) {
	  jwin->delay=0;
	}
	refresh_content(jwin);
	activate_ticks(jwin, 0);

	/* debug only */
	{
	  ULONG gadget;
      	  gadget=get_long_p(jwin->aos3win + 62);
	  while(gadget) {
	    dump_prop_gadget(thread, gadget);
	    gadget=get_long(gadget); /* NextGadget */
	  }
 
	}
      }
      break;

    case IDCMP_MOUSEMOVE:
	/* this might be a little bit expensive? Is it performat enough? 
	 * so maybe we just do it every 3rd mouse move..
	 */

	if(pointer_skip==0) {

	  pointer_skip=3;

	  if(!jwin->custom) {
	    /* hide pointer, if it is above us */
	    struct Layer  *layer;
	    struct Screen *scr = jwin->aroswin->WScreen;
	    GSList        *list_win;
	    JanusWin      *swin;
	    BOOL           found;

	    LockLayerInfo (&scr->LayerInfo);
	    layer = WhichLayer (&scr->LayerInfo, scr->MouseX, scr->MouseY);
	    UnlockLayerInfo (&scr->LayerInfo);

	    if(layer == jwin->aroswin->WLayer) {
	      /* inside ourselves */
	      JWLOG("aros_win_thread[%lx]: IDCMP_MOUSEMOVE: inside\n", thread);
	      if(!pointer_is_hidden) {
		hide_pointer (jwin->aroswin);
		JWLOG("aros_win_thread[%lx]: POINTER: hide\n", thread);
		pointer_is_hidden=TRUE;
	      }
	    }
	    else {

	      found=FALSE;
	      JWLOG("aros_win_thread[%lx]: ObtainSemaphore(&sem_janus_window_list);\n", thread);
	      ObtainSemaphore(&sem_janus_window_list);
	      JWLOG("aros_win_thread[%lx]: obtained sem_janus_window_list sem \n",thread);

	      /* if we are above one of our other windows, hide it too */
	      list_win=janus_windows;
	      while(!found && list_win) {
		swin=(JanusWin *) list_win->data;
		if(swin && swin->aroswin && (layer == swin->aroswin->WLayer)) {
		  found=TRUE;
		  if(!pointer_is_hidden) {
		    hide_pointer (jwin->aroswin);
		    JWLOG("aros_win_thread[%lx]: POINTER: hide\n", thread);
		    pointer_is_hidden=TRUE;
		  }
		}
		list_win=g_slist_next(list_win);
	      }

	      if(!found) {
		if(pointer_is_hidden) {
		  show_pointer (jwin->aroswin);
		  JWLOG("aros_win_thread[%lx]: POINTER: show\n", thread);
		  pointer_is_hidden=FALSE;
		}
	      }

	      JWLOG("aros_win_thread[%lx]: ReleaseSemaphore(&sem_janus_window_list)\n", thread);
	      ReleaseSemaphore(&sem_janus_window_list);
	      JWLOG("aros_win_thread[%lx]: released sem_janus_window_list sem \n",thread);
	    }

	  }
	}
	else {
	  pointer_skip--;
	}
	break;
#if 0
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

    case IDCMP_MENUVERIFY:
      if(!jwin->custom) {
	if(IntuitionBase->ActiveWindow != win) {
	  /* this seems to be a bug in aros, why are we getting those messages at all !? */
	  /* no, it is a feature. Any window on a screen gets those.. C=.. */
	  JWLOG("aros_win_thread[%lx]: WARNING: foreign IDCMP message IDCMP_MENUVERIFY received\n", thread);
	}
	else {
	  struct IntuiMessage *im=(struct IntuiMessage *)msg;
	  flags=get_long_p(jwin->aos3win + 24); 
	  JWLOG("aros_win_thread[%lx]: IDCMP_MENUVERIFY flags: %lx\n", thread, flags);

	  /* depending on whether the original amigaOS window has WFLG_RMBTRAP set
	   * or not, we need to show a menu or not. This is quite a brain damaged
	   * thing, as for example MUI checks on every mouse move, if it has to
	   * change the WFLG_RMBTRAP, depending on whether the mouse is above
	   * a gadget with or without intuition menu..
	   */

	  if(flags & WFLG_RMBTRAP) {
	    JWLOG("aros_win_thread[%lx]: IDCMP_MENUVERIFY => right click only\n", thread);
	    im->Code = MENUCANCEL;
	    my_setmousebuttonstate(0, 1, 1); /* MENUDOWN */
	  }
	  else {
	    JWLOG("aros_win_thread[%lx]: IDCMP_MENUVERIFY => real IDCMP_MENUVERIFY\n", thread);
	    /* show pointer. As long as the menus are displayed, we won't get any mousemove
	     * events, so the pointer would not be shown, if moved.
	     */
	    /* if !mice[0].enabled, mouse coordinates are taken from menux and menuy and
	     * reset to 0 as soon as the mouse was moved there
	     */
#if 0
	    menux=jwin->jscreen->arosscreen->Width;
	    menuy=jwin->jscreen->arosscreen->Height;
	    while(menux != 0) {
	      Delay(5);
	    }
#endif
	    show_pointer(jwin->aroswin);
	    menux=0;
	    menuy=0;
	    j_stop_window_update=TRUE;
	    mice[0].enabled=FALSE; /* disable mouse emulation */
	    my_setmousebuttonstate(0, 1, 1); /* MENUDOWN */
	    clone_menu(jwin);
	  }
	}
      }
      break;

    case IDCMP_MENUPICK:
      if(!jwin->custom) {
	JWLOG("aros_win_thread[%lx]: IDCMP_MENUPICK\n", thread);

	/* nothing selected, but this could mean, the user clicked twice very fast and
	 * thus wanted to activate a DMRequest. So we remember/check the time of the last
	 * click and replay it, if appropriate 
	 */
	if(code==MENUNULL) {
	  if(jwin->micros) {
	    JWLOG("aros_win_thread[%lx]: MENUNULL difference: %d\n", thread,
	          olisecs(secs, micros) - olisecs(jwin->secs, jwin->micros));
	    if(olisecs(secs, micros) - olisecs(jwin->secs, jwin->micros) < 1500) {
	      JWLOG("aros_win_thread[%lx]: MENUNULL DOUBLE!!\n", thread);
	      /* we already had one MENUDOWN up in MENUVERIFY */
	      setmousebuttonstate(0, 1, 0); /* MENUUP */
	      //Delay(1000);
	      setmousebuttonstate(0, 1, 1); /* MENUDOWN */
	      /* and we'll have one MENUDOWN at the end */
	    }
	    jwin->micros=0;
	    jwin->secs=0;
	  }
	  else {
	    jwin->micros=micros;
	    jwin->secs=secs;
	  }
	}
	else {
	  /* user selected something (reasonable) */

	  jwin->micros=0; /* user picked sth, so forget about the last click */
	  jwin->secs=0; 

	  for(selection = code; selection != MENUNULL;
	      selection = (ItemAddress(jwin->arosmenu, (LONG)selection))
			   ->NextSelect) {
	    process_menu(jwin, selection);
	  }
#if 0
	  sleep(4);
	  JWLOG("click_menu(jwin, 1, 0, -1);\n");
	  click_menu(jwin, 1, 0 , -1);
	  sleep(3);
	  JWLOG("click_menu(jwin, 1, 1, -1);\n");
	  click_menu(jwin, 1, 1, -1);
	  sleep(3);
	  JWLOG("click_menu(jwin, 1, 2, -1);\n");
	  click_menu(jwin, 1, 2, -1);
	  sleep(3);
	  JWLOG("click_menu(jwin, 1, 2, -1);\n");
	  click_menu(jwin, 1, 2, -1);
	  sleep(3);
#endif
	}

	my_setmousebuttonstate(0, 1, 0); /* MENUUP */
	menux=0;
	menuy=0;
	mice[0].enabled=TRUE; /* enable mouse emulation */
	j_stop_window_update=FALSE;
	update_pointer(jwin->aroswin, jwin->aroswin->MouseX, jwin->aroswin->MouseY);
      }
      break;

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

#endif
    case IDCMP_REFRESHWINDOW:
      JWLOG("aros_win_thread[%lx]: IDCMP_REFRESHWINDOW!\n", thread);
#if 0
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
      BeginRefresh (win);
     // clone_window(jwin->aos3win, jwin->aroswin, 0, 0xFFFFFF); /* clone window again */
      refresh_content(jwin);
      EndRefresh (win, TRUE);
      break;
    default:
      JWLOG("aros_win_thread[%lx]: Unknown IDCMP class %lx received\n", thread, class);
      break;
  }
}

static void dump_prop_gadget(struct Process *thread, ULONG gadget) {
  UWORD  gadget_type;
  ULONG  specialinfo;
  UWORD  flags;

  gadget_type =get_word(gadget + 16); 

  if((gadget_type & GTYP_GTYPEMASK) != GTYP_PROPGADGET) {
    JWLOG("aros_win_thread[%lx]: gadget %lx is not a  GTYP_PROPGADGET\n", thread, gadget);
  }

  specialinfo=get_long(gadget + 34);
  JWLOG("aros_win_thread[%lx]: gadget %lx SpecialInfo: %lx\n", thread, gadget, specialinfo);

  if(!specialinfo) {
    return;
  }

  flags=get_word(specialinfo);
  JWLOG("aros_win_thread[%lx]: flags %lx\n", thread, flags);
  if(flags & FREEHORIZ) {
    JWLOG("aros_win_thread[%lx]: flag FREEHORIZ\n", thread);
    JWLOG("aros_win_thread[%lx]: HorizPot %d\n", thread, get_word(specialinfo+2));
    JWLOG("aros_win_thread[%lx]: HorizBody %d\n", thread, get_word(specialinfo+6));
  }
  if(flags & FREEVERT) {
    JWLOG("aros_win_thread[%lx]: flag FREEVERT\n", thread);
    JWLOG("aros_win_thread[%lx]: VertPot %d\n", thread, get_word(specialinfo+4));
    JWLOG("aros_win_thread[%lx]: VertBody %d\n", thread, get_word(specialinfo+8));
  }
}

/***********************************************************
 * make_gadgets(jwin)
 *
 * create border gadgets
 ***********************************************************/
static struct Gadget *make_gadgets(struct Process *thread, JanusWin* jwin) {
    IPTR imagew[NUM_IMAGES], imageh[NUM_IMAGES];
    WORD v_offset, h_offset, btop, i;
    struct Gadget   *vertgadget =NULL;
    struct Gadget   *horizgadget=NULL;
    WORD img2which[] = {
   	UPIMAGE, 
   	DOWNIMAGE, 
   	LEFTIMAGE, 
   	RIGHTIMAGE, 
   	SIZEIMAGE
    };

   
   jwin->dri = GetScreenDrawInfo(jwin->jscreen->arosscreen);

    for(i = 0;i < NUM_IMAGES;i++) {

	jwin->img[i] = NewObject(0, SYSICLASS, SYSIA_DrawInfo, (Tag) jwin->dri, 
				         SYSIA_Which, (Tag) img2which[i], 
				         TAG_DONE);

	if (!jwin->img[i]) {
	  JWLOG("aros_win_thread[%lx]: could not create image %d\n", thread, i);
	  return NULL;
	}

	GetAttr(IA_Width,  (Object *) jwin->img[i], &imagew[i]);
	GetAttr(IA_Height, (Object *) jwin->img[i], &imageh[i]);
    }

    btop = jwin->jscreen->arosscreen->WBorTop + jwin->dri->dri_Font->tf_YSize + 1;

    JWLOG("aros_win_thread[%lx]: btop=%d\n", thread, btop);
    JWLOG("aros_win_thread[%lx]: imagew[IMG_UPARROW]  =%d\n", thread, imagew[IMG_UPARROW]);
    JWLOG("aros_win_thread[%lx]: imageh[IMG_DOWNARROW]=%d\n", thread, imageh[IMG_DOWNARROW]);

    v_offset = imagew[IMG_DOWNARROW] / 4;
    h_offset = imageh[IMG_LEFTARROW] / 4;

    if(jwin->arrow_up) {
      vertgadget = 
      jwin->gad[GAD_UPARROW] = NewObject(0, BUTTONGCLASS, GA_Image, (Tag) jwin->img[IMG_UPARROW], 
					  GA_RelRight, -imagew[IMG_UPARROW] + 1, 
					  GA_RelBottom, -imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1, 
					  GA_ID, GAD_UPARROW, 
					  GA_RightBorder, TRUE, 
					  GA_Immediate, TRUE,
					  GA_RelVerify, TRUE, 
					  GA_GZZGadget, TRUE,
					  TAG_DONE);

      jwin->gad[GAD_DOWNARROW] = NewObject(0, BUTTONGCLASS, GA_Image, (Tag) jwin->img[IMG_DOWNARROW], 
					  GA_RelRight, -imagew[IMG_UPARROW] + 1, 
					  GA_RelBottom, -imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1, 
					  GA_ID, GAD_DOWNARROW, 
					  GA_RightBorder, TRUE, 
					  GA_Previous, (Tag) jwin->gad[GAD_UPARROW], 
					  GA_Immediate, TRUE,
					  GA_RelVerify    , TRUE, 
					  GA_GZZGadget, TRUE,
					  TAG_DONE);

      jwin->gad[GAD_VERTSCROLL] = NewObject(0, PROPGCLASS, GA_Top, btop + 1, 
					  GA_RelRight, -imagew[IMG_DOWNARROW] + v_offset + 1, 
					  GA_Width, imagew[IMG_DOWNARROW] - v_offset * 2,
					  GA_RelHeight, -imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] - btop -2, 
					  GA_ID, GAD_VERTSCROLL, 
					  GA_Previous, (Tag) jwin->gad[GAD_DOWNARROW], 
					  GA_RightBorder, TRUE, 
					  GA_RelVerify, TRUE, 
					  GA_Immediate, TRUE, 
					  PGA_NewLook, TRUE, 
					  PGA_Borderless, TRUE, 
#if 0
					  PGA_Total, , 
					  PGA_Visible, 100, 
#endif
					  PGA_VertPot,    MAXPOT,
					  PGA_VertBody,   MAXBODY,
					  PGA_Freedom, FREEVERT, 
					  GA_GZZGadget, TRUE,
					  TAG_DONE);
      for(i = 0;i < 3;i++) {
	  JWLOG("aros_win_thread[%lx]: gad[%d]=%lx\n", thread, i, jwin->gad[i]);
  	  if (!jwin->gad[i]) {
  	    JWLOG("aros_win_thread[%lx]: ERROR: gad[%d]==NULL\n", thread, i);
  	    return NULL;
  	  }
      }
    }

    if(jwin->arrow_left) {
      horizgadget=
      jwin->gad[GAD_RIGHTARROW] = NewObject(0, BUTTONGCLASS, GA_Image, (Tag) jwin->img[IMG_RIGHTARROW], 
					  GA_RelRight, -imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] + 1, 
					  GA_RelBottom, -imageh[IMG_RIGHTARROW] + 1, 
					  GA_ID, GAD_RIGHTARROW, 
					  GA_BottomBorder, TRUE, 
					  jwin->arrow_up ? GA_Previous : TAG_IGNORE, (Tag) jwin->gad[GAD_VERTSCROLL], 
					  GA_Immediate, TRUE, 
					  GA_RelVerify, TRUE,
					  GA_GZZGadget, TRUE,
					  TAG_DONE);

       jwin->gad[GAD_LEFTARROW] = NewObject(0, BUTTONGCLASS, GA_Image, (Tag) jwin->img[IMG_LEFTARROW], 
					  GA_RelRight, -imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] - imagew[IMG_LEFTARROW] + 1, 
					  GA_RelBottom, -imageh[IMG_RIGHTARROW] + 1, 
					  GA_ID, GAD_LEFTARROW, 
					  GA_BottomBorder, TRUE, 
					  GA_Previous, (Tag) jwin->gad[GAD_RIGHTARROW], 
					  GA_Immediate, TRUE, 
					  GA_RelVerify, TRUE ,
					  GA_GZZGadget, TRUE,
					  TAG_DONE);

       jwin->gad[GAD_HORIZSCROLL] = NewObject(0, PROPGCLASS, GA_Left, jwin->jscreen->arosscreen->WBorLeft, 
					  GA_RelBottom, -imageh[IMG_LEFTARROW] + h_offset + 1, 
					  GA_RelWidth, -imagew[IMG_LEFTARROW] - imagew[IMG_RIGHTARROW] - imagew[IMG_SIZE] - jwin->jscreen->arosscreen->WBorRight - 2, 
					  GA_Height, imageh[IMG_LEFTARROW] - (h_offset * 2), 
					  GA_ID, GAD_HORIZSCROLL, 
					  GA_Previous, (Tag) jwin->gad[GAD_LEFTARROW], 
					  GA_BottomBorder, TRUE, 
					  GA_RelVerify, TRUE, 
					  GA_Immediate, TRUE, 
					  PGA_NewLook, TRUE, 
					  PGA_Borderless, TRUE, 
					  PGA_Total, 80, 
					  PGA_Visible, 80, 
					  PGA_Freedom, FREEHORIZ, 
					  GA_GZZGadget, TRUE,
					  TAG_DONE);
      for(i = 3;i < NUM_GADGETS;i++) {
  	  if (!jwin->gad[i]) {
  	    JWLOG("aros_win_thread[%lx]: ERROR: gad[%d]==NULL\n", thread, i);
  	    return NULL;
  	  }
      }

    }

    if(jwin->arrow_up) {
      return vertgadget;
    }

    return horizgadget;
}

/* 
 * AROS creates PROPGADGETS as GTYP_CUSTOMGADGET, *not*
 * as GTYP_PROPGADGET. 
 * See AROS/rom/intuition/propgclass.c 
 *
 * Only when needed (?) propgclass changes the type to 
 * GTYP_PROPGADGET and switches back as soon as possible
 * to GTYP_CUSTOMGADGET.
 *
 * As I don't understand that magic, I don't want to change 
 * that in AROS. We just do the same. If we don't do it,
 * NewModifyProp won't work, as it checks for GTYP_PROPGADGET
 * and does nothing for a GTYP_CUSTOMGADGET.
 *
 */
static UWORD SetGadgetType(struct Gadget *gad, UWORD type) {
  UWORD oldtype;

  oldtype=gad->GadgetType;

  gad->GadgetType &= ~GTYP_GTYPEMASK; 
  gad->GadgetType |= type;

  return oldtype;
}


/***********************************************************
 * every AOS3 window gets its own process
 * to open an according AROS window and
 * to report size changes, keystrokes etc. to AOS3
 ***********************************************************/  

static void aros_win_thread (void) {

  struct Process *thread = (struct Process *) FindTask (NULL);
  GSList         *list_win=NULL;
  JanusWin       *jwin=NULL;
  struct Window  *aroswin=NULL;
  void           *name_mem=NULL;
  char           *aos_title;
  WORD            h,w,x,y;
  WORD            minh,minw,maxh,maxw;
  BYTE            br,bl,bt,bb;
  char           *title=NULL;
  ULONG           flags;
  ULONG           idcmpflags;
  int             i;
  ULONG           signals;
  BOOL            done;
  UWORD           code, qualifier;
  WORD            mx, my, dmx, dmy;
  ULONG           class;
  ULONG           gadget;
  UWORD           gadget_flags;
  UWORD           gadget_type;
  struct IntuiMessage *msg;
  ULONG           secs, micros;
  BOOL            care=FALSE;
  struct Gadget  *aros_gadgets;
  ULONG           specialinfo;
  UWORD           gadgettype;

  /* There's a time to live .. */

  JWLOG("aros_win_thread[%lx]: thread running \n",thread);

  /* 
   * deadlock sometimes..?
   */
  JWLOG("aros_win_thread[%lx]: ObtainSemaphore(&sem_janus_window_list);\n", thread);
  ObtainSemaphore(&sem_janus_window_list);
  JWLOG("aros_win_thread[%lx]: obtained sem_janus_window_list sem \n",thread);

  /* who are we? */
  list_win=g_slist_find_custom(janus_windows, 
			     	(gconstpointer) thread,
			     	&aos3_process_compare);

  JWLOG("aros_win_thread[%lx]: ReleaseSemaphore(&sem_janus_window_list)\n", thread);
  ReleaseSemaphore(&sem_janus_window_list);
  JWLOG("aros_win_thread[%lx]: released sem_janus_window_list sem \n",thread);

  if(!list_win) {
    JWLOG("aros_win_thread[%lx]: ERROR: window of this task not found in window list !?\n",thread);
    goto EXIT; 
  }

  jwin=(JanusWin *) list_win->data;
  JWLOG("aros_win_thread[%lx]: win: %lx \n",thread,jwin);
  JWLOG("aros_win_thread[%lx]: win->aos3win: %lx\n",thread,
                                                              jwin->aos3win);

  if(!jwin->custom) {
    /* now let's hope, the aos3 window is not closed already..? */

    /* wrong offset !?
     * idcmpflags=get_long(jwin->aos3win + 80);
     */

    x=get_word((ULONG) jwin->aos3win +  4);
    y=get_word((ULONG) jwin->aos3win +  6);
    JWLOG("aros_win_thread[%lx]: x,y: %d, %d\n",thread, x, y);

    /* aos3 window title is at jwin->aos3win + 32 */
    JWLOG("aros_win_thread[%lx]: aos title: %lx (%s)\n",thread, get_long_p(jwin->aos3win + 32), 
                                                                get_real_address(get_long_p(jwin->aos3win + 32)));
    if(get_long_p(jwin->aos3win + 32)) {
      aos_title=(char *) get_real_address(get_long_p(jwin->aos3win + 32));
      title=AllocVec(strlen(aos_title)+1, MEMF_ANY);
      strcpy(title, aos_title);
    }
    else {
      title=NULL;
    }

    JWLOG("aros_win_thread[%lx]: title: %lx >%s<\n", thread, title, title);

    /* idcmp flags we always need: */
    idcmpflags= IDCMP_NEWSIZE | 
			    IDCMP_CLOSEWINDOW | 
			    IDCMP_RAWKEY |
			    IDCMP_MOUSEBUTTONS |
			    IDCMP_MOUSEMOVE |
			    IDCMP_ACTIVEWINDOW |
			    IDCMP_CHANGEWINDOW |
			    IDCMP_MENUPICK |
			    IDCMP_MENUVERIFY |
			    IDCMP_REFRESHWINDOW |
			    IDCMP_INTUITICKS |
			    IDCMP_INACTIVEWINDOW;

    /* AROS and Aos3 use the same flags */
    flags     =get_long_p(jwin->aos3win + 24); 

    JWLOG("aros_win_thread[%lx]: org flags: %lx \n", thread, flags);

    JWLOG("aros_win_thread[%lx]: WFLG_ACTIVATE: %lx \n", thread, flags & WFLG_ACTIVATE);

    /* we are always WFLG_SMART_REFRESH and never BACKDROP! */
    flags=flags & 0xFFFFFEFF;  /* remove refresh bits and backdrop */

    /* we always want to get a MENUVERIFY, if there is no menu, we will click right on our own*/
    flags=flags & ~WFLG_RMBTRAP;

    flags=flags | WFLG_SMART_REFRESH | WFLG_GIMMEZEROZERO;

    JWLOG("aros_win_thread[%lx]: new flags: %lx \n", thread, flags);
    
    /* CHECKME: need borders here, too? */
    minw=get_word((ULONG) jwin->aos3win + 16); 
    minh=get_word((ULONG) jwin->aos3win + 18);
    maxw=get_word((ULONG) jwin->aos3win + 20);
    maxh=get_word((ULONG) jwin->aos3win + 22);

    if(flags & WFLG_WBENCHWINDOW) {
      /* seems, as if WBench Windows have invalid maxw/maxh (=acth/actw).
       * I did not find that anywhere, but for aos3 this seems to
       * be true. FIXME?
       */
      JWLOG("aros_win_thread[%lx]: this is a WFLG_WBENCHWINDOW\n");
      maxw=0xF000;
      maxh=0xF000;
    }

    w=get_word((ULONG) jwin->aos3win +  8);
    h=get_word((ULONG) jwin->aos3win + 10);
    JWLOG("aros_win_thread[%lx]: w,h: %d, %d\n",thread, w, h);

    bl=get_byte((ULONG) jwin->aos3win + 54);
    bt=get_byte((ULONG) jwin->aos3win + 55);
    br=get_byte((ULONG) jwin->aos3win + 56);
    bb=get_byte((ULONG) jwin->aos3win + 57);
    JWLOG("aros_win_thread[%lx]: border left, right, top, bottom: %d, %d, %d, %d\n",thread, bl, br, bt, bb);

    gadget=get_long_p(jwin->aos3win + 62);
    JWLOG("aros_win_thread[%lx]: ============= gadget =============\n",thread);
    JWLOG("aros_win_thread[%lx]: gadget window: (%lx) %s\n", thread, title, title);
    JWLOG("aros_win_thread[%lx]: gadget borderleft: %d\n",thread,bl);
    JWLOG("aros_win_thread[%lx]: gadget borderright: %d\n",thread,br);
    JWLOG("aros_win_thread[%lx]: gadget bordertop: %d\n",thread,bt);
    JWLOG("aros_win_thread[%lx]: gadget borderbottom: %d\n",thread,bb);
    while(gadget) {
      care=FALSE; /* we need to care for that gadget in respect to plusx/y */

      JWLOG("aros_win_thread[%lx]: gadget: === %lx ===\n",thread, gadget);
      JWLOG("aros_win_thread[%lx]: gadget: x y: %d x %d\n",thread, 
	       get_word(gadget + 4), get_word(gadget +  6));
      JWLOG("aros_win_thread[%lx]: gadget: w h: %d x %d\n",thread, 
	       get_word(gadget + 8), get_word(gadget + 10));

      gadget_flags=get_word(gadget + 12);
      //JWLOG("aros_win_thread[%lx]: gadget: flags %x\n",thread, 
							     //gadget_flags);
      if(gadget_flags & 0x0010) {
	JWLOG("aros_win_thread[%lx]: gadget: GACT_RIGHTBORDER\n",thread);
	care=TRUE;
      }
      if(gadget_flags & 0x0020) {
	JWLOG("aros_win_thread[%lx]: gadget: GACT_LEFTBORDER\n",thread);
	/* care=TRUE !? */;
      }
      if(gadget_flags & 0x0040) {
	JWLOG("aros_win_thread[%lx]: gadget: GACT_TOPBORDER\n",thread);
	/* care=TRUE !? */;
      }
      if(gadget_flags & 0x0080) {
	JWLOG("aros_win_thread[%lx]: gadget: GACT_BOTTOMBORDER\n",thread);
	care=TRUE;
      }

      gadget_type =get_word(gadget + 16); 
      //JWLOG("aros_win_thread[%lx]: gadget: type %x\n",thread, gadget_type);

      if(gadget_type & 0x8000) {
	JWLOG("aros_win_thread[%lx]: gadget: GTYP_SYSGADGET\n",thread);
      }
      if(gadget_type & 0x0005) {
	JWLOG("aros_win_thread[%lx]: gadget: GTYP_CUSTOMGADGET\n",thread);
      }
      else {
	JWLOG("aros_win_thread[%lx]: gadget: UNKNOW TYPE: %d\n",thread,gadget_type);
      }

      if(gadget_type & 0x0010) {
	JWLOG("aros_win_thread[%lx]: gadget: GTYP_SIZING\n",thread);
	care=FALSE;
      }
      if(gadget_type & 0x0020) {
	JWLOG("aros_win_thread[%lx]: gadget: GTYP_WDRAGGING\n",thread);
	care=FALSE;
      }
      if(gadget_type & 0x0040) {
	JWLOG("aros_win_thread[%lx]: gadget: GTYP_WDEPTH\n",thread);
	care=FALSE;
      }
      if(gadget_type & 0x0060) {
	JWLOG("aros_win_thread[%lx]: gadget: GTYP_WZOOM\n",thread);
	care=FALSE;
      }
      if(gadget_type & 0x0080) {
	JWLOG("aros_win_thread[%lx]: gadget: GTYP_CLOSE\n",thread);
	care=FALSE;
      }

      if((gadget_type & GTYP_GTYPEMASK) == GTYP_PROPGADGET) {
	JWLOG("aros_win_thread[%lx]: gadget: GTYP_PROPGADGET\n",thread);
	dump_prop_gadget(thread, gadget);
      }
      if(care) {
	JWLOG("aros_win_thread[%lx]: gadget: ==> ! CARE ! <==\n",thread);
      }
      else {
	JWLOG("aros_win_thread[%lx]: gadget: ==> NOT CARE <==\n",thread);
      }

      gadget=get_long(gadget); /* NextGadget */
    }

    /* for some reason (?), there are some gadgets in small borders, which
     * we should better ignore here ..*/
    /* TODO: gadgets in the top/left border !? */
    if(care && br>5) {
      jwin->plusx=br;
    }
    else {
      jwin->plusx=0;
    }

    if(care && bb>5) {
      jwin->plusy=bb;
    }
    else {
      jwin->plusy=0;
    }

    JWLOG("aros_win_thread[%lx]: jwin: %lx\n", thread, jwin);
    JWLOG("aros_win_thread[%lx]: jwin->jscreen: %lx\n", thread, jwin->jscreen);
    JWLOG("aros_win_thread[%lx]: jwin->jscreen->arosscreen: %lx\n", thread, jwin->jscreen->arosscreen);

    /* 
     * as we open our AROS screens in an own thread out of sync, it
     * might very well be, that amigaOS already opened a window on
     * the new screen, which is not yet opened on AROS
     */
    i=20;
    while(!jwin->jscreen->arosscreen && i--) {
      JWLOG("aros_win_thread[%lx]: #%d wait for jwin->jscreen->arosscreen ..\n", thread, i);
      Delay(10);
    }

    aros_gadgets=NULL; 
    if(care) {
      init_border_gadgets(thread,jwin);
      if(jwin->arrow_up || jwin->arrow_left) {
	aros_gadgets=make_gadgets(thread, jwin);
	if(!aros_gadgets) {
	  JWLOG("aros_win_thread[%lx]: ERROR: could not create gadgets :(!\n", thread);
	  /* goto EXIT; ? */
	}
	else {
	  idcmpflags=idcmpflags | IDCMP_GADGETDOWN | IDCMP_GADGETUP;
	}
      }
    }

    if(jwin->jscreen->arosscreen) {
      activate_ticks(jwin, 5);

      /* 
       * now we need to open up the window .. 
       * hopefully nobody has thicker borders  ..
       */
      jwin->aroswin =  OpenWindowTags(NULL,WA_Title, title,
				      WA_Left, x - estimated_border_left + bl, /* add 68k borderleft!!*/
				      WA_Top, y - estimated_border_top + bt,
				      /* WA_InnerWidth ..!? */
#if 0
				      WA_Width, w - br - bl + 
						estimated_border_left +
						estimated_border_right +
						jwin->plusx,
						/* see below */
#endif
				      WA_InnerWidth, w - br - bl + jwin->plusx,
#if 0
				      WA_Height, h - bt - bb + 
						estimated_border_top +
						estimated_border_bottom +
						jwin->plusy,
#endif
				      WA_InnerHeight, h - bt - bb +jwin->plusy,
				      WA_MinWidth, minw + jwin->plusx,
				      WA_MinHeight, minh + jwin->plusy,
				      WA_MaxWidth, maxw + jwin->plusx,
				      WA_MaxHeight, maxh + jwin->plusy,
				      WA_SmartRefresh, TRUE,
				      WA_GimmeZeroZero, TRUE,
				      WA_Flags, flags,
				      WA_IDCMP, idcmpflags,
				      WA_PubScreen, jwin->jscreen->arosscreen,
				      WA_NewLookMenus, TRUE,
				      WA_SizeBBottom, TRUE,
				      WA_SizeBRight, TRUE,
				      aros_gadgets ? WA_Gadgets : TAG_IGNORE, (IPTR) aros_gadgets,
				      TAG_DONE);

    }
    else {
      JWLOG("aros_win_thread[%lx]: ERROR: could not wait for my screen :(!\n", thread);
      goto EXIT;
    }

    JWLOG("aros_win_thread[%lx]: x, y : %d, %d\n",thread,  
           x - estimated_border_left + bl,
	   y - estimated_border_top  + bt );

    JWLOG("aros_win_thread[%lx]: opened aros window: %lx (%s)\n", thread, jwin->aroswin, title);

    if(!jwin->aroswin) {
      JWLOG("aros_win_thread[%lx]: ERROR: OpenWindow FAILED!\n",thread);
      goto EXIT;
    }

    aroswin=jwin->aroswin; /* shorter to read..*/

    JWLOG("aros_win_thread[%lx]: opened window: w,h: %d, %d\n", thread, jwin->aroswin->Width, jwin->aroswin->Height);
#if 0
    /* resize now, as we need those damned windows borders added */
    ChangeWindowBox(aroswin,
		    x - aroswin->BorderLeft,
		    y - aroswin->BorderTop,
		    w - br - bl + 
		    aroswin->BorderLeft + 
		    aroswin->BorderRight +
		    jwin->plusx,
		    h - bt - bb +
		    aroswin->BorderTop +
		    aroswin->BorderBottom +
		    jwin->plusy);
#endif

    /* remember for the next time */
    estimated_border_top   =aroswin->BorderTop;
    estimated_border_bottom=aroswin->BorderBottom;
    estimated_border_left  =aroswin->BorderLeft;
    estimated_border_right =aroswin->BorderRight;

  } /* endif !win->custom
     * yes, I know, so long if's are evil
     */
  else {

    JWLOG("aros_win_thread[%lx]: open window on custom screen %lx\n", thread, jwin->jscreen->arosscreen);

    jwin->aroswin =  OpenWindowTags(NULL,
				      WA_CustomScreen, jwin->jscreen->arosscreen,
                                      WA_Title,  NULL,
				      WA_Left,   0,
				      WA_Top,    0,
				      WA_Width,  jwin->jscreen->arosscreen->Width,
				      WA_Height, jwin->jscreen->arosscreen->Height,
				      WA_SmartRefresh,  TRUE,
				      WA_Backdrop,      TRUE,
				      WA_Borderless,    TRUE,
				      WA_RMBTrap,       TRUE,
				      WA_NoCareRefresh, TRUE,
				      WA_ReportMouse,   TRUE,
				      WA_IDCMP, IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY         |
				                IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW | 
						IDCMP_MOUSEMOVE    | IDCMP_DELTAMOVE      |
					 	IDCMP_REFRESHWINDOW,
				      TAG_DONE);

    JWLOG("aros_win_thread[%lx]: new aros window %lx\n", thread, jwin->aroswin);

    if(!jwin->aroswin) {
      JWLOG("aros_win_thread[%lx]: ERROR: unable to open window!!!\n", thread);
      goto EXIT;
    }

    /* patch UAE globals, so that UAE thinks, we are his real output window */

    //gfxvidinfo.height=jwin->jscreen->arosscreen->Height;
    //gfxvidinfo.width =jwin->jscreen->arosscreen->Width;

    S  = jwin->jscreen->arosscreen;
    CM = jwin->jscreen->arosscreen->ViewPort.ColorMap;
    RP = &jwin->jscreen->arosscreen->RastPort;
    W  = jwin->aroswin;

  }

  aroswin=jwin->aroswin;

  done=FALSE;
  if(!aroswin->UserPort) {
    /* should not happen.. */
    JWLOG("aros_win_thread[%lx]: ERROR: win %lx has no UserPort !?!\n", thread,aroswin);
    done=TRUE;
  }

  /* handle IDCMP stuff */
  JWLOG("aros_win_thread[%lx]: jwin->task: %lx\n", thread, jwin->task);
  JWLOG("aros_win_thread[%lx]: UserPort: %lx\n", thread, aroswin->UserPort);

  /* refresh display in case we missed some updates, which happened between
   * the amigaOS OpenWindow and our OpenWindow
   */
  refresh_content(jwin);

  JWLOG("aros_win_thread[%lx]: IDCMP loop for window %lx\n", thread, aroswin);

  while(!done) {

    /* wait either for a CTRL_C or a window signal */
    JWLOG("aros_win_thread[%lx]: Wait(%lx)\n", thread, 1L << aroswin->UserPort->mp_SigBit | SIGBREAKF_CTRL_C);
    signals = Wait(1L << aroswin->UserPort->mp_SigBit | SIGBREAKF_CTRL_C);
    JWLOG("aros_win_thread[%lx]: signals: %lx\n", thread, signals);

    if (signals & (1L << aroswin->UserPort->mp_SigBit)) {
      JWLOG("aros_win_thread[%lx]: aroswin->UserPort->mp_SigBit received\n", thread);
      JWLOG("aros_win_thread[%lx]: GetMsg(aroswin %lx ->UserPort %lx)\n", thread, aroswin, aroswin->UserPort);

      while ((msg = (struct IntuiMessage *) GetMsg(aroswin->UserPort))) {
	//JWLOG("IDCMP msg for window %lx\n",aroswin);

	class     = msg->Class;
	code      = msg->Code;
	dmx       = msg->MouseX;
	dmy       = msg->MouseY;
	mx        = msg->IDCMPWindow->MouseX; // Absolute pointer coordinates
	my        = msg->IDCMPWindow->MouseY; // relative to the window
	qualifier = msg->Qualifier;
	secs      = msg->Seconds;
	micros    = msg->Micros;

#if 0
	if(jwin->task != thread) {
	  JWLOG("YYY thread %lx != jwin->task %lx\n", thread, jwin->task);
	}
	else {
	  JWLOG("YYX thread %lx == jwin->task %lx\n", thread, jwin->task);
	}
#endif
	if(class == IDCMP_INTUITICKS) {

	  if(horiz_prop_active||vert_prop_active) {
	    if(horiz_prop_active) {
	      move_horiz_prop_gadget(thread, jwin);
	    }
	    else {
	      move_vert_prop_gadget(thread, jwin);
	    }
	    ReplyMsg ((struct Message *)msg);
	    break;
	  }
	  else {
	    JWLOG("SpecialInfo: jwin->prop_update_count: %d\n", jwin->prop_update_count);
	    if(jwin->prop_update_count == 0) {
	      jwin->prop_update_count=5;

	      if(jwin->prop_up_down) {
		specialinfo=get_long(jwin->prop_up_down->aos3gadget + 34);
		JWLOG("SpecialInfo: HorizPot: %d\n", get_word(specialinfo + 2));
		JWLOG("SpecialInfo: VertPot: %d\n", get_word(specialinfo + 4));
		JWLOG("SpecialInfo: HorizBody: %d\n", get_word(specialinfo + 6));
		JWLOG("SpecialInfo: VertBody: %d\n", get_word(specialinfo + 8));
		if( 
		  ((struct PropInfo *)jwin->gad[GAD_VERTSCROLL]->SpecialInfo)->HorizPot != get_word(specialinfo + 2) ||
		  ((struct PropInfo *)jwin->gad[GAD_VERTSCROLL]->SpecialInfo)->VertPot  != get_word(specialinfo + 4) ||
		  ((struct PropInfo *)jwin->gad[GAD_VERTSCROLL]->SpecialInfo)->HorizBody!= get_word(specialinfo + 6) ||
		  ((struct PropInfo *)jwin->gad[GAD_VERTSCROLL]->SpecialInfo)->VertBody != get_word(specialinfo + 8)
		  ) {

		    JWLOG("SpecialInfo: NewModifyProp horiz ..\n");

		    gadgettype=SetGadgetType(jwin->gad[GAD_VERTSCROLL], GTYP_PROPGADGET);
		    NewModifyProp(jwin->gad[GAD_VERTSCROLL], jwin->aroswin, NULL,
				  jwin->prop_up_down->flags, 
				  get_word(specialinfo + 2), get_word(specialinfo + 4),
				  get_word(specialinfo + 6), get_word(specialinfo + 8),
				  1);
		    SetGadgetType(jwin->gad[GAD_VERTSCROLL], gadgettype);
		}
	      }
	      if(jwin->prop_left_right) {
		specialinfo=get_long(jwin->prop_left_right->aos3gadget + 34);
		JWLOG("SpecialInfo: HorizPot: %d\n", get_word(specialinfo + 2));
		JWLOG("SpecialInfo: VertPot: %d\n", get_word(specialinfo + 4));
		JWLOG("SpecialInfo: HorizBody: %d\n", get_word(specialinfo + 6));
		JWLOG("SpecialInfo: VertBody: %d\n", get_word(specialinfo + 8));

		if( 
		  ((struct PropInfo *)jwin->gad[GAD_HORIZSCROLL]->SpecialInfo)->HorizPot != get_word(specialinfo + 2) ||
		  ((struct PropInfo *)jwin->gad[GAD_HORIZSCROLL]->SpecialInfo)->VertPot  != get_word(specialinfo + 4) ||
		  ((struct PropInfo *)jwin->gad[GAD_HORIZSCROLL]->SpecialInfo)->HorizBody!= get_word(specialinfo + 6) ||
		  ((struct PropInfo *)jwin->gad[GAD_HORIZSCROLL]->SpecialInfo)->VertBody != get_word(specialinfo + 8)
		  ) {

		  JWLOG("SpecialInfo: NewModifyProp vert ..\n");

		  gadgettype=SetGadgetType(jwin->gad[GAD_HORIZSCROLL], GTYP_PROPGADGET);
		  NewModifyProp(jwin->gad[GAD_HORIZSCROLL], jwin->aroswin, NULL,
				jwin->prop_left_right->flags, 
				get_word(specialinfo + 2), get_word(specialinfo + 4),
				get_word(specialinfo + 6), get_word(specialinfo + 8),
				1);
		  SetGadgetType(jwin->gad[GAD_HORIZSCROLL], gadgettype);
		}
	      }
	    }
	    jwin->prop_update_count--;

	    ReplyMsg ((struct Message *)msg);

	    if(jwin->intui_tickcount) {
	      if(jwin->intui_tickskip++ > jwin->intui_tickspeed) {
		jwin->intui_tickskip=0;
		jwin->intui_tickcount--;
		JWLOG("IDCMP_INTUITICKS %2d refresh_content(%lx)\n", jwin->intui_tickcount, jwin);
		refresh_content(jwin);
	      }
	    }
	  }
	}
	else {

	  handle_msg((struct Message *) msg, aroswin, jwin, class, code, dmx, dmy, mx, my, qualifier, 
		     thread, secs, micros, &done);
	  ReplyMsg ((struct Message *)msg);
	}
      }
    }
    if(signals & SIGBREAKF_CTRL_C) {
      /* Ctrl-C */
      JWLOG("aros_win_thread[%lx]: SIGBREAKF_CTRL_C received\n", thread);
      done=TRUE;
      /* from now on, don't touch it anymore, as it maybe invalid! */
      jwin->dead=TRUE;
    }
  }

  /* ... and a time to die. */
  JWLOG("aros_win_thread[%lx]: time to die!\n", thread);

EXIT:
  JWLOG("aros_win_thread[%lx]: ObtainSemaphore(&sem_janus_window_list)\n", thread);
  ObtainSemaphore(&sem_janus_window_list);

  if(aroswin) {
    JWLOG("aros_win_thread[%lx]: close window %lx\n",thread,aroswin);
    CloseWindow(aroswin);
    JWLOG("aros_win_thread[%lx]: closed window %lx\n",thread,aroswin);
    jwin->aroswin=NULL;
  }

  JWLOG("aros_win_thread[%lx]: scan list_win..\n",thread);
  if(list_win) {
    /*
    *if(list_win->data) {
      * is freed with the pool:
       * name_mem=((JanusWin *)list_win->data)->name;
       * JWLOG("aros_win_thread[%lx]: FreeVec list_win->data \n",thread);
       * FreeVecPooled((JanusWin *)list_win->mempool, list_win->data); 
       *
    *  list_win->data=NULL;
     } 
    */
    JWLOG("aros_win_thread[%lx]: g_slist_remove(%lx)\n",thread,list_win);
    //g_slist_remove(list_win); 
    janus_windows=g_slist_delete_link(janus_windows, list_win);
    list_win=NULL;
  }

  JWLOG("aros_win_thread[%lx]: ReleaseSemaphore(&sem_janus_window_list);\n", thread);
  ReleaseSemaphore(&sem_janus_window_list);

  if(title) {
    FreeVec(title);
  }

  if(jwin) {
    DeletePool(jwin->mempool);
    FreeVec(jwin);
  }
  JWLOG("aros_win_thread[%lx]: dies..\n", thread);
}

int aros_win_start_thread (JanusWin *win) {

    JWLOG("aros_win_start_thread(%lx)\n",win);
    win->name=AllocVecPooled(win->mempool, 8+strlen(TASK_PREFIX_NAME)+1);

    sprintf(win->name,"%s%lx",TASK_PREFIX_NAME,win->aos3win);

    ObtainSemaphore(&aos3_thread_start);
    win->task = (struct Task *)
	    myCreateNewProcTags ( NP_Output, Output (),
				  NP_Input, Input (),
				  NP_Name, (ULONG) win->name,
				  NP_CloseOutput, FALSE,
				  NP_CloseInput, FALSE,
				  NP_StackSize, 4096,
				  NP_Priority, 0,
				  NP_Entry, (ULONG) aros_win_thread,
				  TAG_DONE);

    ReleaseSemaphore(&aos3_thread_start);

    JWLOG("thread %lx created\n",win->task);

    return win->task != 0;
}

#if 0
extern void uae_set_thread_priority (int pri)
{
    SetTaskPri (FindTask (NULL), pri);
}
#endif

