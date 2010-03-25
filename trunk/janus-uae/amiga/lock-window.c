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

/****************************************************
 * BOOL assert_window_on_screen(screen, window)
 *
 * check, if a window still exists on a screen.
 *
 * ATT: you have to call LockIBase yourself!!
 ****************************************************/
static BOOL assert_window_on_screen(struct Screen *screen, struct Window *window) {

  struct Window *i;

  i=screen->FirstWindow;

  while(i) {
    if(i==window) {
      return TRUE;
    }
    i=i->NextWindow;
  }

  return FALSE;
}

/****************************************************
 * BOOL assert_window(window)
 *
 * check, if the window still exists on any screen.
 *
 * ATT: you have to call LockIBase yourself!!
 ****************************************************/
static BOOL assert_window(struct Window *window) {

  struct Screen *i;

  i=IntuitionBase->FirstScreen;

  while(i) {
    if(assert_window_on_screen(i, window)) {
      return TRUE;
    }
    i=i->NextScreen;
  }

  return FALSE;
}

/****************************************************
 * lock_window(window)
 *
 * - tests, if window exists
 * - tries to make sure, that nobody can close
 *   the window from now on
 * - can't work 100%, sorry
 * - please call unlock_window as soon as possible,
 *   as your task will run in a high priority
 *   between lock and unlock!
 * - it is "safe" to lock more than one window, but
 *   you need to unlock them in the opposite order!
 ****************************************************/
struct WindowLock *lock_window(struct Window *window) {

  ULONG lock;
  struct WindowLock *wl;

  lock=LockIBase(0);

  if(!assert_window(window)) {
    UnlockIBase(lock);
    return 0;
  }

  wl=AllocVec(sizeof(struct WindowLock), MEMF_CLEAR);

  wl->pri=SetTaskPri(FindTask(NULL), 30); /* don't let anybody else win .. hopefully */

  UnlockIBase(lock);

  return wl;
}

/****************************************************
 * unlock_window(lock)
 *
 * - undo the lock
 ****************************************************/
void unlock_window(struct WindowLock *wl) {

  SetTaskPri(FindTask(NULL), wl->pri);
  FreeVec(wl);
}


