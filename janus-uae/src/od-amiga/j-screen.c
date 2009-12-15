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

extern UBYTE            *Line;
extern struct RastPort  *TempRPort;
extern struct BitMap    *BitMap;

/************************************************************************
 * aos3screen_is_custom
 ************************************************************************/
BOOL aos3screen_is_custom(uaecptr aos3screen) {
  UWORD          flags;

  JWLOG("screen: %lx\n",aos3screen);

  flags=get_word(aos3screen+20);
  flags=flags & 0xF;
  if(flags!=0xF) {
    JWLOG("  screen %lx is no custom screen\n", aos3screen);
    return FALSE;
  }
  JWLOG("  screen %lx is a custom screen\n", aos3screen);
  return TRUE;
}

/************************************************************************
 * new_aros_cust_screen for an aos3screen
 *
 * open new aros screen for aos3 screen
 ************************************************************************/
static void new_aros_cust_screen(JanusScreen *jscreen, uaecptr aos3screen) {

  struct Screen *arosscr;

  JWLOG("aos3screen: %lx\n", aos3screen);

  jscreen->arosscreen= (struct Screen *) -1;

  aros_custom_screen_start_thread(jscreen); /* fills jscreen->arosscreen */

  return;
}


/************************************************************************
 * new_aros_pub_screen for an aos3screen
 *
 * check if there is already a pubscreen with this name
 * - if yes, return that one
 * - if not, open new one
 ************************************************************************/
static struct Screen *new_aros_pub_screen(JanusScreen *jscreen, 
                                          uaecptr aos3screen) {
  struct Screen *pub;

  JWLOG("screen public name: >%s<\n",jscreen->pubname);

  /* search for already open screen by name */
  pub=(struct Screen *) LockPubScreen(jscreen->pubname);
  if(pub) {
    JWLOG("screen %s already exists\n",jscreen->pubname);
    UnlockPubScreen(NULL, pub);
    return pub;
  }

  jscreen->arosscreen= (struct Screen *) -1;

  aros_screen_start_thread(jscreen);

  /* wait until thread was started and it had a chance to open the screen.
   * If the tread fails, it sets the screen to NULL
   */
  while((LONG) jscreen->arosscreen ==  -1) {
    Delay(10);
  }

  return jscreen->arosscreen;

}

/************************************************************************
 * new_aros_screen(jscreen)
 *
 * fill in jscreen->arosscreen with an already existing poblic screen
 * or a newly opened public/custom screen.
 *
 * It uses new_aros_pub_screen / new_aros_cust_screen, which have to
 * call the thread creating modules then.
 *
 * If it can't create a new screen, it has to ensure, that jscreen
 * gets removed/freed from the list again!
 ************************************************************************/
static void new_aros_screen(JanusScreen *jscreen) {
  struct Screen *arosscr=NULL;
  UWORD flags;
  UWORD screentype;
  uaecptr aos3screen;
  ULONG lock;
  ULONG err;
  ULONG i;

  const UBYTE preferred_depth[] = {24, 32, 16, 15, 8};

  JWLOG("new_aros_screen(%lx)\n",jscreen);

  if(!jscreen || !jscreen->aos3screen) {
    JWLOG("ERROR: new_aros_screen problem!!\n");
    goto EXIT;
  }

  aos3screen=(uaecptr) jscreen->aos3screen;

  JWLOG("jscreen->aos3screen: %lx\n",aos3screen);
  JWLOG("screen title: >%s<\n",get_real_address(get_long(aos3screen+26)));

  /* if we have an amigaos3 public screen, we will create an aros
   * public screen, too
   */

  flags=get_word(aos3screen + 20); /* flags */
  JWLOG("flags: %lx\n",flags);
  screentype=flags & SCREENTYPE;
  JWLOG("screentype: %lx\n",screentype);

  /********************* Custom Screen **************************/
  if(screentype==CUSTOMSCREEN) { /* 0xf */

    jscreen->ownscreen=TRUE;
    JWLOG("screen %lx is CUSTOMSCREEN\n",aos3screen);
    new_aros_cust_screen(jscreen, aos3screen);
    return;

  }
  /******************* Workbench Screen *************************/
  else if(screentype==WBENCHSCREEN) { /* 0x1 */

    JWLOG("screen %lx is WBENCHSCREEN\n",aos3screen);
    /* we just return our wanderer screen here.
     * TODO: What happens, if wanderer is *not* running?
     */
    lock=LockIBase(0);
    arosscr=IntuitionBase->FirstScreen;
    while(arosscr) {
      if(arosscr->Flags & WBENCHSCREEN) {
	UnlockIBase(lock);
	jscreen->ownscreen=FALSE;
	JWLOG("return aros WBENCHSCREEN %lx\n",arosscr);
	jscreen->arosscreen=arosscr;
	return;
      }
      arosscr=arosscr->NextScreen;
    }
    UnlockIBase(lock);
    JWLOG("ERROR: wanderer is not running!?\n");
    goto EXIT;

  }
  /********************* Public Screen **************************/
  else if(screentype==PUBLICSCREEN) {

    JWLOG("screen %lx is PUBLICSCREEN\n",aos3screen);
    //arosscr=new_aros_pub_screen(jscreen, aos3screen);
    new_aros_pub_screen(jscreen, aos3screen);
    return;

  }

  JWLOG("ERROR: screen type unknown !?\n");

EXIT:
  JWLOG("ERROR ERROR: cleanup here / delete jscreen!!");
  return;
}

/**********************************************************
 * ad_job_list_screens
 *
 * we get a list of all aos3 screens here:
 * - link wanderer <-> wb screen
 * - open a new screen for every other aos3 screen
 **********************************************************/
uae_u32 ad_job_list_screens(ULONG *m68k_results) {
  uae_u32 win;
  ULONG   i;
  ULONG   aos3screen=0;
  GSList *list_screen;
  JanusScreen *jscreen;
  char   *public_screen_name;

  JWLOG("ad_job_list_screens()\n");

  ObtainSemaphore(&sem_janus_screen_list);

  i=0;
  while((aos3screen=get_long((uaecptr) m68k_results+i))) {
    JWLOG(" aos3screen: %lx\n",aos3screen);

    list_screen= g_slist_find_custom(janus_screens,
	  (gconstpointer) aos3screen,
	  &aos3_screen_compare);

    if(!list_screen) {
      JWLOG(" screen %lx not found in janus_screens list\n",aos3screen);

      if(get_long((uaecptr) m68k_results+i+4)) {
	public_screen_name=(char *) get_real_address(get_long((uaecptr) m68k_results+i+8));
	JWLOG("pubname: >%s<\n",public_screen_name);
      }
      else {
	public_screen_name=NULL;
      }

      /* TODO: Check here, if a screen with that name already exists
       * and use that screen instead of creating a new one? 
       */

      jscreen=(JanusScreen *) AllocVec(sizeof(JanusScreen),MEMF_CLEAR);
      JWLOG("append aos3screen %lx as %lx\n", aos3screen, jscreen);
      /* jscreen has to be FreeVec'ed by the screen task */

      jscreen->aos3screen=(gpointer) aos3screen;
      jscreen->depth     =get_long((uaecptr) m68k_results+i+4);
      jscreen->pubname   =public_screen_name;                   /* ?? good idea ?? */
      jscreen->maxwidth  =(UWORD)  get_long((uaecptr) m68k_results+i+12);
      jscreen->maxheight =(UWORD)  get_long((uaecptr) m68k_results+i+16);

      JWLOG("maxwidth/maxheight: %d, %d\n", jscreen->maxwidth, jscreen->maxheight);

      /* append it, if new_aros_screen fails, it has to cleanup itself !*/
      janus_screens=g_slist_append(janus_screens,jscreen);
      new_aros_screen(jscreen);
    }
    i=i+20; /* we get *5* ULONGs: screen pointer, depth and pubname */
  }
  ReleaseSemaphore(&sem_janus_screen_list);

  JWLOG("ad_job_list_screens() done\n");

  return TRUE;
}

/**********************************************************
 * ad_job_open_custom_screen
 *
 * we get a screen structure in a0 here
 *
 * In *this* case, the argument is already a screen
 * pointer, no pointer to parameter data.
 *
 * It seems, we need an own thread here, too.
 **********************************************************/
uae_u32 ad_job_open_custom_screen(ULONG aos3screen) {

  JWLOG("new aros screen: %lx (but we let sync screen handle it.\n", aos3screen);

  return TRUE;

#if 0
  GSList *list_screen;
  JanusScreen *jscreen;
  UWORD width;
  UWORD depth;
  UWORD height;
  ULONG mode;
  ULONG newwidth;
  ULONG newheight;
  const UBYTE preferred_depth[] = {8, 15, 16, 24, 32, 0};
  ULONG i;
  ULONG error;
  static struct NewWindow NewWindowStructure = {
	0, 0, 800, 600, 0, 1,
	IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY /*| IDCMP_DISKINSERTED | IDCMP_DISKREMOVED*/
		| IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW | IDCMP_MOUSEMOVE | IDCMP_DELTAMOVE |
		IDCMP_REFRESHWINDOW,
	WFLG_SMART_REFRESH | WFLG_BACKDROP | WFLG_RMBTRAP | WFLG_NOCAREREFRESH
	 | WFLG_BORDERLESS | WFLG_ACTIVATE | WFLG_REPORTMOUSE,
	NULL, NULL, NULL, NULL, NULL, 5, 5, 800, 600,
	CUSTOMSCREEN
  };

  JWLOG("entered\n");

/* ?? */  currprefs.gfx_width_win= graphics_init();

  JWLOG("aos3screen: %lx\n", aos3screen);

  if(!aos3screen_is_custom(aos3screen)) {
    JWLOG("  no custom screen, do nothing\n");
    return TRUE;
  }

  /* check, if we already have that screen, as OpenScreen might call OpenScreenTags ..!! */
  ObtainSemaphore(&sem_janus_screen_list);
  list_screen= g_slist_find_custom(janus_screens,
	  (gconstpointer) aos3screen,
	  &aos3_screen_compare);

  if(list_screen) {
    JWLOG(" screen %lx already in janus_screens list\n",aos3screen);
    ReleaseSemaphore(&sem_janus_screen_list);
    return TRUE;
  }


  jscreen=(JanusScreen *) AllocVec(sizeof(JanusScreen),MEMF_CLEAR);
  JWLOG("append aos3screen %lx as %lx\n", aos3screen, jscreen);
  jscreen->aos3screen=aos3screen;


  width =get_word(aos3screen+12);
  height=get_word(aos3screen+14);
  JWLOG("w: %d h: %d\n", width, height);

#if 0
  currprefs.gfx_width_win=width;
  currprefs.gfx_height = height;
  currprefs.amiga_screen_type=0 /*UAESCREENTYPE_CUSTOM*/;

  uae_main_window_closed=FALSE;
  j_stop_window_update=TRUE;
  gfx_set_picasso_state(0);
  graphics_init();
  gfx_set_picasso_state(0);
  jscreen->arosscreen=S;

  JWLOG("new aros custom screen: %lx (%d x %d)\n",jscreen->arosscreen, jscreen->arosscreen->Width, 
                                                                       jscreen->arosscreen->Height);
  janus_screens=g_slist_append(janus_screens,jscreen);


  ReleaseSemaphore(&sem_janus_screen_list);
#endif

#warning remove me <======================================================================
  width=720;
  height=568;
  JWLOG("w: %d h: %d\n", width, height);
#warning remove me <======================================================================
 
  currprefs.gfx_width_win=width;
  currprefs.gfx_height = height;
  currprefs.amiga_screen_type=0 /*UAESCREENTYPE_CUSTOM*/;

  /* this would be the right approach, but getting this pointer stuff
   * working in 68k adress space is a pain. 
   * screen->vp->RasInfo->BitMap->Depth ... */

  /* open a screen, which matches best */
  i=0;
  mode=INVALID_ID;
  newwidth =width;
  newheight=height;
  while(mode==(ULONG) INVALID_ID && preferred_depth[i]) {
    mode=find_rtg_mode(&newwidth, &newheight, preferred_depth[i]);
    i++;
  }

  if(mode == (ULONG) INVALID_ID) {
    FreeVec(jscreen);
    JWLOG("ERROR: could not find modeid !?!!\n");
    ReleaseSemaphore(&sem_janus_screen_list);
    return FALSE;
  }

  JWLOG("found modeid: %lx\n", mode);
  JWLOG("found w: %d h: %d\n", newwidth, newheight);


  /* If the screen is larger than requested, center UAE's display */
  XOffset=0;
  YOffset=0;
  if (newwidth > (ULONG)width) {
    XOffset = (newwidth - width) / 2;
  }
  if (newheight > (ULONG) height) {
    YOffset = (newheight - height) / 2;
  }
  JWLOG("XOffset, YOffset: %d, %d\n", XOffset, YOffset);

  jscreen->arosscreen=OpenScreenTags(NULL,
                                     //SA_Type,      PUBLICSCREEN,
				     //SA_PubName,   "FOO",
                                     SA_Width,     newwidth,
				     SA_Height,    newheight,
				     SA_Depth,     depth,
				     SA_DisplayID, mode,
				     SA_Behind,    FALSE,
				     SA_ShowTitle, FALSE,
				     SA_Quiet,     TRUE,
				     SA_ErrorCode, (ULONG)&error,
				     TAG_DONE);

  if(!jscreen->arosscreen) {
    JWLOG("ERROR: could not open screen !? (Error: %ld\n",error);
    gui_message ("Cannot open custom screen (Error: %ld)\n", error);
    FreeVec(jscreen);
    ReleaseSemaphore(&sem_janus_screen_list);
    return FALSE;
  }

  JWLOG("new aros custom screen: %lx (%d x %d)\n",jscreen->arosscreen, jscreen->arosscreen->Width, 
                                                                       jscreen->arosscreen->Height);
#warning TODO Semaphore !?
  janus_screens=g_slist_append(janus_screens,jscreen);

  gfxvidinfo.height     =height;
  gfxvidinfo.width      =width;
  //currprefs.gfx_linedbl =0;
  //currprefs.gfx_lores   =1;
 
  S  = jscreen->arosscreen;
  CM = jscreen->arosscreen->ViewPort.ColorMap;
  RP = &jscreen->arosscreen->RastPort;

//  NewWindowStructure.Width  = jscreen->arosscreen->Width;
//  NewWindowStructure.Height = jscreen->arosscreen->Height;
  NewWindowStructure.Width  = newwidth;
  NewWindowStructure.Height = newheight;
  NewWindowStructure.Screen = jscreen->arosscreen;

  W = (void*)OpenWindow (&NewWindowStructure);
  if (!W) {
    gui_message ("Cannot open window on new custom screen !?");
    CloseScreen(jscreen->arosscreen);
    FreeVec(jscreen);
    ReleaseSemaphore(&sem_janus_screen_list);
    return FALSE;
  }

  ReleaseSemaphore(&sem_janus_screen_list);

  JWLOG("new aros window on custom screen: %lx (%d x %d)\n", W, W->Width, W->Height);

  /* TODO !!*/
  uae_main_window_closed=FALSE;

  reset_drawing(); /* this will bring a custon screen, with keyboard working */

#if 0
  j_stop_window_update=TRUE;
  custom_screen_active=jscreen;

  gfx_set_picasso_state(0);
  reset_drawing();
  init_row_map();
  init_aspect_maps();
  notice_screen_contents_lost();
  inputdevice_acquire ();
  inputdevice_release_all_keys ();
  reset_hotkeys ();
  hide_pointer (W);
#endif

  JWLOG("W: %lx (%s)\n", W, W->Title);
  aros_custom_screen_start_thread(jscreen);

  JWLOG("Line: %lx\n", Line);
  JWLOG("BitMap: %lx\n", BitMap);
  JWLOG("TempRPort: %lx\n", TempRPort);

  if(!Line || !BitMap || !TempRPort) {
    JWLOG("ERROR ERROR ERROR ERROR: NULL pointer in Line/BitMap/TempRPort!!\n");
    return FALSE; /* ? */
  }

  return TRUE;
#endif
}

uae_u32 ad_job_close_screen(ULONG aos3screen) {
  GSList        *list_screen;
  JanusScreen   *jscreen;
  struct Screen *arosscreen;

  JWLOG("screen: %lx\n",aos3screen);

  if(!aos3screen_is_custom(aos3screen)) {
    JWLOG("  no custom screen, do nothing\n");
  }

  ObtainSemaphore(&sem_janus_screen_list);
  list_screen= g_slist_find_custom(janus_screens,
	  (gconstpointer) aos3screen,
	  &aos3_screen_compare);

  if(!list_screen) {
    JWLOG(" screen %lx not found in janus_screens list\n", aos3screen);
    ReleaseSemaphore(&sem_janus_screen_list);
    return TRUE;
  }

  jscreen=(JanusScreen *) list_screen->data;

  if(!jscreen->arosscreen) {
    JWLOG(" jscreen %lx has no arosscreen !?\n", jscreen);
    ReleaseSemaphore(&sem_janus_screen_list);
    return FALSE;
  }

  if(!jscreen->task) {
    JWLOG(" ERROR: jscreen %lx has no task !?\n", jscreen);
    ReleaseSemaphore(&sem_janus_screen_list);
    return FALSE;
  }

  JWLOG("send SIGBREAKF_CTRL_C to task %lx of jscreen %lx \n", jscreen->task, jscreen);
  Signal(jscreen->task, SIGBREAKF_CTRL_C);

  ReleaseSemaphore(&sem_janus_screen_list);
  return TRUE;
}

/**********************************************************
 * ad_job_top_screen
 *
 * return aos3 screen, which should be on top or
 * NULL, if current AROS screen has no sibling
 *
 * gets curent aos3 top screen as parameter
 **********************************************************/
uae_u32 ad_job_top_screen(ULONG *m68k_results) {
  uae_u32 screen;

  ENTER

  /* need to remember first screen for j-custom-screen-thread.c/handle_msg */
  aos3_first_screen=get_long_p(m68k_results);

  ObtainSemaphore(&sem_janus_active_custom_screen);

  if(!janus_active_screen) {
    screen=0;
  }
  else {
    screen=(uae_u32) janus_active_screen->aos3screen;
  }

  ReleaseSemaphore(&sem_janus_active_custom_screen);

  LEAVE

  return screen;
}


/***********************************************************
 * close_all_janus_screens()
 *
 * send a CTRL-C to all aros janus tasks, so that they
 * close their screen and free their resources.
 ***********************************************************/
void close_all_janus_screens() {
  GSList      *list_screen;
  JanusScreen *jscreen;

  ENTER

  if(!aos3_task) {
    /* never registered, nothing to close */
    LEAVE
    return;
  }

  JWLOG("close_all_janus_screens\n");

  ObtainSemaphore(&sem_janus_screen_list);
  list_screen=janus_screens;
  while(list_screen) {
    jscreen=(JanusScreen *) list_screen->data;
    JWLOG("  send CLTR_C to task %lx\n",jscreen->task);
    if(jscreen->task) {
      Signal(jscreen->task, SIGBREAKF_CTRL_C);
      list_screen=g_slist_next(list_screen);
    }
    else {
      JWLOG("  -> remove %lx from janus_screens, as it has no own thread\n", list_screen);
      janus_screens=g_slist_delete_link(janus_screens, list_screen);
      FreeVec(jscreen);

      /* continue with new list */
      list_screen=janus_screens;
    }
  }
  ReleaseSemaphore(&sem_janus_screen_list);
  LEAVE
}

