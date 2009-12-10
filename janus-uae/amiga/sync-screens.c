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
#include <proto/graphics.h>

#include "janus-daemon.h"

extern struct IntuitionBase* IntuitionBase;

/* ATTENTION: janus-daemon.c does not check for errors, so if you 
 * generate an error here, check it there!
 */
BOOL init_sync_screens() {
#if 0
  old_MoveWindowInFront_Window=NULL;
  old_MoveWindowInFront_BehindWindow=NULL;
  old_MoveWindowInFront_Counter=0;
#endif
  ENTER
  LEAVE

  return TRUE;
}


void screen_test() {
  ULONG *command_mem;

  ENTER

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);

  calltrap (AD_GET_JOB, AD_TEST, command_mem);

  FreeVec(command_mem);

  LEAVE
}

/****************************************************
 * update_top_screen()
 *
 * ask, which aos3 screen is the top screen in AROS
 * land. Then we bring it to the top, too.
 *
 * AD_GET_JOB_TOP_SCREEN returns NULL, if the first
 * AROS screen is a pure native AROS screen.
 ****************************************************/
void update_top_screen() {
  struct Screen *screen;
  ULONG foo[2];

  ENTER

  screen=(struct Screen *) calltrap (AD_GET_JOB, AD_GET_JOB_TOP_SCREEN, foo);

  if(screen && (screen != IntuitionBase->FirstScreen)) {
    if(screen->Title) {
      DebOut("update_top_screen(): bring screen %lx to front (%s)\n",screen,screen->Title);
    }
    else {
      DebOut("update_top_screen(): bring screen %lx to front\n", screen);
    }
    ScreenToFront(screen);
  }

  LEAVE
}

static void screen_fix_resolution(ULONG modeID, WORD *x, WORD *y) {

  DebOut("screen_fix_resolution(%d, %d)\n", *x, *y);
  
  if(modeID == INVALID_ID) {
    DebOut("screen_fix_resolution WARNING: modeID == INVALID_ID\n");
    return;
  }

  if(modeID & SUPERHIRES) {
    *x=*x / 2;
    DebOut("==>SUPER_KEY\n");
  }
  else if(modeID & HIRES) {
    DebOut("==>HIRES_KEY\n");
  }
  else { 
    DebOut("==>LORES_KEY\n");
    *x=*x * 2;
  }
  /* lores */
  if(! (modeID & LACE) ) {
    DebOut("==>no INTERLACE\n");
    *y=*y * 2;
  }

  DebOut("screen_fix_resolution: x,y: %d, %d\n",*x,*y);
}


/***********************************************************************
 * update the screen list, so that new aros screens
 * and are created for new amigaOS windows
 *
 * we send the following via AD_GET_JOB_LIST_SCREENS:
 *  screen pointer, depth, screen public name, max_overscan_width/height
 *  screen pointer, depth, screen public name, max_overscan_width/height
 *  ...
 *
 * if there is no public name, NULL is sent.
 * if it is a custom screen, max_overscan_width/height must be set.
 ***********************************************************************/
void update_screens() {
  ULONG *command_mem;
  ULONG  i;
  UWORD  maxwidth, maxheight;
  struct Screen   *screen;
  struct Rectangle rect;
  UWORD flags;
  ULONG display_id;

  ENTER

  DebOut("update_screens()\n");

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);

  /* the FirstScreen variable points to the frontmost Screen.  Screens are
   * then maintained in a front to back order using Screen.NextScreen
   */

  screen=IntuitionBase->FirstScreen;

  if(!screen) {
    printf("ERROR: no screen!?\n"); /* TODO */
    DebOut("ERROR: no screen!?\n"); /* TODO */
    LEAVE
    return;
  }


  i=0;
  while(screen) {
    DebOut("screen #%d: %lx (%s)\n",i,screen,screen->Title);

    flags=screen->Flags & 0x000F;
    if(flags == CUSTOMSCREEN) {
      display_id=GetVPModeID(&screen->ViewPort);
      if(!(display_id==INVALID_ID) && !QueryOverscan(display_id, &rect, OSCAN_VIDEO)) {
	maxwidth =720;
	maxheight=568;
	DebOut("ERROR. QueryOverscan for ID %lx failed, assuming default values 720x568\n",
		display_id);
      }
      else {
	maxwidth =rect.MaxX-rect.MinX;
	maxheight=rect.MaxY-rect.MinY;
	screen_fix_resolution(display_id, &maxwidth, &maxheight);
      }

      DebOut("  -> oscan_video: %d x %d\n", maxwidth, maxheight);
    }
    else {
      DebOut("  -> no custom screen\n");
    }


    command_mem[i++]=(ULONG) screen;
    command_mem[i++]=(ULONG) screen->RastPort.BitMap->Depth;
    command_mem[i++]=(ULONG) public_screen_name(screen);
    command_mem[i++]=(ULONG) maxwidth;
    command_mem[i++]=(ULONG) maxheight;
    screen=screen->NextScreen;
  }

  calltrap (AD_GET_JOB, AD_GET_JOB_LIST_SCREENS, command_mem);

  FreeVec(command_mem);

  DebOut("update_screens() done\n");
  LEAVE
}

