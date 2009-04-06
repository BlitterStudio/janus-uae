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

/************************************************************************
 * new_aros_pub_screen for an aos3screen
 *
 * check if there is already a pubscreen with this name
 * - if yes, return that one
 * - if not, open new one
 ************************************************************************/
static struct Screen *new_aros_pub_screen(JanusScreen *jscreen, 
                                          uaecptr aos3screen) {
  ULONG width;
  ULONG height;
  ULONG depth;
  LONG mode  = INVALID_ID;
  ULONG i;
  ULONG err;
  struct Screen *arosscr;
  const UBYTE preferred_depth[] = {24, 32, 16, 15, 8};
  struct screen *pub;

  JWLOG("screen public name: >%s<\n",jscreen->pubname);

  /* search for already open screen by name */
  pub=LockPubScreen(jscreen->pubname);
  if(pub) {
    JWLOG("screen %s already exists\n",jscreen->pubname);
    UnlockPubScreen(NULL, pub);
    return pub;
  }

  aros_screen_start_thread(jscreen);

  return jscreen->arosscreen;

#if 0
  /* we need a new screen */
  width =get_word(aos3screen+12);
  height=get_word(aos3screen+14);
  depth =jscreen->depth;

  JWLOG("we want %dx%d in %d bit\n",width,height,depth);

  mode=find_rtg_mode(&width, &height, depth);
  JWLOG("screen mode with depth %d: %lx\n",depth,mode);

  if(mode == INVALID_ID) {
    JWLOG("try different depth:\n");
    i=0;
    while( (i<sizeof(preferred_depth)) && (mode==INVALID_ID)) {
      depth = preferred_depth[i];
      mode = find_rtg_mode (&width, &height, depth);
      i++;
    }
    JWLOG("screen mode with depth %d: %lx\n",depth,mode);
  }

  if(mode==INVALID_ID) {
    JWLOG("ERROR: unable to find a screen mode !?\n");
    return NULL;
  }

  arosscr=OpenScreenTags(NULL,
		     SA_Type, PUBLICSCREEN,
		     SA_DisplayID, mode,
		     SA_Width, width, /* necessary ? */
		     SA_Height, height,
		     SA_Title, get_real_address(get_long(aos3screen+26)),
		     SA_PubName, jscreen->pubname,
#if 0
		     SA_PubTask, jscreen->task,
		     SA_SA_PubSig, jscreen->signal,
#endif
		     SA_ErrorCode, &err,
		     TAG_DONE);

  jscreen->ownscreen=TRUE; /* TODO we have to close this one, how..? */

  return arosscr;
#endif
}

static struct Screen *new_aros_screen(JanusScreen *jscreen) {
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
    return NULL;
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
    JWLOG("ERROR: CUSTOMSCREEN IS NOT YET SUPPORTED !!!!\n");
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
	return arosscr;
      }
      arosscr=arosscr->NextScreen;
    }
    UnlockIBase(lock);
    JWLOG("ERROR: wanderer is not running!?\n");
    return NULL;
  }
  /********************* Public Screen **************************/
  else if(screentype==PUBLICSCREEN) {
    JWLOG("screen %lx is PUBLICSCREEN\n",aos3screen);
    //arosscr=new_aros_pub_screen(jscreen, aos3screen);
    new_aros_pub_screen(jscreen, aos3screen);
    return jscreen->arosscreen;
  }
  else {
    JWLOG("ERROR: screen type unknown !?\n");
  }
  JWLOG("new screen: %lx\n",arosscr);
  return arosscr;
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
	public_screen_name=get_real_address(get_long((uaecptr) m68k_results+i+8));
      }
      else {
	public_screen_name=NULL;
      }
      JWLOG("pubname: >%s<\n",public_screen_name);

      /* TODO: Check here, if a screen with that name already exists
       * and use that screen instead of creating a new one? 
       */

      jscreen=(JanusScreen *) AllocVec(sizeof(JanusScreen),MEMF_CLEAR);
      JWLOG("append aos3screen %lx as %lx\n", aos3screen, jscreen);

      jscreen->aos3screen=aos3screen;
      jscreen->depth=get_long((uaecptr) m68k_results+i+4);
      jscreen->pubname=public_screen_name; /* ?? good idea ?? */
      jscreen->arosscreen=new_aros_screen(jscreen);
      if(jscreen->arosscreen) {
	JWLOG("new aros screen: %lx\n",jscreen->arosscreen);
	janus_screens=g_slist_append(janus_screens,jscreen);
      }
      else {
	JWLOG("ERROR: new_aros_screen failed!\n");
	FreeVec(jscreen);
      }
    }
    i=i+12; /* we get *3* ULONGs: screen pointer, depth and pubname */
  }
  ReleaseSemaphore(&sem_janus_screen_list);

  JWLOG("ad_job_list_screens() done\n");

  return TRUE;
}

