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
#include <string.h>

#include <utility/utility.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>

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
#define MAX_PATH_LENGTH   512

ULONG make_path_absolute(STRPTR in, STRPTR out) {
  BPTR    lock;

  lock=Lock(in, SHARED_LOCK);
  if(!lock) {
    return 205;
  }
  NameFromLock(lock, out, MAX_PATH_LENGTH);
  UnLock(lock);

  return 0;
}

int main (int argc, char **argv) {
  struct JUAE_Sync_Launch_Message  *msg;
  struct Message                   *back_msg;
  struct MsgPort                   *port;
  struct MsgPort                   *back_port;
  char                              amiga_os_command[MAX_PATH_LENGTH+1];
  char                             *cmd;
  ULONG                             result;
  ULONG                             count;
  ULONG                             i;

  if(argc < 2) {
    printf("usage: %s amiga_os_command [param..]\n", argv[0]);
    return -1;
  }

  if(!FindPort((STRPTR) CLI_PORT_NAME)) {
    printf("ERROR: port \"%s\" not open (j-uae/janusd not running)\n", CLI_PORT_NAME);
    PrintFault(121, (STRPTR) argv[1]);
    return -1;
  }

  if(! ((back_port=CreateMsgPort() )) ) {
    printf("ERROR: could not open own message port!\n");
    PrintFault(103, (STRPTR) argv[0]);
    return -1;
  }

  result=make_path_absolute((STRPTR) argv[1], (STRPTR) amiga_os_command);
  if(result) {
    PrintFault(result, (STRPTR) argv[1]);
    return -1;
  }

  /* count required memory for command plus parameters */
  count=strlen(amiga_os_command)+1;
  i=2;
  while(i < argc) {
    count=count+strlen(argv[i])+1;
    i++;
  }

  cmd=AllocVec(count, MEMF_PUBLIC|MEMF_CLEAR);
  if(!cmd) {
    PrintFault(103, (STRPTR) argv[0]);
    return -1;
  }

  sprintf(cmd, "%s", amiga_os_command);
  i=2;
  while(i < argc) {
    sprintf(cmd, "%s %s", cmd, argv[i]);
    i++;
  }

  msg=AllocVec(sizeof(struct JUAE_Sync_Launch_Message), MEMF_PUBLIC|MEMF_CLEAR);
  msg->ln_Name=(STRPTR) cmd;
  if(msg) {
    Forbid();
    if(( port=FindPort((STRPTR) CLI_PORT_NAME))) {

      ((struct Message *) msg)->mn_ReplyPort=back_port;
      msg->type=2;

      PutMsg(port, (struct Message *) msg); /* two way */

      back_msg=WaitPort(back_port);
    }
    Permit();
    FreeVec(msg);
  }

  FreeVec(cmd);

  return 0;
}

