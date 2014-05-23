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
#include <proto/dos.h>

#include "janus-daemon.h"

/*
 * public_screen_name
 * is based on FrontPubScreen 
 * 12 Apr 1992 john fieber (jfieber@sophia.smith.edu)
 */

/* Bad thing here is, that AmigaOS seems to just do a LockIntuition for LockPubScreenList, 
 * which is too much * here.. 
 * but LockIntuition is just one ObtainSemaphore, maybe we can live with that.
 */

char *public_screen_name(struct Screen *scr) {
  struct List          *public_screen_list;
  struct PubScreenNode *public_screen_node;
  ULONG                 i;

  ENTER

  public_screen_list=NULL;
  i=0;

  DebOut("entered\n");

  public_screen_list = (struct List *) LockPubScreenList();
  DebOut("public_screen_name(%lx): LockPubScreenList succeeded\n", scr, i);

  if(!public_screen_list) {
    DebOut("ERROR: unable to LockPubScreenList for screen %lx\n", scr);
    return NULL;
  }

  public_screen_node = (struct PubScreenNode *) public_screen_list->lh_Head;

  if(public_screen_node) {
    while (public_screen_node) {
      if(public_screen_node->psn_Screen == scr) {
        UnlockPubScreenList();
        return public_screen_node->psn_Node.ln_Name;
      }
      public_screen_node=(struct PubScreenNode *)
      public_screen_node->psn_Node.ln_Succ;
    }
  }

  UnlockPubScreenList();
  LEAVE
  return NULL;
}

