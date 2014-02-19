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

#include <stdio.h>
#include <stdlib.h>

#include <proto/exec.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <exec/nodes.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <intuition/intuitionbase.h>
#ifdef __AROS__
#include <proto/intuition.h>
#include <intuition/extensions.h>
#endif

#define DEBUG 1
#include "janus-daemon.h"

extern struct IntuitionBase* IntuitionBase;

#if 0
/* this would be a much nicer way, to forward a close message to
 * the amigaos window, but:
 * - aos3 CLI windows have no UserPort to get messages (?)
 * - it does not work always (?)
 * => so we keep it simple and stupid .. (see below)
 */
void closewin(struct Window *w) {
  struct MsgPort *myport;
  struct MsgPort *winport;
  struct IntuiMessage *closemsg;
  struct Message *backmsg;

  /* use Forbid() here, if you want to be sure, your window is still around */
  if(!w) {
    printf("window is NULL\n");
    return;
  }

  /* this is only a prototype, in real life, you should check, 
   * if w is really a window!  */

  printf("try to close window %lx (%s)\n",(ULONG) w,w->Title);

  winport=w->UserPort;
  printf("window->UserPort: %lx\n",(ULONG) winport);
  if(!winport) {
    /* Cli has no UserPort ..? */
    printf("ERROR: window %lx has no UserPort !?\n",(ULONG) w);
    return;
  }

  myport=CreateMsgPort();
  if(!myport) {
    printf("ERROR: no port!\n");
    return;
  }

  closemsg=AllocVec(sizeof(struct IntuiMessage),MEMF_CLEAR|MEMF_PUBLIC);

  ((struct Message *)closemsg)->mn_Length=sizeof(closemsg);
  ((struct Message *)closemsg)->mn_ReplyPort=myport;
  closemsg->Class=IDCMP_CLOSEWINDOW;
  closemsg->Qualifier=0x800;

  printf("send msg %lx ..\n",(ULONG) closemsg);
  PutMsg(winport, (struct Msg *) closemsg);
  printf("sent msg\n");

  printf("WaitPort(%lx)\n",(ULONG) myport);
  backmsg=WaitPort(myport);
  printf("backmsg: %lx\n",(ULONG) backmsg);

  if(backmsg != (struct Msg *) closemsg) {
    printf("FATAL ERROR: Got back another message as we sent !?\n");
    /* this might crash !! */
  }
  FreeVec(closemsg);
  DeleteMsgPort(myport);
  return;
}
#endif


#ifndef __AROS__
/* just "press" the button */
void closewin(struct Window *w) {
  UWORD m;
  ULONG lock;
  struct Screen *scr;
  WORD  x,y;

  ENTER

  DebOut("closewin(%p)\n", w);

  /* who knows, maybe window was closed inbetween already */
  DebOut("LockIBase()\n");
  lock=LockIBase(0);
  if(!assert_window(w)) {
    DebOut("window %p was already closed!\n", w);
    printf("window %p was already closed!\n", w);
    DebOut("UnlockIBase()\n");
    UnlockIBase(lock);
    LEAVE
    return;
  }

  /* get all window attributes now, as long as we hold the intui lock */
  m=w->BorderTop / 2; /* middle of close gadget */

  x  =w->LeftEdge + m;
  y  =w->TopEdge + m;
  scr=w->WScreen;

  DebOut("m %d, x %d, y %d, scr %p\n", m, x, y, scr);

  DebOut("UnlockIBase()\n");
  UnlockIBase(lock);

  SetMouse(scr, x, y, IECODE_LBUTTON, TRUE, TRUE);

  LEAVE

  /* next sync sets the mouse back to the right coordinates */
}
#else

/* use WindowAction, safe, even if window is closed inbetween, nice. */
void closewin(struct Window *w) {

  ENTER

  DebOut("send WAC_SENDIDCMPCLOSE to window %lx\n", w);

  WindowActionTags(w, WAC_SENDIDCMPCLOSE,
                   TAG_DONE);

  LEAVE
}
#endif

/*****************************************
 * check, if new IDCMP messages for 
 * amigaOS windows are waiting and send
 * them to the right message ports.
 *****************************************/
void forward_messages() {
  ULONG *command_mem;
  UBYTE  done;
  ULONG  type;
  struct Window *w;

  ENTER

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);

  done=0;
  while(!done) {

    calltrap (AD_GET_JOB, AD_GET_JOB_MESSAGES, command_mem);
    if(!command_mem[0]) {
      done=1; /* nothing more to do */
    }
    else {
      type=command_mem[0];
      DebOut("got message (type %d)\n",(unsigned int) type);
      switch(type) {
        case J_MSG_CLOSE:
          w=(struct Window *) command_mem[1];
          closewin(w);
          break;
        default:
          DebOut("unknown message type: %d\n",(unsigned int) type);
      }
    }

  }

  FreeVec(command_mem);

  LEAVE
}

