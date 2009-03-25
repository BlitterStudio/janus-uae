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
 ************************************************************************/

#include <proto/exec.h>
#include "janus-daemon.h"

extern struct IntuitionBase* IntuitionBase;

/****************************************************
 * patch_functions
 *
 * Although I tried to avoid patching the OS, I
 * have to patch the folowing:
 *
 * - CloseWindow
 *   If a window is closed, the clonewindow
 *   function has no chance, to detect that.
 *   So we need to detect a CloseWindow, *before*
 *   it is closed.
 *
 * - OpenWindow
 *   It feels too slow, to wait for the window 
 *   sync cycle, as intuition
 *   also needs some time to open the window.
 *   Also it is necessary, to update the public
 *   screen list, in case the window should be
 *   opened on a public screen, which not yet
 *   exists as an AROS public screen.
 */

APTR           old_CloseWindow;
APTR           old_OpenWindow;
APTR           old_OpenWindowTagList;

#define PUSHFULLSTACK "movem.l d0-d7/a0-a6,-(SP)\n"
#define POPFULLSTACK  "movem.l (SP)+,d0-d7/a0-a6\n"
#define PUSHSTACK     "movem.l d2-d7/a2-a6,-(SP)\n"
#define POPSTACK      "movem.l (SP)+,d2-d7/a2-a6\n"
#define PUSHA3        "move.l a3,-(SP)\n"
#define POPA3         "move.l (SP)+,a3\n"
#define PUSHD3        "move.l d3,-(SP)\n"
#define POPD3         "move.l (SP)+,d3\n"

/*
 * _my_CloseWindow_SetFunc
 *
 * done in assembler, as C-Source always at least destroys the
 * a5 register for the local variable stack frame. But library
 * functions may only trash d0, d1, a0 and a1.
 *
 * for calltrap:
 * AD_GET_JOB  11 (d0)
 * AD_GET_JOB_MARK_WINDOW_DEAD 7 (d1)
 * window is already in a0
 */
__asm__("_my_CloseWindow_SetFunc:\n"
	PUSHSTACK
	"moveq #11,d0\n"
	"moveq #7,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPSTACK
	PUSHA3
	"move.l _old_CloseWindow, a3\n"
	"jsr (a3)\n"
	POPA3
        "rts\n");

#if 0
void clear_size_gadget(REG(a0, struct NewWindow *newwin)) {
  printf("clear_size_gadget(%lx)\n",newwin);
}
#endif
/*
 * _my_OpenWindow_SetFunc
 *
 * done in assembler, as C-Source always at least destroys the
 * a5 register for the local variable stack frame. But library
 * functions may only trash d0, d1, a0 and a1.
 *
 * for calltrap:
 * AD_GET_JOB  11 (d0)
 * AD_GET_JOB_NEW_WINDOW 11 (d1)
 * new window in a0
 */
__asm__("_my_OpenWindow_SetFunc:\n"
/*
	"movem.l d4/a4,-(SP)\n"
	"lea 24(a0),a4\n"
	"move.l (a4),d4\n"
	"clr.l (a4)\n"
	*/
	PUSHFULLSTACK
	"jsr _update_screens\n"
	POPFULLSTACK
	PUSHA3
	"move.l _old_OpenWindow,a3\n"
	"jsr (a3)\n"
	POPA3
	/*
	"move.l d4,(a4)\n"
	"movem.l (SP)+,d4/a4\n"
	*/
	PUSHFULLSTACK
	"move.l d0,a0\n"
	"moveq #11,d0\n"
	"moveq #11,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPFULLSTACK
        "rts\n");

#if 0
void my_OpenWindow(REG(a0, struct NewWindow *newwin),
                          REG(a6, APTR ibase)) {

  struct Window *win;
  register unsigned long _res __asm("d0");
  struct Window * (*real_OpenWindow)(REG(a0, struct NewWindow *),
                                     REG(a6, APTR ibase))=old_OpenWindow;

  printf("Open window (%lx)..\n",newwin);
  win=real_OpenWindow(newwin, ibase);
  printf("Open window (%lx): %lx\n",newwin,win);

  _res=(unsigned long) win;
}
#endif


__asm__("_my_OpenWindowTagList_SetFunc:\n"
	PUSHFULLSTACK
	"jsr _update_screens\n"
	POPFULLSTACK
	PUSHA3
	"move.l _old_OpenWindowTagList, a3\n"
	"jsr (a3)\n"
	POPA3
	PUSHFULLSTACK
	"move.l d0,a0\n"
	"moveq #11,d0\n"
	"moveq #11,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPFULLSTACK
        "rts\n");

#if 0
static void my_OpenWindowTagList(REG(a0, struct NewWindow *newwin),
                                 REG(a1, ULONG *tags),
                           REG(a2, ULONG a2),
                           REG(a3, ULONG a3),
                           REG(a4, ULONG a4),
                           REG(a5, ULONG a5),
                           REG(d2, ULONG d2),
                           REG(d3, ULONG d3),
                           REG(d4, ULONG d4),
                           REG(d5, ULONG d5),
                           REG(d6, ULONG d6),
                           REG(d7, ULONG d7),
                                 REG(a6, APTR ibase)) {

  struct Window *win;
  struct Window * (*real_OpenWindowTagList)(REG(a0, struct NewWindow *),
                                     REG(a1, ULONG *tags),
                                     REG(a6, APTR ibase))=old_OpenWindowTagList;
  register unsigned long _res __asm("d0");

  printf("OpenWindowTagList (%lx)..\n",(ULONG) newwin);
  win=real_OpenWindowTagList(newwin, tags, ibase);
  printf("OpenWindowTagList (%lx): %lx\n",(ULONG) newwin,(ULONG) win);

  _res=(unsigned long) win;
}
#endif

/*
 * assembler functions need to be delcared or used, before
 * you can reference them (?).
 */
void my_CloseWindow_SetFunc();
void my_OpenWindow_SetFunc();
void my_OpenWindowTagList_SetFunc();

/* According to Ralph Babel: ".. as
 * of 2.0, SetFunction() calls Forbid()/Permit() 
 * automatically  and also takes care of the code 
 * cache itself."
 */
void patch_functions() {

  old_CloseWindow=SetFunction((struct Library *)IntuitionBase, 
                              -72, 
			      (APTR) my_CloseWindow_SetFunc);

  old_OpenWindow=SetFunction((struct Library *)IntuitionBase, 
                              -204, 
			      (APTR) my_OpenWindow_SetFunc);

  old_OpenWindowTagList=SetFunction((struct Library *)IntuitionBase, 
                              -606, 
			      (APTR) my_OpenWindowTagList_SetFunc);
}

/* TODO: unsafe! */
void unpatch_functions() {
  SetFunction((struct Library *)IntuitionBase,
                              -72,
			      old_CloseWindow);
#if 0
  Forbid();
      if (patchfunc != (temp = SetFunction(library,offset,originalfunc)))
	        // can't remove, put patch back and refuse to quit
      SetFunction(library,offset,temp));
		//             else done = TRUE;
		//                 Permit(); 
#endif
}

