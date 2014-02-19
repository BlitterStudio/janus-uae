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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#define DEBUG 1
#include "janus-daemon.h"

/* global */
ULONG           notify_signal;
struct MsgPort *notify_port;

/*********************************************************************************
 * setup_notify
 *
 * alloc notify signal
 *********************************************************************************/
static ULONG setup_notify(void) {

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

static APTR do_notify(struct Screen *screen, struct MsgPort *port, ULONG class, ULONG thread) {
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


/****************************************************
 * patch_functions
 *
 * Use notifications, wherever possible. On AmigaOS
 * systems (<4), SetPatch is required to force
 * notifications.
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
 * - CloseWindow
 *   If a window is closed, the clonewindow
 *   function has no chance, to detect that.
 *   So we need to detect a CloseWindow, *before*
 *   it is closed.
 *
 ****************************************************/
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

  //do_notify(NULL, notify_port, SNOTIFY_SCREENDEPTH       |SNOTIFY_WAIT_REPLY, thread);

  LEAVE
}

extern struct IntuitionBase* IntuitionBase;

/****************************************************
 * TODO(?)
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

void handle_notify_msg(ULONG notify_class, ULONG notify_object) {

  //notify_code      = notify_msg->snm_Code;
  //notify_userdata  = notify_msg->snm_UserData;

  DebOut("notify_class %lx, notify_object %lx, state %d\n", notify_class, notify_object, state);

  if(state) {

    switch(notify_class) {
      /*********** Window handling ************/
      case SNOTIFY_BEFORE_OPENWINDOW: DebOut("SNOTIFY_BEFORE_OPENWINDOW\n");
        update_screens();
        break;
      case SNOTIFY_AFTER_OPENWINDOW: DebOut("SNOTIFY_AFTER_OPENWINDOW\n");
        calltrap(AD_GET_JOB, AD_GET_JOB_NEW_WINDOW, (ULONG *) notify_object);
        break;

      case SNOTIFY_BEFORE_CLOSEWINDOW: DebOut("SNOTIFY_BEFORE_CLOSEWINDOW\n");
        calltrap(AD_GET_JOB, AD_GET_JOB_MARK_WINDOW_DEAD, (ULONG *) notify_object);
        break;
      /*case SNOTIFY_AFTER_CLOSEWINDOW: DebOut("SNOTIFY_AFTER_CLOSEWINDOW\n");
        break;*/


      /*********** Screen handling ************/
      /*case SNOTIFY_BEFORE_OPENSCREEN: DebOut("SNOTIFY_BEFORE_OPENSCREEN\n");
        remove tagging ..!? 
        break;*/
      case SNOTIFY_AFTER_OPENSCREEN: DebOut("SNOTIFY_AFTER_OPENSCREEN\n");
        update_screens();
        break;

      case SNOTIFY_BEFORE_CLOSESCREEN: DebOut("SNOTIFY_BEFORE_CLOSESCREEN\n");
        calltrap(AD_GET_JOB, AD_GET_JOB_CLOSE_SCREEN, (ULONG *) notify_object);
        break;
      /*case SNOTIFY_AFTER_CLOSESCREEN: DebOut("SNOTIFY_AFTER_CLOSESCREEN\n");
        break;*/

      case SNOTIFY_SCREENDEPTH: DebOut("SNOTIFY_SCREENDEPTH\n");
        /* WARNING: THIS IS MAYBE WRONG, have a llok at the 666 hack.. */
        calltrap(AD_GET_JOB, AD_GET_JOB_SCREEN_DEPTH, (ULONG *) notify_object);
        break;

      default: printf("unknown(%lx)\n", notify_class);
        break;
    }
  }
}
#endif
