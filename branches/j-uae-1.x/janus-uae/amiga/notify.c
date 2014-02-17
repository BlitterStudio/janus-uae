/************************************************************************ 
 *
 * Janus-Daemon
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-Daemon.
 *
 * AmigaOS specific hooks/patches for syncing and triggering
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

#if __AROS__
#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include "janus-daemon.h"

ULONG           notify_signal;
struct MsgPort *notify_port;

/*********************************************************************************
 * setup_notify
 *
 * alloc notify signal
 *********************************************************************************/
ULONG setup_notify(void) {

  DebOut("entered\n");

  notify_port=CreateMsgPort();
  if(!notify_port) {
    DebOut("ERROR: no notify_port!?\n");
    printf("ERROR: no notify_port!?\n");
    exit(1);
  }
  notify_signal=1L << notify_port->mp_SigBit;

  return notify_signal;
}

APTR do_notify(struct Screen *screen, struct MsgPort *port, ULONG class, ULONG thread) {
  APTR notify;

  ENTER

  screen=IntuitionBase->FirstScreen;
  DebOut("do_notify(class %lx, screen %lx, port %lx, thread %lx)\n", class, screen, port, thread);

  if(screen) {
    notify=StartScreenNotifyTags(SNA_MsgPort,  port,
            SNA_UserData, screen,
            SNA_SigTask,  thread,
            SNA_Notify,   class,
            TAG_END); 
  }
  else {
    notify=StartScreenNotifyTags(SNA_MsgPort,  port,
            SNA_SigTask,  thread,
            SNA_Notify,   class,
            TAG_END); 

  }

  if(!notify) {
    DebOut("ERROR: StartScreenNotifyTags(%lx) failed\n", class);
    printf("ERROR: StartScreenNotifyTags(%lx) failed\n", class);
  }

  LEAVE

  return notify;
}


void patch_functions(void ) {
  ULONG thread= (ULONG) FindTask(NULL);

  ENTER

  setup_notify();

  DebOut("Installing notifications..\n");

  do_notify(NULL, notify_port, SNOTIFY_BEFORE_OPENWINDOW |SNOTIFY_WAIT_REPLY, thread);
  do_notify(NULL, notify_port, SNOTIFY_AFTER_OPENWINDOW  |SNOTIFY_WAIT_REPLY, thread);

  do_notify(NULL, notify_port, SNOTIFY_BEFORE_CLOSEWINDOW|SNOTIFY_WAIT_REPLY, thread);

  do_notify(NULL, notify_port, SNOTIFY_AFTER_OPENSCREEN  |SNOTIFY_WAIT_REPLY, thread);
  do_notify(NULL, notify_port, SNOTIFY_BEFORE_CLOSESCREEN|SNOTIFY_WAIT_REPLY, thread);

  do_notify(NULL, notify_port, SNOTIFY_SCREENDEPTH       |SNOTIFY_WAIT_REPLY, thread);

  LEAVE
}

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
 *
 * - CloseScreen
 *   We need it for custom screens
 *
 * - ModifyIDCMP
 *   MUI windows seem to use this quite
 *   often. Without this patch, menus
 *   might not work.
 *
 * - AddGadget/AddGList/RemoveGadget/RemoveGList
 *   If someone adds/removes gadgets to his 
 *   window, we might need to do that,
 *   too.
 *
 * - SetWindowTitles
 *   if they update, we follow
 *
 * - WindowLimits
 *   if it updates, we follow
 */

APTR           old_CloseWindow;
APTR           old_OpenWindow;
APTR           old_OpenWindowTagList;
APTR           old_CloseScreen;
APTR           old_OpenScreen;
APTR           old_OpenScreenTagList;
APTR           old_ScreenDepth;
APTR           old_ModifyIDCMP;
APTR           old_AddGadget;
APTR           old_AddGList;
APTR           old_RemoveGadget;
APTR           old_RemoveGList;
APTR           old_RefreshGList;
APTR           old_RefreshGadgets;
APTR           old_SetWindowTitles;
APTR           old_WindowLimits;

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

#ifndef __AROS__
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
	PUSHFULLSTACK
	"moveq #11,d0\n"
	"moveq #7,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPFULLSTACK
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
#endif /* AROS */


struct TagItem       mytags_linked[2]={ { SA_Draggable, FALSE}, { TAG_MORE, 0}};
struct TagItem       mytags[2]={ { SA_Draggable, FALSE}, { TAG_DONE, TAG_DONE}};
struct ExtNewScreen  nodrag_newscreen;
struct TagItem       taglist_mytags_linked[2]={ { SA_Draggable, FALSE}, { TAG_MORE, 0}};
struct TagItem       taglist_mytags[2]={ { SA_Draggable, FALSE}, { TAG_DONE, TAG_DONE}};


/*********************************************************************************
 * remove_dragging
 *
 * set SA_Draggable to FALSE, AROS has no screen dragging ;)
 *********************************************************************************/
#ifndef __AROS__
ULONG remove_dragging (struct NewScreen *newscreen __asm("a0")) {
#else
ULONG remove_dragging (struct NewScreen *newscreen) {
#endif
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
#ifndef __AROS__
ULONG remove_dragging_TagList (struct TagItem *original_tags __asm("a1")) {
#else
ULONG remove_dragging_TagList (struct TagItem *original_tags) {
#endif
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

void do_update_screens (void) {
  update_screens();
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
 * We don't want draggable screens, as we run into trouble with updates of
 * the background, if for example behind the amigaOS screen a native
 * aros screen should be visible. So we create a new NewScreen struct
 * and patch it so that it has SA_Draggable=FALSE. The original
 * struct stays intact (including the Taglist).
 *
 * We might need to define exceptions from that, as stuff like
 * Brilliance, which uses two screens at the same time, will
 * stop to work otherwise. This is a TODO!
 *
 * for calltrap:
 * AD_GET_JOB  11                   (d0)
 * AD_GET_JOB_OPEN_CUSTOM_SCREEN 13 (d1)
 * new screen                       (a0)
 *
 * ad_job_open_custom_screen is a dummy at the moment. Screens are
 * opened with sync-screen.
 *
 *********************************************************************************/
#ifndef __AROS__
__asm__("_my_OpenScreen_SetFunc:\n"
        PUSHA0
	"cmp.l #1,_patch_draggable\n"
	"bne no_patch_draggable\n"
        "movem.l d1-d7/a1-a6,-(SP)\n"
	"jsr _remove_dragging\n"      /* we get back a new struct Screen here in D0*/
        "movem.l (SP)+,d1-d7/a1-a6\n"
	/* call original function */
	"move.l d0,a0\n"
	"no_patch_draggable:\n"
	PUSHA3
	"move.l _old_OpenScreen,a3\n"
	"jsr (a3)\n"
	POPA3
	POPA0                        /* restore original struct Screen */
	/* check, if we are disabled */
	"cmp.l #1,_state\n"
	"blt openscreen_patch_disabled\n"
	PUSHFULLSTACK
	"jsr _update_screens\n"
	POPFULLSTACK
	"openscreen_patch_disabled:\n"
        "rts\n");

#if 0
	/* call AD_GET_JOB_OPEN_CUSTOM_SCREEN */
	"move.l d0,a0\n"
	"moveq #11,d0\n"
	"moveq #13,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
#endif

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
 *
 * AD_GET_JOB_OPEN_CUSTOM_SCREEN is not used anymore
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
	"jsr _update_screens\n"
	POPFULLSTACK
	"openscreentags_patch_disabled:\n"
	
        "rts\n");

#if 0
	/* call AD_GET_JOB_OPEN_CUSTOM_SCREEN */
	"move.l d0,a0\n"
	"moveq #11,d0\n"
	"moveq #13,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
#endif

/*********************************************************************************
 * _my_CloseScreen_SetFunc
 *
 * done in assembler, as C-Source always at least destroys the
 * a5 register for the local variable stack frame. But library
 * functions may only trash d0, d1, a0 and a1.
 *
 * for calltrap:
 * AD_GET_JOB  11 (d0)
 * AD_GET_JOB_CLOSE_SCREEN 14 (d1)
 * screen is already in a0
 *********************************************************************************/
__asm__("_my_CloseScreen_SetFunc:\n"
	"cmp.l #1,_state\n"
	"blt close_screen_patch_disabled\n"
	PUSHSTACK
	"moveq #11,d0\n"
	"moveq #14,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPSTACK
	"close_screen_patch_disabled:\n"
	PUSHA3
	"move.l _old_CloseScreen, a3\n"
	"jsr (a3)\n"
	POPA3

        "rts\n");

/*********************************************************************************
 * _my_ScreenDepth_SetFunc
 *
 * done in assembler, as C-Source always at least destroys the
 * a5 register for the local variable stack frame. But library
 * functions may only trash d0, d1, a0 and a1.
 *
 * We won't let the screen depth change here. We call uae, let
 * it change the AROS screen for us and then wait, until
 * janusd changes the screen itself (with A1 == 666).
 *
 * For the moment, we only patch SDEPTH_TOBACK, but I am
 * unsure, if this is the right thing to do.
 *
 * for calltrap:
 * AD_GET_JOB  11 (d0)
 * AD_GET_JOB_SCREEN_DEPTH 16 (d1)
 * screen is already in a0
 * send flags in d3, as d0 is used for AD_GET_JOB
 *********************************************************************************/
__asm__("_my_ScreenDepth_SetFunc:\n"
	"cmp.l #1,_state\n"
	"blt call_old_ScreenDepth\n"
	"cmp.l #666, a1\n"
	"beq call_old_ScreenDepth\n"
	"cmp.l #1, d0\n"
	"bne call_old_ScreenDepth\n"
	PUSHSTACK
	"move.l d0,d3\n"
	"moveq #11,d0\n"
	"moveq #16,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPSTACK
        "rts\n"
	"call_old_ScreenDepth:\n"
	PUSHA3
	"sub.l a1, a1\n"
	"move.l _old_ScreenDepth, a3\n"
	"jsr (a3)\n"
	POPA3
	"rts\n"
	);

/*********************************************************************************
 * _my_ModifyIDCMP_SetFunc
 *
 * done in assembler, as C-Source always at least destroys the
 * a5 register for the local variable stack frame. But library
 * functions may only trash d0, d1, a0 and a1.
 *
 * for calltrap:
 * AD_GET_JOB  11 (d0)
 * AD_GET_JOB_MODIFY_IDCMP 17 (d1)
 * new IDCMP flags (d3)
 * window (a0)
 *********************************************************************************/
__asm__("_my_ModifyIDCMP_SetFunc:\n"
	PUSHFULLSTACK
	"move.l d0,d3\n"
	"moveq #11,d0\n"
	"moveq #17,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPFULLSTACK
	PUSHA3
	"move.l _old_ModifyIDCMP, a3\n"
	"jsr (a3)\n"
	POPA3
	"rts\n"
	);

/*********************************************************************************
 * _my_AddGadget_SetFunc
 *
 * done in assembler, as C-Source always at least destroys the
 * a5 register for the local variable stack frame. But library
 * functions may only trash d0, d1, a0 and a1.
 *
 * We call the original function, then we need to react on the
 * changes.
 *
 * for calltrap:
 * AD_GET_JOB  11 (d0)
 * AD_GET_JOB_UPDATE_GADGETS 18 (d1)
 * window is already in a0
 *********************************************************************************/
__asm__("_my_AddGadget_SetFunc:\n"
	PUSHA0
	PUSHA3
	"move.l _old_AddGadget, a3\n"
	"jsr (a3)\n"
	POPA3
	POPA0

	"cmp.l #1,_state\n"
	"blt addgadget_patch_disabled\n"
	PUSHSTACK
	"moveq #11,d0\n"
	"moveq #18,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPSTACK
	"addgadget_patch_disabled:\n"

        "rts\n");

__asm__("_my_AddGList_SetFunc:\n"
	PUSHA0
	PUSHA3
	"move.l _old_AddGList, a3\n"
	"jsr (a3)\n"
	POPA3
	POPA0

	"cmp.l #1,_state\n"
	"blt addglist_patch_disabled\n"
	PUSHSTACK
	"moveq #11,d0\n"
	"moveq #18,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPSTACK
	"addglist_patch_disabled:\n"

        "rts\n");

__asm__("_my_RemoveGadget_SetFunc:\n"
	PUSHA0
	PUSHA3
	"move.l _old_RemoveGadget, a3\n"
	"jsr (a3)\n"
	POPA3
	POPA0

	"cmp.l #1,_state\n"
	"blt removegadget_patch_disabled\n"
	PUSHSTACK
	"moveq #11,d0\n"
	"moveq #18,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPSTACK
	"removegadget_patch_disabled:\n"

        "rts\n");

__asm__("_my_RemoveGList_SetFunc:\n"
	PUSHA0
	PUSHA3
	"move.l _old_RemoveGList, a3\n"
	"jsr (a3)\n"
	POPA3
	POPA0

	"cmp.l #1,_state\n"
	"blt removeglist_patch_disabled\n"
	PUSHSTACK
	"moveq #11,d0\n"
	"moveq #18,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPSTACK
	"removeglist_patch_disabled:\n"

        "rts\n");

__asm__("_my_RefreshGList_SetFunc:\n"
	"movem.l a0/a1/a3,-(SP)\n"  
	"move.l _old_RefreshGList, a3\n"
	"jsr (a3)\n"
	"movem.l (SP)+,a0/a1/a3\n"  

	"cmp.l #1,_state\n"
	"blt refreshglist_patch_disabled\n"
	PUSHSTACK
	"move.l a1,a3\n" /* supply window parameter in a3 */
	"moveq #11,d0\n"
	"move.l #18,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPSTACK
	"refreshglist_patch_disabled:\n"

        "rts\n");

__asm__("_my_RefreshGadgets_SetFunc:\n"
	"movem.l a0/a1/a3,-(SP)\n"  
	"move.l _old_RefreshGadgets, a3\n"
	"jsr (a3)\n"
	"movem.l (SP)+,a0/a1/a3\n"  

	"cmp.l #1,_state\n"
	"blt refreshgadgets_patch_disabled\n"
	PUSHSTACK
	"move.l a1,a3\n" /* supply window parameter in a3 */
	"moveq #11,d0\n"
	"move.l #18,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	POPSTACK
	"refreshgadgets_patch_disabled:\n"

        "rts\n");

/*********************************************************************************
 * _my_SetWindowTitles_SetFunc
 *
 * done in assembler, as C-Source always at least destroys the
 * a5 register for the local variable stack frame. But library
 * functions may only trash d0, d1, a0 and a1.
 *
 * We call the original function, then we need to react on the
 * changes.
 *
 * for calltrap:
 * AD_GET_JOB  11 (d0)
 * AD_GET_JOB_SET_WINDOW_TITLES 19 (d1)
 * window is already in a0, we don't need the rest
 *********************************************************************************/

__asm__("_my_SetWindowTitles_SetFunc:\n"

	"cmp.l #1,_state\n"
	"blt setwindowtitles_patch_disabled_test\n"

    "movem.l d1-d7/a0-a6,-(SP)\n"    /* backup everything but D0  */
	  "jsr _set_aros_titles\n"         /* return in D0, if there is something to change */
	  "movem.l (SP)+,d1-d7/a0-a6\n"    /* restore everything but D0 */
	  "move.l d0,-(SP)\n"              /* backup D0  */

	"setwindowtitles_patch_disabled_test:\n"

	/* call original function */
	PUSHA0
	PUSHA3
	"move.l _old_SetWindowTitles, a3\n"
	"jsr (a3)\n"
	POPA3
	POPA0

	"cmp.l #1,_state\n"
	"blt setwindowtitles_patch_disabled\n"

	  "move.l (SP)+, d0\n"    /* restore D0  */
	  "cmp.l #0,d0\n"
	  "beq setwindowtitles_patch_disabled\n" /* nothing to do */
	    PUSHSTACK
	    "moveq #11,d0\n"
	    "moveq #19,d1\n"
	    "move.l _calltrap,a1\n"
	    "jsr (a1)\n"
	    POPSTACK
	"setwindowtitles_patch_disabled:\n"

        "rts\n");


/*********************************************************************************
 * _my_WindowLimits_SetFunc
 *
 * done in assembler, as C-Source always at least destroys the
 * a5 register for the local variable stack frame. But library
 * functions may only trash d0, d1, a0 and a1.
 *
 * We call the original function, then we need to react on the
 * changes.
 *
 * for calltrap:
 * AD_GET_JOB  11 (d0)
 * AD_GET_JOB_WINDOW_LIMITS 20 (d1)
 * window is already in a0, so we move d0,d1,d2,d3 to d2,d3,d4,d5, 
 * as d0/d1 are trashed by our service numbers
 * we store the return code of the original WindowLimits function in d6, to
 * return it later on
 *********************************************************************************/

__asm__("_my_WindowLimits_SetFunc:\n"
	/* we trash d6 to store the amigaOS result, so back it up */
	"move.l d6,-(SP)\n"
	/* amigaOS WindowLimits may trash d0-d3/a0, but we need it later */
	"movem.l d0-d3/a0,-(SP)\n"
	PUSHA3
	"move.l _old_WindowLimits, a3\n"
	"jsr (a3)\n"
	POPA3
	/* store amigaOS WindowLimits result in d6 */
	"move.l d0,d6\n"
	/* restore original WindowLimits parameter */
	"movem.l (SP)+,d0-d3/a0\n"
	
	"cmp.l #1,_state\n"
	"blt windowlimits_patch_disabled\n"
	/* backup everything */
	PUSHFULLSTACK
	/* shift registers, as we use d0/d1 for job identifier */
	"move.l d3,d5\n"
	"move.l d2,d4\n"
	"move.l d1,d3\n"
	"move.l d0,d2\n"
	"moveq #11,d0\n"
	"moveq #20,d1\n"
	"move.l _calltrap,a1\n"
	"jsr (a1)\n"
	/* restore  everything, don't care for calltrap result */
	POPFULLSTACK
	"windowlimits_patch_disabled:\n"
	/* restore amigaOS WindowLimits result from d6 */
	"move.l d6,d0\n"
	"move.l (SP)+,d6\n"
        "rts\n");
#endif /* AROS */

/*
 * assembler functions need to be declared or used, before
 * you can reference them (?).
 */
void my_CloseWindow_SetFunc();
void my_OpenWindow_SetFunc();
void my_OpenWindowTagList_SetFunc();
void my_OpenScreen_SetFunc();
void my_OpenScreenTagList_SetFunc();
void my_CloseScreen_SetFunc();
void my_ScreenDepth_SetFunc();
void my_ModifyIDCMP_SetFunc();
void my_AddGadget_SetFunc();
void my_AddGList_SetFunc();
void my_RemoveGadget_SetFunc();
void my_RemoveGList_SetFunc();
void my_SetWindowTitles_SetFunc();
void my_WindowLimits_SetFunc();
void my_RefreshGadgets_SetFunc();
void my_RefreshGList_SetFunc();

/* According to Ralph Babel: ".. as
 * of 2.0, SetFunction() calls Forbid()/Permit() 
 * automatically  and also takes care of the code 
 * cache itself."
 */
void patch_functions_amigaos() {

  ENTER

#ifndef __AROS__
  old_CloseWindow=SetFunction((struct Library *)IntuitionBase, 
                              -72, 
			      (APTR) my_CloseWindow_SetFunc);

  /* AROS version uses SNOTIFY_BEFORE_OPENWINDOW */
  old_OpenWindow=SetFunction((struct Library *)IntuitionBase, 
                              -204, 
			      (APTR) my_OpenWindow_SetFunc);

  old_OpenWindowTagList=SetFunction((struct Library *)IntuitionBase, 
                              -606, 
			      (APTR) my_OpenWindowTagList_SetFunc);

  old_CloseScreen=SetFunction((struct Library *)IntuitionBase, 
                              -66, 
			      (APTR) my_CloseScreen_SetFunc);

  old_OpenScreen=SetFunction((struct Library *)IntuitionBase, 
                              -198, 
			      (APTR) my_OpenScreen_SetFunc);

  old_OpenScreenTagList=SetFunction((struct Library *)IntuitionBase, 
                              -612, 
			      (APTR) my_OpenScreenTagList_SetFunc);

  old_ScreenDepth=SetFunction((struct Library *)IntuitionBase, 
                              -786, 
			      (APTR) my_ScreenDepth_SetFunc);
#endif

#if 0
  old_ModifyIDCMP=SetFunction((struct Library *)IntuitionBase, 
                              -150, 
			      (APTR) my_ModifyIDCMP_SetFunc);
#endif

#if 0
  old_AddGadget=SetFunction((struct Library *)IntuitionBase, 
                              -42, 
			      (APTR) my_AddGadget_SetFunc);

  old_AddGList=SetFunction((struct Library *)IntuitionBase, 
                              -438, 
			      (APTR) my_AddGList_SetFunc);

  old_RemoveGadget=SetFunction((struct Library *)IntuitionBase, 
                              -228, 
			      (APTR) my_RemoveGadget_SetFunc);

  old_RemoveGList=SetFunction((struct Library *)IntuitionBase, 
                              -444, 
			      (APTR) my_RemoveGList_SetFunc);
#endif

#ifndef __AROS__
  old_SetWindowTitles=SetFunction((struct Library *)IntuitionBase, 
                              -276, 
			      (APTR) my_SetWindowTitles_SetFunc);
#endif

#ifndef __AROS__
  old_WindowLimits=SetFunction((struct Library *)IntuitionBase, 
                              -318, 
			      (APTR) my_WindowLimits_SetFunc);

  old_RefreshGadgets=SetFunction((struct Library *)IntuitionBase, 
                              -222, 
			      (APTR) my_RefreshGadgets_SetFunc);

  old_RefreshGList=SetFunction((struct Library *)IntuitionBase, 
                              -432, 
			      (APTR) my_RefreshGList_SetFunc);
#else
#warning TODO!!!!!!!!!!!!!!!!!!!!!!1
#endif



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

#endif
