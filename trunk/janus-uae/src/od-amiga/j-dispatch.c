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

#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>

//#include "sysconfig.h"
//#include "sysdeps.h"
//#include "options.h"

//#include "inputdevice.h"
#include "j.h"

BOOL init_done=FALSE;

/* this is disabled! TODO! */
struct Window *assert_window (struct Window *search) {

  return search;
#if 0
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
#endif
}

gint aos3_process_compare(gconstpointer aos3win, gconstpointer t) {

  if(((JanusWin *)aos3win)->task == t) {
    return 0;
  }
  return 1; /* don't care for sorting here */
}

gint aos3_screen_process_compare(gconstpointer jscreen, gconstpointer t) {

  if(((JanusScreen *)jscreen)->task == t) {
    return 0;
  }
  return 1; /* don't care for sorting here */
}


gint aos3_window_compare(gconstpointer aos3win, gconstpointer w) {

  if(((JanusWin *)aos3win)->aos3win == w) {
    return 0;
  }
  return 1; /* don't care for sorting here */
}

gint aros_window_compare(gconstpointer aos3win, gconstpointer w) {

  if(((JanusWin *)aos3win)->aroswin == w) {
    return 0;
  }
  return 1; /* don't care for sorting here */
}

gint aos3_screen_compare(gconstpointer jscreen, gconstpointer s) {

  //JWLOG("screen: %lx == %lx ?\n",((JanusScreen *)jscreen)->aos3screen,s);

  if(((JanusScreen *)jscreen)->aos3screen == s) {
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



//APTR m68k_win;

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

WORD get_lo_word(ULONG *field) {
  ULONG l;

  l=get_long_p(field);
  l=l & 0xFFFF;
  return (WORD) l;
}

WORD get_hi_word(ULONG *field) {
  ULONG l;

  l=get_long_p(field);
  l=l & 0xFFFF0000;
  l=l/0x10000;
  return (WORD) l;
}

static uae_u32 ad_job_get_mouse(ULONG *m68k_results) {
  struct Screen *screen;

  //JWLOG("ad_job_get_mouse\n");

  /* only return mouse movement, if one of our windows is active.
   * as we do not access the result, no Semaphore access is
   * necessary. save semaphore access.
   */
  if(!janus_active_window) {
    return FALSE;
  }

  if(mice[0].enabled) {
    screen=IntuitionBase->FirstScreen;
  
    put_long_p(m68k_results, screen->MouseX); 
    put_long_p(m68k_results+1, screen->MouseY); 
  }
  else {
    if(!menux && !menuy) {
      return FALSE;
    }
    /* fake for menu selection */
    put_long_p(m68k_results,   menux); 
    put_long_p(m68k_results+1, menuy); 
    /* clear again */
    menux=0;
    menuy=0;
  }

  return TRUE;
}


/**********************************************************
 * ad_job_fetch_message
 *
 * This is the aros-daemon calling us, to get his next
 * message. if we put a 0 in the first long of the results,
 * no messages are waiting.
 *
 * Only return one message per call, the janus-daemon
 * will keep calling, as long as he gets messages
 **********************************************************/
int wait=0;
static uae_u32 ad_job_fetch_message(ULONG *m68k_results) {
  JanusMsg *j;

  ObtainSemaphore(&janus_messages_access);

  /* check, if we already sent that messages */
  if(janus_messages) {
    j=(JanusMsg *) janus_messages->data;
    if(j->old) {
      janus_messages=g_slist_remove_link(janus_messages, janus_messages);
      //janus_messages=g_list_remove(janus_messages, j);
      FreeVec(j);
    }
  }

  /* send new message and mark it old */
  if(janus_messages) {
    j=(JanusMsg *) janus_messages->data;
    //j=(JanusMsg *) g_slist_nth_data(janus_messages, 0);
    put_long((ULONG) (m68k_results    ), (ULONG) j->type);
    put_long((ULONG) (m68k_results + 1), (ULONG) j->jwin->aos3win);
    j->old=TRUE;
  }
  else {
    put_long_p(m68k_results,0); /* nothing to do */
  }

  ReleaseSemaphore(&janus_messages_access);

  return TRUE;
}


static uae_u32 ad_test(ULONG *m68k_results) {
  JWLOG("\nAD_TEST AD_TEST AD_TEST\n\n");
  return TRUE;
}

void put_long_p(ULONG *p, ULONG value) {
  put_long((ULONG) p, value);
}

ULONG get_long_p(ULONG *p) {
  return (ULONG) get_long((uaecptr) p);
}

void unlock_jgui(void);

/*********************************
 * setup janusd
 *********************************/
static uae_u32 jd_setup(TrapContext *context, ULONG *param) {
    ULONG want_to_die;
    /* want_to_die:
     *   0: ignore the value, do nothing, just fetch the prefs setting
     *   1: daemon wants to quit
     *   2: demon wants to run again
     */

    JWLOG("jd_setup(.., task %lx, .., stop %d)\n",get_long_p(param),get_long_p(param+8));
#if 0
    JWLOG("::::::::::::::AD_SETUP::::::::::::::::::::::::\n");
    JWLOG("AD__MAXMEM: %d\n", m68k_dreg(&context->regs, 1));
    JWLOG("param:      %lx\n",m68k_areg(&context->regs, 0));
    JWLOG("Task:       %lx\n",get_long_p(param  ));
    JWLOG("Signal:     %lx\n",get_long_p(param+4));
    JWLOG("Stop:       %d\n", get_long_p(param+8));
#endif

    if(!init_done) {
      JWLOG("AD_SETUP called first time => InitSemaphore etc\n");

      /* from now on (aos3_task && aos3_task_signal) the
       * aos3 aros-daemon is ready to take orders!
       */

      aos3_task=get_long_p(param);
      aos3_task_signal=get_long_p(param+4);

      InitSemaphore(&sem_janus_window_list);
      InitSemaphore(&sem_janus_screen_list);
      InitSemaphore(&aos3_thread_start);
      InitSemaphore(&janus_messages_access);
      InitSemaphore(&sem_janus_active_win);
      init_done=TRUE;

      unlock_jgui();

    }

    want_to_die=get_long_p(param+8);

    if(want_to_die == 1) {
      JWLOG("jdaemon tells us, he wants to die (received a SIG-C)\n");
      changed_prefs.jcoherence=FALSE;
      close_all_janus_windows();

      /* update gui !! */
    }

    if(want_to_die == 2) {
      JWLOG("jdaemon tells us, he wants to live again (received a SIG-D)\n");
      changed_prefs.jcoherence=TRUE;
      /* update gui !! */
    }

    JWLOG("return %d\n", changed_prefs.jcoherence);
    put_long_p(param+8, changed_prefs.jcoherence);
    return changed_prefs.jcoherence;
}

/*********************************
 * setup clipd
 *********************************/
static uae_u32 cd_setup(TrapContext *context, ULONG *param) {
    ULONG want_to_die;
    /* want_to_die:
     *   0: ignore the value, do nothing, just fetch the prefs setting
     *   1: daemon wants to quit
     *   2: demon wants to run again
     */

    JWLOG("cd_setup(.., task %lx, .., stop %d)\n",get_long_p(param),get_long_p(param+12));

    if(!aos3_clip_task) {
      JWLOG("AD_CLIP_SETUP called first time => Init..\n");

      /* from now on (aos3_clip_task && aos3_clip_signal) the
       * aos3 clipd is ready to take orders!
       */

      aos3_clip_task=get_long_p(param);
      aos3_clip_signal=get_long_p(param+4);
      aos3_clip_to_amigaos_signal=get_long_p(param+8);

      clipboard_hook_install();

      unlock_jgui();
    }

    want_to_die=get_long_p(param+12);
    JWLOG("want_to_die:%d\n",want_to_die);

    if(want_to_die == 1) {
      JWLOG("clipd tells us, he wants to die (received a SIG-C)\n");
      changed_prefs.jclipboard=FALSE;

      /* update gui !! */
    }

    if(want_to_die == 2) {
      JWLOG("clipd tells us, he wants to live again (received a SIG-D)\n");
      changed_prefs.jclipboard=TRUE;
      /* update gui !! */
    }

    JWLOG("return %d\n", changed_prefs.jclipboard);
    put_long_p(param+12, changed_prefs.jclipboard);
    return changed_prefs.jclipboard;
}

/**********************************************************
 * this stuff gets called from janusd/clipd 
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

    case AD_SETUP: {
      /* the janusd gets ready to serve */
      ULONG *param= (ULONG *) m68k_areg(&context->regs, 0);

      return jd_setup(context, param);
    };

    case AD_CLIP_SETUP: {
      /* the clipd gets ready to serve */
      ULONG *param= (ULONG *) m68k_areg(&context->regs, 0);

      return cd_setup(context, param);
    };

    case AD_SHUTDOWN: {

      /* there is *no* way, to stop a daemon, as they
       * patch the aos3 system and those patches are *not safe*
       * to remove
       */
      printf("ERROR: DO NOT USE AD_SHUTDOWN!!\n");
      JWLOG ("ERROR: DO NOT USE AD_SHUTDOWN!!\n");

/*
      aos3_task=0;
      aos3_task_signal=0;
*/

      return FALSE;
      break;
    };

    /* the aros_daemon wants its orders*/
    case AD_GET_JOB: {
      ULONG job=m68k_dreg(&context->regs, 1);
      ULONG *m68k_results= (ULONG *) m68k_areg(&context->regs, 0);

      //JWLOG("::::::::::::::AD_GET_JOB::::::::::::::::::::::::\n");

      switch(job) {
    	case AD_GET_JOB_LIST_WINDOWS: 
       	  return ad_job_update_janus_windows(m68k_results);
    	case AD_TEST: 
       	  return ad_test(m68k_results);
    	case AD_GET_JOB_DEBUG: 
       	  return ad_debug(m68k_results);
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
    	case AD_GET_JOB_LIST_SCREENS: 
       	  return ad_job_list_screens(m68k_results);
    	case AD_GET_JOB_MESSAGES: 
       	  return ad_job_fetch_message(m68k_results);

	default:
	  JWLOG("ERROR!! aroshack_helper: unkown job: %d\n",m68k_dreg(&context->regs, 1));
	  return FALSE;
      }
      case AD_CLIP_JOB: {
	ULONG job=m68k_dreg(&context->regs, 1);
	ULONG *m68k_results= (ULONG *) m68k_areg(&context->regs, 0);
	switch(job) {
	  case JD_AMIGA_CHANGED:
	    /* remember clipboard change in amigaOS */
	    clipboard_amiga_changed=get_long_p(m68k_results);
	    JWLOG("JD_AMIGA_CHANGED: clipboard_amiga_changed %d\n", clipboard_amiga_changed);
	    put_long((ULONG) (m68k_results), (ULONG) TRUE);
	    return TRUE;
	  case JD_CLIP_COPY_TO_AROS:
	    /* clipd told us to take the amigaOS clipboard and copy it to AROS clipboard */
	    copy_clipboard_to_aros_real(get_long(m68k_results + 4), get_long(m68k_results + 8));
	    JWLOG("copy_clipboard_to_aros_real returned\n");
	    return TRUE;
	  case JD_CLIP_GET_AROS_LEN:
	    /* get clipboard len */
	    put_long((ULONG) (m68k_results), aros_clipboard_len());
	    return TRUE;
	  case JD_CLIP_COPY_FROM_AROS:
	    copy_clipboard_to_amigaos_real(get_long_p(m68k_results), get_long_p(m68k_results+4));
	    return TRUE;
  	  default:
  	    JWLOG("ERROR!! CLIP_JOB: unkown job: %d\n",m68k_dreg(&context->regs, 1));
  	    return FALSE;
	}
      }

   }

    default:
      JWLOG("ERROR!! aroshack_helper: unkown service %d\n",service);
      return FALSE;
  }

}
