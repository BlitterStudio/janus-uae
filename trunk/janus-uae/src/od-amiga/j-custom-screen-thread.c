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

/***********************************************************
 * every AOS3 custom screen gets its own process
 ***********************************************************/  
static void aros_custom_screen_thread (void) {

  struct Process *thread = (struct Process *) FindTask (NULL);
  GSList         *list_screen=NULL;
  JanusScreen    *jscr=NULL;
  BOOL            done;
  ULONG           s;
  struct Window  *aroswin;

  /* There's a time to live .. */

  JWLOG("aros_cscr_thread[%lx]: thread running \n",thread);

  ObtainSemaphore(&sem_janus_screen_list);
  list_screen=g_slist_find_custom(janus_screens, 
	 		     	  (gconstpointer) thread,
			     	  &aos3_screen_process_compare);

  ReleaseSemaphore(&sem_janus_screen_list);
  JWLOG("aros_cscr_thread[%lx]: released sem_janus_window_list sem \n",thread);

  if(!list_screen) {
    JWLOG("aros_cscr_thread[%lx]: ERROR: screen of this thread not found in screen list !?\n",thread);
    goto EXIT; 
  }

  jscr=(JanusScreen *) list_screen->data;
  JWLOG("aros_scr_thread[%lx]: win: %lx \n",thread,jscr);

  JWLOG("aros_cscr_thread[%lx]: jscr->arosscreen: %lx\n", thread, jscr->arosscreen);
  if(!jscr->arosscreen) {
    JWLOG("aros_scr_thread[%lx]: ERROR: screen has to be already opened !!\n",thread);
    goto EXIT; 
  }

  /* our custom screen has only one window */
  aroswin=jscr->arosscreen->FirstWindow;
  if(!aroswin) {
    JWLOG("aros_scr_thread[%lx]: ERROR: screen %lx has no window !!\n",thread, jscr->arosscreen);
  }

  done=FALSE;
  while(!done) {
    handle_events_W(aroswin);
    //done=TRUE;
    JWLOG("aros_scr_thread[%lx]: handle_events_W returned\n", thread);
  }

  /* ... and a time to die. */

EXIT:
  JWLOG("aros_scr_thread[%lx]: EXIT\n");
  done=TRUE;  /* need something, if debug is off.. */
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

int aros_custom_screen_start_thread (JanusScreen *screen) {

    JWLOG("aros_screen_start_thread(%lx)\n",screen);

    screen->name=AllocVec(8+strlen(CUSTOM_SCREEN_TASK_PREFIX_NAME)+1,MEMF_CLEAR);

    sprintf(screen->name,"%s%lx", CUSTOM_SCREEN_TASK_PREFIX_NAME, screen->aos3screen);

    ObtainSemaphore(&aos3_thread_start);
    screen->task = (struct Task *)
	    myCreateNewProcTags ( NP_Output, Output (),
				  NP_Input, Input (),
				  NP_Name, (ULONG) screen->name,
				  NP_CloseOutput, FALSE,
				  NP_CloseInput, FALSE,
				  NP_StackSize, 4096,
				  NP_Priority, 0,
				  NP_Entry, (ULONG) aros_custom_screen_thread,
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

