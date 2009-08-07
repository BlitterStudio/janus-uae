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

#include <proto/exec.h>
#include <proto/utility.h>
#include "janus-daemon.h"

extern struct IntuitionBase* IntuitionBase;

/****************************************************
 * patch_functions
 *
 * Although I tried to avoid patching the OS, I
 * have to patch the following:
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
 *
 * - OpenScreen
 *   If it is a custom screen, we want
 *   to open up an according screen
 *   on AROS. If it is a public
 *   screen, we can care for that,
 *   if a window opens on the
 *   screen.
 */

APTR           old_CloseWindow;
APTR           old_OpenWindow;
APTR           old_OpenWindowTagList;
APTR           old_OpenScreen;
APTR           old_OpenScreenTagList;

#define PUSHFULLSTACK "movem.l d0-d7/a0-a6,-(SP)\n"
#define POPFULLSTACK  "movem.l (SP)+,d0-d7/a0-a6\n"
#define PUSHSTACK     "movem.l d2-d7/a2-a6,-(SP)\n"
#define POPSTACK      "movem.l (SP)+,d2-d7/a2-a6\n"
#define PUSHA3        "move.l a3,-(SP)\n"
#define POPA3         "move.l (SP)+,a3\n"
#define PUSHD3        "move.l d3,-(SP)\n"
#define POPD3         "move.l (SP)+,d3\n"
#define PUSHA0        "move.l a0,-(SP)\n"
#define POPA0         "move.l (SP)+,a0\n"


/*********************************************************************************
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
 *********************************************************************************/
__asm__("_my_CloseWindow_SetFunc:\n"
	"cmp.l #1,_state\n"
	"blt close_patch_disabled\n"
	PUSHSTACK
	"moveq #11,d0\n"
	"moveq #7,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPSTACK
	"close_patch_disabled:\n"
	PUSHA3
	"move.l _old_CloseWindow, a3\n"
	"jsr (a3)\n"
	POPA3
        "rts\n");

/*********************************************************************************
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
 *********************************************************************************/
__asm__("_my_OpenWindow_SetFunc:\n"
/*
	"movem.l d4/a4,-(SP)\n"
	"lea 24(a0),a4\n"
	"move.l (a4),d4\n"
	"clr.l (a4)\n"
	*/
	"cmp.l #1,_state\n"
	"blt openwin_patch_disabled1\n"
	PUSHFULLSTACK
	"jsr _update_screens\n"
	POPFULLSTACK
	"openwin_patch_disabled1:\n"
	PUSHA3
	"move.l _old_OpenWindow,a3\n"
	"jsr (a3)\n"
	POPA3
	/*
	"move.l d4,(a4)\n"
	"movem.l (SP)+,d4/a4\n"
	*/
	"cmp.l #1,_state\n"
	"blt openwin_patch_disabled2\n"
	PUSHFULLSTACK
	"move.l d0,a0\n"
	"moveq #11,d0\n"
	"moveq #11,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPFULLSTACK
	"openwin_patch_disabled2:\n"
        "rts\n");

/*********************************************************************************
 * _my_OpenWindowTagList_SetFunc
 *********************************************************************************/
__asm__("_my_OpenWindowTagList_SetFunc:\n"
	"cmp.l #1,_state\n"
	"blt openwintag_patch_disabled1\n"
	PUSHFULLSTACK
	"jsr _update_screens\n"
	POPFULLSTACK
	"openwintag_patch_disabled1:\n"
	PUSHA3
	"move.l _old_OpenWindowTagList, a3\n"
	"jsr (a3)\n"
	POPA3
	"cmp.l #1,_state\n"
	"blt openwintag_patch_disabled2\n"
	PUSHFULLSTACK
	"move.l d0,a0\n"
	"moveq #11,d0\n"
	"moveq #11,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPFULLSTACK
	"openwintag_patch_disabled2:\n"
        "rts\n");


struct TagItem       mytags_linked[2]={ { SA_Draggable, FALSE}, { TAG_MORE, NULL}};
struct TagItem       mytags[2]={ { SA_Draggable, FALSE}, { TAG_DONE, TAG_DONE}};
struct ExtNewScreen  nodrag_newscreen;
struct TagItem       taglist_mytags_linked[2]={ { SA_Draggable, FALSE}, { TAG_MORE, NULL}};
struct TagItem       taglist_mytags[2]={ { SA_Draggable, FALSE}, { TAG_DONE, TAG_DONE}};


/*********************************************************************************
 * remove_dragging
 *
 * set SA_Draggable to FALSE, AROS has no screen dragging ;)
 *********************************************************************************/
ULONG remove_dragging (struct NewScreen *newscreen __asm("a0")) {
  struct TagItem      *tags;
  struct TagItem      *tstate;
  struct TagItem      *tag;
  ULONG result;

  DebOut("entered remove_dragging\n");
  DebOut("newscreen: %lx\n", newscreen);
  DebOut(" name: %s\n",newscreen->DefaultTitle);
  DebOut(" type: %x\n",newscreen->Type);
  if((newscreen->Type & SCREENTYPE) != CUSTOMSCREEN) {
    DebOut(" no custom screen\n");
    /* return; */
  }

  if(! (newscreen->Type & NS_EXTENDED)) {
    /* allocvec a new ExtNewScreen structure! */
    DebOut(" not extended\n");
    CopyMem( newscreen, &nodrag_newscreen, sizeof(struct NewScreen));
    nodrag_newscreen.Extension=NULL;
    nodrag_newscreen.Type = nodrag_newscreen.Type | NS_EXTENDED;
  }
  else {
    DebOut(" extended\n");
    CopyMem( newscreen, &nodrag_newscreen, sizeof(struct ExtNewScreen));
  }

  /* now we have a NS_EXTENDED nodrag_newscreen */

  DebOut(" new screen with Extension: %lx\n");

  tags=nodrag_newscreen.Extension; 
  DebOut(" Tags: %lx\n", tags);

  tstate=tags;
  while((tag = NextTagItem(&tstate))) {
    DebOut("  TAG    : %08lx = %08lx\n", tag->ti_Tag - 0x80000020, tag->ti_Data); /* - SA_Dummy */
  }

  if(tags) {
    DebOut(" we have tags\n"); /* TODO: care, if Draggable was set to TRUE ? who would do that..? */
    mytags_linked[1].ti_Data=(ULONG) nodrag_newscreen.Extension;
    nodrag_newscreen.Extension=mytags_linked;
  }
  else {
    nodrag_newscreen.Extension=mytags;
  }

  DebOut("after the patch:\n");
  tags=nodrag_newscreen.Extension;
  DebOut(" Tags at: %lx\n", tags);

  tstate=tags;
  while((tag = NextTagItem(&tstate))) {
    DebOut("  TAG    : %08lx = %08lx\n", tag->ti_Tag - 0x80000020, tag->ti_Data); /* - SA_Dummy */
  }

  result=(ULONG) &nodrag_newscreen;

  return result;
}

/* 
 * use taglist_mytags_linked here and not mytags_linked, as OpenScreen might call
 * OpenScreenTagList.. and so we would cause wrong links 
 */
ULONG remove_dragging_TagList (struct TagItem *original_tags __asm("a1")) {
  struct TagItem      *tstate;
  struct TagItem      *tag;

  DebOut("entered remove_dragging_TagList\n");

  tstate=original_tags;
  while((tag = NextTagItem(&tstate))) {
    DebOut("  TAG    : %08lx = %08lx\n", tag->ti_Tag - 0x80000020, tag->ti_Data); /* - SA_Dummy */
  }

  if(!original_tags) {
    return (ULONG) taglist_mytags;
  }

  taglist_mytags_linked[1].ti_Data=(ULONG) original_tags;
  return (ULONG) taglist_mytags_linked;
}


/*********************************************************************************
 * _my_OpenScreen_SetFunc
 *
 * We need to care for new custom screens. Public screens are handled, as soon
 * as the first window is opened on them.
 * AD_GET_JOB_OPEN_CUSTOM_SCREEN has to check, if it is really a custom
 * screen. We could do that here, but there are not many OpenScreens
 * at all, so performance is no issue.
 *
 * We don't watn draggable screens, as we run into trouble with updates of
 * the background, if for example behind the amigaOS screen a native
 * aros screen should be visible. So we create a new NewScreen struct
 * and patch it so that it has SA_Draggable=FALSE. The original
 * sctruct stays intact (including the Taglist).
 *
 * for calltrap:
 * AD_GET_JOB  11                   (d0)
 * AD_GET_JOB_OPEN_CUSTOM_SCREEN 13 (d1)
 * new screen                       (a0)
 *********************************************************************************/
__asm__("_my_OpenScreen_SetFunc:\n"
        PUSHA0
        "movem.l d1-d7/a1-a6,-(SP)\n"
	"jsr _remove_dragging\n"      /* we get back a new struct Screen here in D0*/
        "movem.l (SP)+,d1-d7/a1-a6\n"
	/* call original function */
	PUSHA3
	"move.l d0,a0\n"
	"move.l _old_OpenScreen,a3\n"
	"jsr (a3)\n"
	POPA3
	POPA0                        /* restore original struct Screen */
	/* check, if we are disabled */
	"cmp.l #1,_state\n"
	"blt openscreen_patch_disabled\n"
	PUSHFULLSTACK
	"move.l d0,a0\n"
	"moveq #11,d0\n"
	"moveq #13,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPFULLSTACK
	"openscreen_patch_disabled:\n"
        "rts\n");

/*********************************************************************************
 * _my_OpenScreenTagList_SetFunc
 *
 * We need to care for new custom screens. Public screens are handled, as soon
 * as the first window is opened on them.
 * AD_GET_JOB_OPEN_CUSTOM_SCREEN has to check, if it is really a custom
 * screen. We could do that here, but there are not many OpenScreens
 * at all, so performance is no issue.
 *
 * for calltrap:
 * AD_GET_JOB  11                   (d0)
 * AD_GET_JOB_OPEN_CUSTOM_SCREEN 13 (d1)
 * new screen                       (a0)
 *********************************************************************************/
__asm__("_my_OpenScreenTagList_SetFunc:\n"
        "move.l  a0,-(SP)\n"             /* backup struct NewScreen   */
        "movem.l d0-d7/a2-a6,-(SP)\n"    /* backup everything but A1  */
	"jsr _remove_dragging_TagList\n"
	"move.l d0, a1\n"                /* new TagList               */
        "movem.l (SP)+,d0-d7/a2-a6\n"    /* restore everything but A1 */
        "move.l  (SP)+,a0\n"             /* restore struct NewScreen  */
	/* call original function */
	PUSHA3
	"move.l _old_OpenScreenTagList,a3\n"
	"jsr (a3)\n"
	POPA3
	/* check, if we are disabled */
	"cmp.l #1,_state\n"
	"blt openscreentags_patch_disabled\n"
	PUSHFULLSTACK
	"move.l d0,a0\n"
	"moveq #11,d0\n"
	"moveq #13,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPFULLSTACK
	"openscreentags_patch_disabled:\n"
        "rts\n");

/*
 * assembler functions need to be delcared or used, before
 * you can reference them (?).
 */
void my_CloseWindow_SetFunc();
void my_OpenWindow_SetFunc();
void my_OpenWindowTagList_SetFunc();
void my_OpenScreen_SetFunc();
void my_OpenScreenTagList_SetFunc();

/* According to Ralph Babel: ".. as
 * of 2.0, SetFunction() calls Forbid()/Permit() 
 * automatically  and also takes care of the code 
 * cache itself."
 */
void patch_functions() {

  ENTER

  old_CloseWindow=SetFunction((struct Library *)IntuitionBase, 
                              -72, 
			      (APTR) my_CloseWindow_SetFunc);

  old_OpenWindow=SetFunction((struct Library *)IntuitionBase, 
                              -204, 
			      (APTR) my_OpenWindow_SetFunc);

  old_OpenWindowTagList=SetFunction((struct Library *)IntuitionBase, 
                              -606, 
			      (APTR) my_OpenWindowTagList_SetFunc);

  old_OpenScreen=SetFunction((struct Library *)IntuitionBase, 
                              -198, 
			      (APTR) my_OpenScreen_SetFunc);

  old_OpenScreenTagList=SetFunction((struct Library *)IntuitionBase, 
                              -612, 
			      (APTR) my_OpenScreenTagList_SetFunc);


  LEAVE
}

/* TODO: unsafe! */
void unpatch_functions() {

  ENTER

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

  LEAVE
}

