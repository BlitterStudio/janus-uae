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
 * update the screen list, so that new aros screens
 * and are created for new amigaOS windows
 *
 * we send the following via AD_GET_JOB_LIST_SCREENS:
 *  screen pointer, depth, screen public name
 *  screen pointer, depth, screen public name
 *  ...
 *
 * if there is no public name, NULL is sent.
 ****************************************************/
void update_screens() {
  ULONG *command_mem;
  ULONG i;
  struct Screen *screen;

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
    DebOut("add screen #%d: %lx (%s)\n",i,screen,screen->Title);
    command_mem[i++]=(ULONG) screen;
    command_mem[i++]=(ULONG) screen->RastPort.BitMap->Depth;
    command_mem[i++]=(ULONG) public_screen_name(screen);
    screen=screen->NextScreen;
  }

  calltrap (AD_GET_JOB, AD_GET_JOB_LIST_SCREENS, command_mem);

  FreeVec(command_mem);

  DebOut("update_screens() done\n");
  LEAVE
}

