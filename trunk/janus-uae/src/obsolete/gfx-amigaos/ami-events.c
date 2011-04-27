/************************************************************************ 
 *
 * Amiga event handling
 *
 * Copyright 1996,1997,1998 Samuel Devulder.
 * Copyright 2003-2007 Richard Drummond
 * Copyright 2009-2010 Oliver Brunner - aros<at>oliver-brunner.de
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

# ifdef __amigaos4__
#  define __USE_BASETYPE__
# endif

#include <intuition/pointerclass.h>
#include <proto/layers.h>

/****************************************************************************/

#include "sysconfig.h"
#include "sysdeps.h"

#include "td-amigaos/thread.h"

#if defined(CATWEASEL)
#include "catweasel.h"
#endif

//#define JW_ENTER_ENABLED  1
//#define JWTRACING_ENABLED 1
#include "od-amiga/j.h"

#include "uae.h"
#include "gui.h"
#include "debug.h"
#include "hotkeys.h"
#include "ami.h"

#ifdef __amigaos4__ 
static int mouseGrabbed;
static int grabTicks;
#define GRAB_TIMEOUT 50
#endif

/****************************************************************************/

unsigned long            frame_num; /* for arexx */

/****************************************************************************/
/* 
 * in e-uae handle_events always handle the events of the main uae
 * window. j-uae has quite some windows more ;).
 *
 * if we have integrated windows, this is no problem at all. But
 * for custom screens, we need to call the according handle_custom_events_W.
 */
void handle_events(void) {
  JWLOG("handle_events: dispatcher\n", W);

  if(janus_active_screen==NULL) {
    JWLOG("custom_screen_active==NULL\n");
    handle_events_W(W, FALSE);

  }
  else {
    /* custom screens only have one window */
    JWLOG("custom screen %lx, window %lx\n", 
             janus_active_screen->arosscreen, 
             janus_active_screen->arosscreen->FirstWindow);
    handle_events_W(janus_active_screen->arosscreen->FirstWindow, TRUE);
    JWLOG("do nothing, custom screen thread will handle us\n");
  }
}

/***************************************************
 * handle_events_W
 *
 * works with a local W, so it can be called
 * for a window on our own custom screen, for 
 * example
 ***************************************************/
void handle_events_W(struct Window *W, BOOL customscreen) {

    struct IntuiMessage *msg;
    int dmx, dmy, mx, my, class, code, qualifier;

    JWLOG("aros W: %lx\n", W);
    gui_handle_events();

/* janusd now has its own interrupt server! */
#if 0
    if(aos3_task && aos3_task_signal) {
      JWLOG("send signal to Wait of janusd (%lx)\n", aos3_task);
      uae_Signal(aos3_task, aos3_task_signal);
    }
#endif

    JWLOG("W: %lx\n", W);
    if(!W) {
      JWLOG("Warning: W is NULL!\n", W);
      return;
    }

   /* this function is called at each frame, so: */
    ++frame_num;       /* increase frame counter */
#if 0
    save_frame();      /* possibly save frame    */
#endif

#ifdef DEBUGGER
    /*
     * This is a hack to simulate ^C as is seems that break_handler
     * is lost when system() is called.
     */
    if (SetSignal (0L, SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D) &
		(SIGBREAKF_CTRL_C|SIGBREAKF_CTRL_D)) {
      kprintf("SIGBREAKF_CTRL_C|SIGBREAKF_CTRL_D: activate_debugger !!\n");
	activate_debugger ();
    }
#endif

    #ifdef PICASSO96
    if (screen_is_picasso) {
        int i;

	JWLOG("handle_events->screen_is_picasso (aroswin %lx)\n", W);

        picasso_invalid_lines[picasso_invalid_end+1] = 0;
        picasso_invalid_lines[picasso_invalid_end+2] = 1;

    	//o1i_Display_Update(0,0);

	/* o1i TODO: remove loops?*/
	for (i = picasso_invalid_start; i < picasso_invalid_end;)
	{
	    int start = i;

            for (;picasso_invalid_lines[i]; i++)
	        picasso_invalid_lines[i] = 0;

            //DoMethod(uaedisplay, MUIM_UAEDisplay_Update, start, i);
	    //JWLOG("MUIM_UAEDisplay_Update TODO\n");
	    o1i_Display_Update(start,i);

	    for (; !picasso_invalid_lines[i]; i++);
	}

	picasso_invalid_start = picasso_vidinfo.height + 1;
	picasso_invalid_end   = -1;
    }
    else {
      JWLOG("screen_is_picasso == FALSE (aroswin %lx)\n", W);
    }
    #endif

    if(customscreen) {
      JWLOG("custom screen, don't do GetMsg(W %lx ->UserPort %lx)\n", W, W->UserPort);
    }
    else {
      if(!uae_main_window_closed) {
	obtain_W();
	JWLOG("GetMsg(W %lx->UserPort %lx)\n", W, W->UserPort);

	while (!uae_main_window_closed && (msg = (struct IntuiMessage*) GetMsg(W->UserPort))) {
	  class     = msg->Class;
	  code      = msg->Code;
	  dmx       = msg->MouseX;
	  dmy       = msg->MouseY;
	  mx        = msg->IDCMPWindow->MouseX; // Absolute pointer coordinates
	  my        = msg->IDCMPWindow->MouseY; // relative to the window
	  qualifier = msg->Qualifier;

	  ReplyMsg ((struct Message*)msg);

	  JWLOG("W: %lx (%s)\n",W, W->Title);
	  JWLOG("ami-win.c: handle_input(win %lx)\n", W);

	  switch (class) {
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

	      case IDCMP_CLOSEWINDOW:
		  uae_quit ();
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
		/* classic mouse move, if either option is disabled or janusd is not (yet) running */
		if( ((!changed_prefs.jmouse) || (!aos3_task) ) && (!uae_main_window_closed)) {
		  JWLOG("classic mouse move enabled\n");
		  setmousestate (0, 0, dmx, 0);
		  setmousestate (0, 1, dmy, 0);

		  if (usepub) {
		    update_pointer(W, mx, my);
#if 0
		      POINTER_STATE new_state = get_pointer_state (W, mx, my);
		      if (new_state != pointer_state) {
			  pointer_state = new_state;
			  if (pointer_state == INSIDE_WINDOW)
			      hide_pointer (W);
			  else
			      show_pointer (W);
		      }
#endif
		  }
		}
		else {
		  JWLOG("classic mouse move disabled ((%d || %d) && %d)\n", (!changed_prefs.jmouse), (!aos3_task), (!uae_main_window_closed));
		}
		break;

	    case IDCMP_MOUSEBUTTONS:
		if (code == SELECTDOWN) 
		{
			setmousebuttonstate (0, 0, 1);
			gMouseState |= 8;
		}
		if (code == SELECTUP)   
		{
			setmousebuttonstate (0, 0, 0);
			gMouseState &= ~8;
		}
        if (code == MIDDLEDOWN) 
		{
			setmousebuttonstate (0, 2, 1);
			gMouseState |= 4;
		}
		if (code == MIDDLEUP)   
		{
			setmousebuttonstate (0, 2, 0);
			gMouseState &= ~4;
		}
		if (code == MENUDOWN)
		{
			setmousebuttonstate (0, 1, 1);
			gMouseState |= 2;
		}
		if (code == MENUUP)     
		{
			setmousebuttonstate (0, 1, 0);
			gMouseState &= ~2;
		}
		break;

	      /* Those 2 could be of some use later. */
	      case IDCMP_DISKINSERTED:
		  /*printf("diskinserted(%d)\n",code);*/
		  break;

	      case IDCMP_DISKREMOVED:
		  /*printf("diskremoved(%d)\n",code);*/
		  break;

	      case IDCMP_ACTIVEWINDOW:
		  /* When window regains focus (presumably after losing focus at some
		   * point) UAE needs to know any keys that have changed state in between.
		   * A simple fix is just to tell UAE that all keys have been released.
		   * This avoids keys appearing to be "stuck" down.
		   */
		  JWLOG("IDCMP_ACTIVEWINDOW(%lx)\n", W);
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

	      default:
		  write_log ("Unknown event class: %x\n", class);
		  JWLOG("Unknown event class: %x\n", class);
		  break;
	  }
	}

	JWLOG("GetMsg(W->UserPort %lx) done\n", W->UserPort);
	release_W();
      }
    }

    appw_events();
}

/****************************************************************************
 *
 * Keyboard inputdevice functions
 */
static unsigned int get_kb_num (void)
{
    return 1;
}

static const char *get_kb_name (unsigned int kb)
{
    return "Default keyboard";
}

static unsigned int get_kb_widget_num (unsigned int kb)
{
    return 128;
}

static int get_kb_widget_first (unsigned int kb, int type)
{
    return 0;
}

static int get_kb_widget_type (unsigned int kb, unsigned int num, char *name, uae_u32 *code)
{
    // fix me
    *code = num;
    return IDEV_WIDGET_KEY;
}

static int keyhack (int scancode, int pressed, int num)
{
    return scancode;
}

static void read_kb (void)
{
#ifdef CATWEASEL
		uae_u8 kc;
		if (catweasel_read_keyboard (&kc))
		{
			inputdevice_do_keyboard (kc & 0x7f, !(kc & 0x80));
		}
#endif
}

static int init_kb (void)
{
    return 1;
}

static void close_kb (void)
{
}

static int acquire_kb (unsigned int num, int flags)
{
    return 1;
}

static void unacquire_kb (unsigned int num)
{
}

struct inputdevice_functions inputdevicefunc_keyboard =
{
    init_kb,
    close_kb,
    acquire_kb,
    unacquire_kb,
    read_kb,
    get_kb_num,
    get_kb_name,
    get_kb_widget_num,
    get_kb_widget_type,
    get_kb_widget_first
};

int getcapslockstate (void)
{
    return 0;
}

void setcapslockstate (int state)
{
}

