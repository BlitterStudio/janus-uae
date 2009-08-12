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

#include "j.h"
#include "memory.h"

static void handle_msg(struct Window *win, JanusWin *jwin, ULONG class, UWORD code, int dmx, int dmy, WORD mx, WORD my, int qualifier, struct Process *thread, ULONG secs, ULONG micros, BOOL *done);

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

static void handle_msg(struct Window *win, JanusWin *jwin, ULONG class, UWORD code, int dmx, int dmy, WORD mx, WORD my, int qualifier, struct Process *thread, ULONG secs, ULONG micros, BOOL *done) {

  GSList   *list_win;
  JanusMsg *jmsg;
  UWORD selection;

  switch (class) {
      
    case IDCMP_CLOSEWINDOW:
      /* fake IDCMP_CLOSEWINDOW to original aos3 window */
      JWLOG("aros_win_thread[%lx]: CLOSEWINDOW received for jwin %lx (%s)\n", 
              thread, jwin, win->Title);

      ObtainSemaphore(&janus_messages_access);
      /* this gets freed in ad_job_fetch_message! */
      jmsg=AllocVec(sizeof(JanusMsg), MEMF_CLEAR); 
      if(!jmsg) {
	JWLOG("ERROR: no memory (ignored message)\n");
	break;
      }
      jmsg->jwin=jwin;
      jmsg->type=J_MSG_CLOSE;
      janus_messages=g_slist_append(janus_messages, jmsg);
      ReleaseSemaphore(&janus_messages_access);

      break;

    case IDCMP_CHANGEWINDOW:
      /* ChangeWindowBox is not done at once, but deferred. You 
       * can detect that the operation has completed by receiving 
       * the IDCMP_CHANGEWINDOW IDCMP message
       */
      JWLOG("aros_win_thread[%lx]: IDCMP_CHANGEWINDOW: received\n", thread);
      if(jwin->delay > WIN_DEFAULT_DELAY) {
	jwin->delay=0;
      }
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

    case IDCMP_MOUSEMOVE:
	JWLOG("WARNING: IDCMP_MOUSEMOVE *not* handled\n");
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

    case IDCMP_MOUSEBUTTONS:
	if (code == SELECTDOWN) setmousebuttonstate (0, 0, 1);
	if (code == SELECTUP)   setmousebuttonstate (0, 0, 0);
	if (code == MIDDLEDOWN) setmousebuttonstate (0, 2, 1);
	if (code == MIDDLEUP)   setmousebuttonstate (0, 2, 0);
	if (code == MENUDOWN)   setmousebuttonstate (0, 1, 1);
	if (code == MENUUP)     setmousebuttonstate (0, 1, 0);
	break;

    case IDCMP_MENUVERIFY:
	if(IntuitionBase->ActiveWindow != win) {
	  /* this seems to be a bug in aros, why are we getting those messages at all !? */
	  JWLOG("WARNING: foreign IDCMP message IDCMP_MENUVERIFY received\n");
	}
	else {
	  JWLOG("IDCMP_MENUVERIFY\n");
	  menux=0;
	  menuy=0;
	  j_stop_window_update=TRUE;
	  mice[0].enabled=FALSE; /* disable mouse emulation */
	  my_setmousebuttonstate(0, 1, 1); /* MENUDOWN */
	  clone_menu(jwin);
	}
	break;

    case IDCMP_MENUPICK:
	JWLOG("IDCMP_MENUPICK\n");

	/* nothing selected, but this could mean, the user clicked twice very fast and
	 * thus wanted to activate a DMRequest. So we remember/check the time of the last
	 * click and replay it, if appropriate 
	 */
	if(code==MENUNULL) {
	  if(jwin->micros) {
	    JWLOG("MENUNULL difference: %d\n", olisecs(secs, micros) - olisecs(jwin->secs, jwin->micros));
	    if(olisecs(secs, micros) - olisecs(jwin->secs, jwin->micros) < 1500) {
	      JWLOG("MENUNULL DOUBLE!!\n");
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
	break;

    case IDCMP_ACTIVEWINDOW:
	/* When window regains focus (presumably after losing focus at some
	 * point) UAE needs to know any keys that have changed state in between.
	 * A simple fix is just to tell UAE that all keys have been released.
	 * This avoids keys appearing to be "stuck" down.
	 */

	JWLOG("IDCMP_ACTIVEWINDOW(%lx, %s)\n", win, win->Title);
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
	JWLOG("janus_active_window=%lx\n", janus_active_window);
	ReleaseSemaphore(&sem_janus_active_win);

	break;

    /* there might be a race.. ? */
    case IDCMP_INACTIVEWINDOW: {
	JanusWin *old;

	JWLOG("IDCMP_INACTIVEWINDOW(%lx, %s)\n", win, win->Title);
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
	  JWLOG("janus_active_window=NULL\n");
	  copy_clipboard_to_aros();
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
	JWLOG("aros_win_thread[%lx]: Unknown IDCMP class %lx received\n", thread, class);
	break;
  }
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
  ULONG           aos_title;
  WORD            h,w,x,y;
  WORD            minh,minw,maxh,maxw;
  BYTE            br,bl,bt,bb;
  char            title[256];
  ULONG           flags;
  ULONG           idcmpflags;
  int             i;
  char            c;
  ULONG           signals;
  BOOL            done;
  UWORD           code;
  WORD            mx, my;
  ULONG           class;
  ULONG           gadget;
  UWORD           gadget_flags;
  UWORD           gadget_type;
  struct IntuiMessage *msg;
  int dmx, dmy, qualifier;
  ULONG           secs, micros;
  BOOL            care=FALSE;

  /* There's a time to live .. */

  JWLOG("aros_win_thread[%lx]: thread running \n",thread);

  /* I think, the sem would be better, but seems, as if we are running
   * into a deadlock sometimes..?
   */
  JWLOG("ObtainSemaphore(&sem_janus_window_list);\n");
  ObtainSemaphore(&sem_janus_window_list);
  JWLOG("aros_win_thread[%lx]: obtained sem_janus_window_list sem \n",thread);

  /* who are we? */
  list_win=g_slist_find_custom(janus_windows, 
			     	(gconstpointer) thread,
			     	&aos3_process_compare);

  JWLOG("ReleaseSemaphore(&sem_janus_window_list)\n");
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

    /* AROS and Aos3 use the same flags */
    flags     =get_long_p(jwin->aos3win + 24); 
    JWLOG("aros_win_thread[%lx]: flags: %lx \n", thread, flags);

    /* wrong offset !?
     * idcmpflags=get_long(jwin->aos3win + 80);
     */

    x=get_word((ULONG) jwin->aos3win +  4);
    y=get_word((ULONG) jwin->aos3win +  6);

    aos_title=get_long_p(jwin->aos3win + 32);

    //JWLOG("TITLE: aos_title=%lx\n",aos_title);

    c='X';
    for(i=0;i<255 && c;i++) {
      c=get_byte(aos_title + i);
      //JWLOG("TITLE: %d: %c\n",i,c);
      title[i]=c;
    }
    title[i]=(char) 0;
    JWLOG("aros_win_thread[%lx]: title: >%s<\n",thread,title);

    /* idcmp flags we always need: */
    idcmpflags= IDCMP_NEWSIZE | 
			    IDCMP_CLOSEWINDOW | 
			    IDCMP_RAWKEY |
			    IDCMP_MOUSEBUTTONS |
			    //IDCMP_MOUSEMOVE |
			    IDCMP_ACTIVEWINDOW |
			    IDCMP_CHANGEWINDOW |
			    IDCMP_MENUPICK |
			    IDCMP_MENUVERIFY |
			    IDCMP_INACTIVEWINDOW;

    JWLOG("aros_win_thread[%lx]: idcmpflags: %lx \n", thread, idcmpflags);

    /* we are always WFLG_SMART_REFRESH and never BACKDROP! */
    flags=flags & 0xFFFFFEFF;  /* remove refresh bits and backdrop */
    flags=flags | WFLG_SMART_REFRESH | WFLG_GIMMEZEROZERO | WFLG_ACTIVATE;
    
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

    bl=get_byte((ULONG) jwin->aos3win + 54);
    bt=get_byte((ULONG) jwin->aos3win + 55);
    br=get_byte((ULONG) jwin->aos3win + 56);
    bb=get_byte((ULONG) jwin->aos3win + 57);

    gadget=get_long_p(jwin->aos3win + 62);
    JWLOG("aros_win_thread[%lx]: ============= gadget =============\n",thread);
    JWLOG("aros_win_thread[%lx]: gadget window: %s\n",thread,title);
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

    JWLOG("jwin: %lx\n",jwin);
    JWLOG("jwin->jscreen: %lx\n",jwin->jscreen);
    JWLOG("jwin->jscreen->arosscreen: %lx\n",jwin->jscreen->arosscreen);

    if(jwin->jscreen->arosscreen) {
      /* now we need to open up the window .. 
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
				      TAG_DONE);
    }

    JWLOG("opened window: %s\n", title);
    JWLOG("  WA_Left: x %d - estimated_border_left %d = %d\n", x, estimated_border_left, x-estimated_border_left);


    aroswin=jwin->aroswin; /* shorter to read..*/

    JWLOG("aros_win_thread[%lx]: aroswin: %lx\n",thread, aroswin);

    if(!aroswin) {
      JWLOG("aros_win_thread[%lx]: ERROR: OpenWindow FAILED!\n",thread);
      goto EXIT;
    }

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
    estimated_border_top=aroswin->BorderTop;
    estimated_border_bottom=aroswin->BorderBottom;
    estimated_border_left=aroswin->BorderLeft;
    estimated_border_right=aroswin->BorderRight;

    /* handle IDCMP stuff */
    done=FALSE;
    JWLOG("IDCMP loop for window %lx\n",aroswin);

    /* should not happen.. */
    if(!aroswin->UserPort) {
      JWLOG("aros_win_thread[%lx]: ERROR: win %lx has no UserPort !?!\n",
	      thread,aroswin);

      done=TRUE;
    }
  } /* endif !win->custom
     * yes, I know, so long if's are evil
     */
  else {
    JWLOG("aros_win_thread[%lx]: we are a custom win thread!\n", thread);
  }

  while(!done) {
#if 0
    sleep(100);
#endif
    if(!jwin->custom) {
      signals = Wait(1L << aroswin->UserPort->mp_SigBit | SIGBREAKF_CTRL_C);

    /* message */
      if (signals & (1L << aroswin->UserPort->mp_SigBit)) {
	while (NULL != 
	       (msg = (struct IntuiMessage *)GetMsg(aroswin->UserPort))) {
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

	  ReplyMsg ((struct Message*)msg);

	  handle_msg(aroswin, jwin, class, code, dmx, dmy, mx, my, qualifier, 
		     thread, secs, micros, &done);
	}
      }
    }
    else {
      /* custom */
      signals = Wait(SIGBREAKF_CTRL_C);
    }
    /* Ctrl-C */
    if(signals & SIGBREAKF_CTRL_C) {
      JWLOG("aros_win_thread[%lx]: SIGBREAKF_CTRL_C received\n", thread);
      done=TRUE;
    }
  }

  /* ... and a time to die. */

EXIT:
  JWLOG("ObtainSemaphore(&sem_janus_window_list)\n");
  ObtainSemaphore(&sem_janus_window_list);

  if(aroswin) {
    JWLOG("aros_win_thread[%lx]: close window %lx\n",thread,aroswin);
    CloseWindow(aroswin);
    jwin->aroswin=NULL;
  }

  if(list_win) {
    if(list_win->data) {
      name_mem=((JanusWin *)list_win->data)->name;
      /* is freed with the pool:
       * JWLOG("aros_win_thread[%lx]: FreeVec list_win->data \n",thread);
       * FreeVecPooled((JanusWin *)list_win->mempool, list_win->data); 
       */
      list_win->data=NULL;
    }
    JWLOG("aros_win_thread[%lx]: g_slist_remove(%lx)\n",thread,list_win);
    //g_slist_remove(list_win); 
    janus_windows=g_slist_delete_link(janus_windows, list_win);
    list_win=NULL;
  }

  /* not nice to free our own name, but should not be a problem */
#if 0
  if(name_mem) {
    JWLOG("aros_win_thread[%lx]: FreeVec() >%s<\n", thread,
	    name_mem);
    FreeVecPooled(jwin->mempool, name_mem);
  }
#endif

  JWLOG("ReleaseSemaphore(&sem_janus_window_list);\n");
  ReleaseSemaphore(&sem_janus_window_list);
  DeletePool(jwin->mempool);
  FreeVec(jwin);
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

