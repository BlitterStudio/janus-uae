
#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>

#include "aroswin.h"
#include "threaddep/thread.h"

//#define JWTRACING_ENABLED 1
#if JWTRACING_ENABLED
#define JWLOG(...)   do { kprintf("JW: ");kprintf(__VA_ARGS__); } while(0)
#else
#define JWLOG(...)     do { ; } while(0)
#endif

/* we need that, to wake up the aos3 high pri daemon */
ULONG aos3_task=0;
ULONG aos3_task_signal=0;

/* access the JanusWin list */
struct SignalSemaphore sem_janus_window_list;

/* wait until thread is allowed to start */
struct SignalSemaphore aos3_thread_start;

/* wait until thread is allowed to start */
struct SignalSemaphore sem_janus_active_win;

/* protect aos3_messages list */
struct SignalSemaphore janus_messages_access;

GSList *janus_messages;

static struct Window *assert_window (struct Window *search) {

  struct Window *win;
  struct Screen *screen;

  screen=LockPubScreen(NULL);
  win=screen->FirstWindow;
  UnlockPubScreen(NULL,screen);

  while(win) {
    if(search==win) {
      return win;
    }
    win=win->NextWindow;
  }

  JWLOG("assert_window(%lx) failed\n",search);

  return NULL;
}

static gint aos3_process_compare(gconstpointer aos3win, gconstpointer t) {

  if(((JanusWin *)aos3win)->task == t) {
    return 0;
  }
  return 1; /* don't care for sorting here */
}

static gint aos3_window_compare(gconstpointer aos3win, gconstpointer w) {

  if(((JanusWin *)aos3win)->aos3win == w) {
    return 0;
  }
  return 1; /* don't care for sorting here */
}

static gint aros_window_compare(gconstpointer aos3win, gconstpointer w) {

  if(((JanusWin *)aos3win)->aroswin == w) {
    return 0;
  }
  return 1; /* don't care for sorting here */
}



/* we should definately split it up more .. */


#if 0
	  case IDCMP_NEWSIZE: {
	    JanusMsg *j;

	    JWLOG("aros_win_thread[%lx]: IDCMP_NEWSIZE: win %lx: %dx%d\n",
	                                              thread,
	                                              aroswin,
	                                              aroswin->Width,
	                                              aroswin->Height);
	    j=(JanusMsg *) AllocVec(sizeof(JanusMsg),MEMF_CLEAR);
	    if(!j) {
	      JWLOG("aros_win_thread[%lx]: ERROR: AllocVec return NULL \n",
	                                              thread);
	      break;
	    }

	    /* init j */
	    j->aroswin=aroswin;
	    j->aos3win=win->aos3win;
	    j->arosmsg=(struct IntuiMessage *) AllocVec(sizeof(struct IntuiMessage),MEMF_CLEAR);
	    memcpy(j->arosmsg, msg, sizeof(struct IntuiMessage));

	    /* add j to the queue */
	    ObtainSemaphore(&janus_messages_access);
	    janus_messages=g_slist_append(janus_messages, j);
	    ReleaseSemaphore(&janus_messages_access);

	    break;
	  }
	}
	ReplyMsg((struct Message *)msg);
      }
#endif

/***********************************************************
 * every AOS3 window gets its own process
 * to open an according AROS window and
 * to report size changes, keystrokes etc. to AOS3
 ***********************************************************/  


/* TODO: correct data types */

JanusWin *janus_active_window=NULL;

void handle_msg(struct Window *win, ULONG class, UWORD code, int dmx, int dmy, WORD mx, WORD my, int qualifier, struct Process *thread, BOOL *done) {

  GSList *list_win;

  switch (class) {
      
    case IDCMP_CLOSEWINDOW:
    /* TODO: NO!!!!!!!!!!!!!!!! */
    /* fake IDCMP_CLOSEWINDOW to original aos3 window!! TODO!! */
	JWLOG("aros_win_thread[%lx]: IDCMP_CLOSEWINDOW received\n", thread);
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
	list_win=g_slist_find_custom(janus_windows,
                               (gconstpointer) win,
                               &aros_window_compare);
	janus_active_window=(JanusWin *) list_win->data;
	JWLOG("janus_active_window=%lx\n",list_win->data);
	ReleaseSemaphore(&sem_janus_active_win);

	break;

    /* there might be a race.. ? */
    case IDCMP_INACTIVEWINDOW: {
	JanusWin *old;
	sleep(1);
	ObtainSemaphore(&sem_janus_active_win);
	list_win=g_slist_find_custom(janus_windows,
                               (gconstpointer) win,
                               &aros_window_compare);
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
	JWLOG("aros_win_thread[%lx]: Unknown IDCMP class %lx received\n", thread, class);
	break;
  }
}

/* we don't know those values, but we always keep the last value here */
UWORD estimated_border_top=25;
UWORD estimated_border_bottom=25;
UWORD estimated_border_left=25;
UWORD estimated_border_right=25;

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
  BOOL            care;

  /* There's a time to live .. */

  JWLOG("aros_win_thread[%lx]: thread running \n",thread);

  /* I think, the sem would be better, but seems, as if we are running
   * into a deadlock sometimes..?
   */
  //ObtainSemaphore(&aos3_thread_start);
  ObtainSemaphore(&sem_janus_window_list);
  JWLOG("aros_win_thread[%lx]: obtained sem_janus_window_list sem \n",thread);

#if 0
  i=0;
  /* who are we? */
  while(!list_win && i<5) {
#endif
    list_win=g_slist_find_custom(janus_windows, 
			     	(gconstpointer) thread,
			     	&aos3_process_compare);
#if 0
    if(!list_win) {
      JWLOG("aros_win_thread[%lx]: sleep until we find us (%d)\n",thread,i);
      sleep(1);
    }
    i++;
  }
#endif

  ReleaseSemaphore(&sem_janus_window_list);
  JWLOG("aros_win_thread[%lx]: released sem_janus_window_list sem \n",thread);
  //ReleaseSemaphore(&aos3_thread_start);

  if(!list_win) {
    JWLOG("aros_win_thread[%lx]: ERROR: window of this task not found in window list !?\n",thread);
    return; 
  }

  jwin=(JanusWin *) list_win->data;
  JWLOG("aros_win_thread[%lx]: win: %lx \n",thread,jwin);
  JWLOG("aros_win_thread[%lx]: win->aos3win: %lx\n",thread,
                                                              jwin->aos3win);

  /* now let's hope, the aos3 window is not closed already..? */

  /* AROS and Aos3 use the same flags */
  flags     =get_long(jwin->aos3win + 24); 
  JWLOG("aros_win_thread[%lx]: flags: %lx \n", thread, flags);

  /* wrong offset !?
   * idcmpflags=get_long(jwin->aos3win + 80);
   */

  x=get_word(jwin->aos3win +  4);
  y=get_word(jwin->aos3win +  6);

  aos_title=get_long(jwin->aos3win + 32);

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
			  IDCMP_MOUSEMOVE |
			  IDCMP_ACTIVEWINDOW |
			  IDCMP_INACTIVEWINDOW;

  JWLOG("aros_win_thread[%lx]: idcmpflags: %lx \n", thread, idcmpflags);

  /* we are always WFLG_SMART_REFRESH and never BACKDROP! */
  flags=flags & 0xFFFFFEFF;  /* remove refresh bits and backdrop */
  flags=flags | WFLG_SMART_REFRESH | WFLG_GIMMEZEROZERO | WFLG_ACTIVATE;
  
  minw=get_word(jwin->aos3win + 16); /* CHECKME: need borders here, too? */
  minh=get_word(jwin->aos3win + 18);
  maxw=get_word(jwin->aos3win + 20);
  maxh=get_word(jwin->aos3win + 22);

  w=get_word(jwin->aos3win +  8);
  h=get_word(jwin->aos3win + 10);

  bl=get_byte(jwin->aos3win + 54);
  bt=get_byte(jwin->aos3win + 55);
  br=get_byte(jwin->aos3win + 56);
  bb=get_byte(jwin->aos3win + 57);

  gadget=get_long(jwin->aos3win + 62);
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

  /* now we need to open up the window .. 
   * hopefully nobody has thicker borders  ..
   */
   jwin->aroswin =  OpenWindowTags(NULL,WA_Title, title,
      				    WA_Left, x - estimated_border_left,
      				    WA_Top, y - estimated_border_top,
      				    WA_Width, w - br - bl + 
				              estimated_border_left +
					      estimated_border_right +
					      jwin->plusx,
					      /* see below */
				    WA_Height, h - bt - bb + 
				              estimated_border_top +
					      estimated_border_bottom +
					      jwin->plusy,
				    WA_MinWidth, minw + jwin->plusx,
				    WA_MinHeight, minh + jwin->plusy,
				    WA_MaxWidth, maxw + jwin->plusx,
				    WA_MaxHeight, maxh + jwin->plusy,
				    WA_SmartRefresh, TRUE,
				    WA_GimmeZeroZero, TRUE,
				    WA_Flags, flags,
				    WA_IDCMP, idcmpflags,
                                    TAG_DONE);

  aroswin=jwin->aroswin; /* shorter to read..*/

  JWLOG("aros_win_thread[%lx]: aroswin: %lx\n",thread, aroswin);

  if(!aroswin) {
    JWLOG("aros_win_thread[%lx]: ERROR: OpenWindow FAILED!\n",thread);
    return;
  }

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

  while(!done) {
#if 0
    sleep(100);
#endif
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

	ReplyMsg ((struct Message*)msg);

	handle_msg(aroswin, class, code, dmx, dmy, mx, my, qualifier, 
	           thread, &done);
      }
    }
    /* Ctrl-C */
    if(signals & SIGBREAKF_CTRL_C) {
      JWLOG("aros_win_thread[%lx]: SIGBREAKF_CTRL_C received\n", thread);
      done=TRUE;
    }
  }

  /* ... and a time to die. */

  ObtainSemaphore(&sem_janus_window_list);

  if(aroswin) {
    JWLOG("aros_win_thread[%lx]: close window %lx\n",thread,aroswin);
    CloseWindow(aroswin);
    jwin->aroswin=NULL;
  }

  if(list_win) {
    if(list_win->data) {
      name_mem=((JanusWin *)list_win->data)->name;
      JWLOG("aros_win_thread[%lx]: FreeVec list_win->data \n",thread);
      FreeVec(list_win->data); 
      list_win->data=NULL;
    }
    JWLOG("aros_win_thread[%lx]: g_slist_remove(%lx)\n",thread,list_win);
    //g_slist_remove(list_win); 
    janus_windows=g_slist_delete_link(janus_windows, list_win);
    list_win=NULL;
  }

  /* not nice to free our own name, but should not be a problem */
  if(name_mem) {
    JWLOG("aros_win_thread[%lx]: FreeVec() >%s<\n", thread,
	    name_mem);
    FreeVec(name_mem);
  }

  ReleaseSemaphore(&sem_janus_window_list);
  JWLOG("aros_win_thread[%lx]: dies..\n", thread);
}

static int aros_win_start_thread (JanusWin *win) {

    JWLOG("AD_GET_JOB_LIST_WINDOWS: aros_win_start_thread(%lx)\n",win);
    win->name=AllocVec(8+strlen(TASK_PREFIX_NAME)+1,MEMF_CLEAR);

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

    JWLOG("AD_GET_JOB_LIST_WINDOWS: thread %lx created\n",win->task);

    return win->task != 0;
}

#if 0
extern void uae_set_thread_priority (int pri)
{
    SetTaskPri (FindTask (NULL), pri);
}
#endif

APTR m68k_win;

#define GET_JOB_RESULTSIZE 32*4

/*********************************************************
 *
 * The GET_JOB magic (how it should work)
 * 
 * It is not trivial (at least not for me) to call
 * amigaOS functions from the x86 uae code. The other
 * way is working without problems with the help of
 * emulator traps.
 *
 * So to call 68k functions, you just place a "job"
 * in the queue, which a 68k-slave process (aros-daemon)
 * pulls.
 * The Slave then tries to do the job.
 *
 * with uae_Signal you can easily send a signal
 * to an aos3 task (our daemon). At setup, the
 * daemon told us his task and his selected signal.
 *
 * A emulator trap causes aroshack_helper to
 * be run. If D0 == GET_JOBS, D1 decides,
 * which job to do.
 *
 * D0: primary group (GET_JOB, ..)
 * D1: secondary group (GET_JOB_RESIZE, ..)
 * A0: memory block for parameters/results
 *
 *********************************************************/

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
 *
 * ATTENTION: you have to care for sem_janus_window_list
 *            access yourself!
 ********************************************************/
static void new_aos3window(ULONG aos3win) {
  JanusWin *jwin;
  int found;
  GSList *elem;

  if(g_slist_find_custom(janus_windows, 
			  (gconstpointer) aos3win,
			  &aos3_window_compare)) {
    /* we already have this window */
    return;
  }

  jwin=AllocVec(sizeof(JanusWin),MEMF_CLEAR);
  JWLOG("AD_GET_JOB_LIST_WINDOWS: append aos3window %lx as %lx\n",
	   aos3win,
	   jwin);
  jwin->aos3win=aos3win;
  janus_windows=g_slist_append(janus_windows,jwin);

  aros_win_start_thread(jwin);
}

/*********************************************************
 * ad_job_update_janus_windows(aos3 window array)
 *
 * for every window in the array call new_aos3window(win)
 *
 * this is called at janus-daemon startup once
 *
 *********************************************************/
static uae_u32 ad_job_update_janus_windows(ULONG *m68k_results) {
  int i=0;

  ObtainSemaphore(&sem_janus_window_list);
  while(get_long(m68k_results+i)) {
    new_aos3window(get_long(m68k_results+i));
    i++;
  }
  ReleaseSemaphore(&sem_janus_window_list);

  return TRUE;
}

WORD get_lo_word(ULONG *field) {
  ULONG l;

  l=get_long(field);
  l=l & 0xFFFF;
  return (WORD) l;
}

WORD get_hi_word(ULONG *field) {
  ULONG l;

  l=get_long(field);
  l=l & 0xFFFF0000;
  l=l/0x10000;
  return (WORD) l;
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

static uae_u32 ad_job_report_uae_windows(ULONG *m68k_results) {

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
    JWLOG("ad_job_report_uae_windows: window=%lx\n",aos3win);
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

static uae_u32 ad_job_report_host_windows(ULONG *m68k_results) {

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

static uae_u32 ad_job_mark_window_dead(ULONG aos_window) {
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

static uae_u32 ad_job_get_mouse(ULONG *m68k_results) {
  struct Screen *screen;

  JWLOG("ad_job_get_mouse\n");

  /* only return mouse movement, if one of our windows is active.
   * as we do not access the result, no Semaphore access is
   * necesary. save semaphore access.
   */
  if(!janus_active_window) {
    return FALSE;
  }

  screen=(struct Screen *) LockPubScreen(NULL);
  put_long(m68k_results, screen->MouseX); 
  put_long(m68k_results+1, screen->MouseY); 
  UnlockPubScreen(NULL,screen);

  return TRUE;
}

BOOL uae_main_window_closed=FALSE;

static uae_u32 ad_job_switch_uae_window(ULONG *m68k_results) {

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
static uae_u32 ad_job_sync_windows(ULONG *m68k_results) {

  struct Layer  *layer;
  struct Window *window;
  struct Window *last_window;
  struct Screen *screen;
  GSList        *list_win;
  JanusWin       *win;
  ULONG          i;

  screen=(struct Screen *) LockPubScreen(NULL);

  if(!screen) {
    JWLOG("ad_job_sync_windows: screen==NULL !?!\n");
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
    UnlockPubScreen(NULL, screen);
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

  UnlockPubScreen(NULL, screen);
  JWLOG("ad_job_sync_windows left\n");
  return TRUE;
}

/**********************************************************
 * ad_job_fetch_message
 *
 * This is the aros-daemon calling us, to get his next
 * message. if we put a 0 in the first long of the results,
 * no messages are waiting.
 **********************************************************/
int wait=0;
static uae_u32 ad_job_fetch_message(ULONG *m68k_results) {
  JanusMsg *j;

  JWLOG("ad_job_fetch_message\n");

  ObtainSemaphore(&janus_messages_access);

  if(janus_messages) {
    j=(JanusMsg *) g_slist_nth_data(janus_messages, 0);

    /* THIS IS NOT USED ATM, TODO !!*/
  }
  else {
    put_long(m68k_results,0); /* nothing to do */
  }

  ReleaseSemaphore(&janus_messages_access);

  return TRUE;
}

/**********************************************************
 * ad_job_active_window
 *
 * return aos3 window, which should be active
 **********************************************************/
static uae_u32 ad_job_active_window(ULONG *m68k_results) {
  uae_u32 win;

  ObtainSemaphore(&sem_janus_active_win);

  if(!janus_active_window) {
    win=NULL;
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
static uae_u32 ad_job_new_window(ULONG aos3win) {
  uae_u32 win;

  JWLOG("ad_job_new_window(aos3win %lx)\n",aos3win);

  ObtainSemaphore(&sem_janus_active_win);
  new_aos3window(aos3win);
  ReleaseSemaphore(&sem_janus_active_win);

  return TRUE;
}

/**********************************************************
 * this stuff gets called from the aros_daemon after he
 * received a signal
 **********************************************************/
uae_u32 REGPARAM2 aroshack_helper (TrapContext *context) {

  unsigned long service;
  char *source;
  char *windowtitle;
  char c;
  int i=0;
  APTR m68k_bm;
  struct BitMap *aos_bm;
  struct RastPort *rport;
  struct Task *clone_task;

  service = m68k_dreg (&context->regs, 0);

  //JWLOG("aroshack_helper: service %d\n",service);

  //t=get_long(m68k_dreg (&context->regs, 1));
  
  switch(service) {
    /* the aros_daemon gets ready to serve */
    case AD_SETUP: {
      ULONG *param= m68k_areg(&context->regs, 0);

      JWLOG("::::::::::::::AD_SETUP::::::::::::::::::::::::\n");
      JWLOG("AD__MAXMEM: %d\n", m68k_dreg(&context->regs, 1));
      JWLOG("param:      %lx\n",m68k_areg(&context->regs, 0));
      JWLOG("Task:       %lx\n",get_long(param  ));
      JWLOG("Signal:     %lx\n",get_long(param+4));
      aos3_task=get_long(param);
      aos3_task_signal=get_long(param+4);

      InitSemaphore(&sem_janus_window_list);
      InitSemaphore(&aos3_thread_start);
      InitSemaphore(&janus_messages_access);
      InitSemaphore(&sem_janus_active_win);

      /* from now on (aos3_task && aos3_task_signal) the
       * aos3 aros-daemon is ready to take orders!
       */

      /* setup clone window task */
#if 0
      clone_task = (struct Task *)
	    myCreateNewProcTags ( NP_Output, Output (),
				  NP_Input, Input (),
				  NP_Name, (ULONG) "Janus Clone Task",
				  NP_CloseOutput, FALSE,
				  NP_CloseInput, FALSE,
				  NP_StackSize, 4096,
				  NP_Priority, 0,
				  NP_Entry, (ULONG) o1i_clone_windows_task,
				  TAG_DONE);
#endif


      return TRUE;
      break;
    };

    case AD_SHUTDOWN: {

      /* from now on (!aos3_task && !aos3_task_signal) the
       * aos3 aros-daemon is not ready any more to take orders!
       *
       * TODO: There might be race conditons here ? 
       */

      aos3_task=NULL;
      aos3_task_signal=NULL;

      return TRUE;
      break;
    };

    /* the aros_daemon wants its orders*/
    case AD_GET_JOB: {
      ULONG job=m68k_dreg(&context->regs, 1);
      LONG *m68k_results=m68k_areg(&context->regs, 0);

      //JWLOG("::::::::::::::AD_GET_JOB::::::::::::::::::::::::\n");

      switch(job) {
    	case AD_GET_JOB_LIST_WINDOWS: 
       	  return ad_job_update_janus_windows(m68k_results);
    	case AD_GET_JOB_SYNC_WINDOWS: 
       	  return ad_job_sync_windows(m68k_results);
    	case AD_GET_JOB_REPORT_UAE_WINDOWS: 
       	  return ad_job_report_uae_windows(m68k_results);
    	case AD_GET_JOB_REPORT_HOST_WINDOWS: 
       	  return ad_job_report_host_windows(m68k_results);
    	case AD_GET_JOB_MARK_WINDOW_DEAD: 
       	  return ad_job_mark_window_dead((ULONG) m68k_results);
    	case AD_GET_JOB_GET_MOUSE: 
       	  return ad_job_get_mouse(m68k_results);
    	case AD_GET_JOB_SWITCH_UAE_WINDOW: 
       	  return ad_job_switch_uae_window(m68k_results);
    	case AD_GET_JOB_ACTIVE_WINDOW: 
       	  return ad_job_active_window(m68k_results);
    	case AD_GET_JOB_NEW_WINDOW: 
       	  return ad_job_new_window((ULONG) m68k_results);


#if 0
    	case AD_GET_JOB_MESSAGES: 
       	  return ad_job_fetch_message(m68k_results);
#endif

	default:
	  JWLOG("ERROR!! aroshack_helper: unkown job: %d\n",m68k_dreg(&context->regs, 1));
	  return FALSE;
      }
    }

    default:
      JWLOG("ERROR!! aroshack_helper: unkown service %d\n",service);
      return FALSE;
  }

}
