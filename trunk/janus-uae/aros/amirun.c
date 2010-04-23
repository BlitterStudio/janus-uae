/************************************************************************ 
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE.
 *
 * Janus-UAE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Janus-UAE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id$
 *
 ************************************************************************/

#include <stdio.h>

#include <utility/utility.h>
#include <exec/exec.h>
#include <proto/exec.h>

/* send a lauch message to the j-uae cli port */

/* keep this in sync with j-cli-thread.c !! */
struct JUAE_Sync_Launch_Message {
  struct Message  ExecMessage;
  ULONG           type;
  STRPTR          ln_Name;
  struct TagItem *tags;
  void           *mempool;
  ULONG           error;
};

#define CLI_PORT_NAME     "J-UAE Run"

int main (int argc, char **argv) {
  struct JUAE_Sync_Launch_Message  *msg;
  struct Message                   *back_msg;
  struct MsgPort                   *port;
  struct MsgPort                   *back_port;

  printf("argc: %d\n", argc);
  if(argc < 2) {
    printf("usage: %s amiga_os_command [param..]\n", argv[0]);
    return -1;
  }

  if(!FindPort((STRPTR) CLI_PORT_NAME)) {
    printf("ERROR: port %s not open (j-uae/janusd not running)\n", CLI_PORT_NAME);
    return -1;
  }

  if(! ((back_port=CreateMsgPort() )) ) {
    printf("ERROR: could not open own message port!\n");
    return -1;
  }

  msg=AllocVec(sizeof(struct JUAE_Sync_Launch_Message), MEMF_CLEAR);
  msg->ln_Name=argv[1];
  if(msg) {
    Forbid();
    if(( port=FindPort((STRPTR) CLI_PORT_NAME))) {

      ((struct Message *) msg)->mn_ReplyPort=back_port;
      msg->type=2;

      printf("send msg message to %s..\n", CLI_PORT_NAME);
      PutMsg(port, msg); /* two way */

      printf("wait for answer from %s ..\n", CLI_PORT_NAME);
      back_msg=WaitPort(back_port);
    }
    Permit();
    printf("error: %d\n", msg->error);
    FreeVec(msg);
  }
  printf("send msg to %s done.\n", CLI_PORT_NAME);

  return 0;
}

