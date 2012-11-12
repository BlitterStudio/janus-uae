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

#include <utility/utility.h>
#include <workbench/workbench.h>
#include <proto/utility.h>

//#define JWTRACING_ENABLED 1
#include "j.h"
#include "memory.h"
#include "include/filesys.h"
#include "include/filesys_internal.h"

#if 0
/* we get a JUAE_Sync_Launch_Message for a synchronous command  */
struct JUAE_Sync_Launch_Message {
  struct Message  ExecMessage;
  ULONG           type;
  STRPTR          ln_Name;
  struct TagItem *tags;
  void           *mempool;
  ULONG           error;
};
#endif



/***********************************************************
 * This is the process, which watches the BSD Execute
 * Port of J-UAE, so wanderer can send us messages.
 ***********************************************************/  
static void aros_bsdsocket_thread (void) {

  struct Process *thread = (struct Process *) FindTask (NULL);
  BOOL            done   = FALSE;
  ULONG           s;
  struct MsgPort *port   = NULL;
  char           *amiga_exe;
  JanusLaunch    *jlaunch;
  ULONG           error;
  struct JUAE_Sync_Launch_Message *msg;

  /* There's a time to live .. */

  JWLOG("aros_bsdsocket_thread[%lx]: thread running \n",thread);

  JWLOG("aros_bsdsocket_thread[%lx]: open port %s\n",thread, BSD_PORT_NAME);

  port=CreateMsgPort();
  if(!port) {
    JWLOG("aros_bsdsocket_thread[%lx]: ERROR: failed to open port \n",thread);
    goto EXIT;
  }

  port->mp_Node.ln_Name = AllocVec(strlen(BSD_PORT_NAME)+1, MEMF_PUBLIC);
  strcpy(port->mp_Node.ln_Name, BSD_PORT_NAME);
  port->mp_Node.ln_Type = NT_MSGPORT;
  port->mp_Node.ln_Pri  = 1; /* not too quick search */

  Forbid();
  if( !FindPort(port->mp_Node.ln_Name) ) {
    AddPort(port);
    Permit();
    JWLOG("aros_bsdsocket_thread[%lx]: new port %lx\n",thread, port);
  }
  else {
    Permit();
    JWLOG("aros_bsdsocket_thread[%lx]: port was already open (other j-uae running?)\n",thread);
    goto EXIT;
  }


  while(!done) {

    WaitPort(port);
    JWLOG("aros_bsdsocket_thread[%lx]: WaitPort done\n",thread);
    while( (msg = (struct JUAE_Sync_Launch_Message *) GetMsg(port)) ) {
      JWLOG("aros_bsdsocket_thread[%lx]: msg %lx received!\n", thread, msg);
      JWLOG("aros_bsdsocket_thread[%lx]: msg->ln_Name: >%s< \n", thread, msg->ln_Name);
      error=0;

#if 0
      switch (msg->type) {
	case BSD_TYPE_DIE:
	  done=TRUE;
	  break;
	case BSD_TYPE_BSD_ASYNC:
	  amiga_exe=aros_path_to_amigaos(msg->ln_Name);
	  if(amiga_exe) {
	    /* store it for the launchd to fetch and execute */
	    ObtainSemaphore(&sem_janus_launch_list);
	    jlaunch=(JanusLaunch *) AllocVec(sizeof(JanusLaunch),MEMF_CLEAR);
	    if(jlaunch) {
	      jlaunch->type      =BSD_TYPE_BSD_ASYNC;
	      jlaunch->amiga_path=amiga_exe;
	      jlaunch->args      =NULL;
	      janus_launch       =g_slist_append(janus_launch,jlaunch);
	    }
    
	    ReleaseSemaphore(&sem_janus_launch_list);
	    JWLOG("aros_bsdsocket_thread[%lx]: uae_Signal(%lx, %lx)\n", thread, aos3_launch_task, aos3_launch_signal);
	    uae_Signal(aos3_launch_task, aos3_launch_signal);
	  }
	  else {
	    msg->error=ERROR_OBJECT_NOT_FOUND;
	    JWLOG(    "ERROR: not reachable from AmigaOS: %s\n", msg->ln_Name);
	    write_log("ERROR: not reachable from AmigaOS: %s\n", msg->ln_Name);
	  }
	  break;
	default:
	  JWLOG("aros_bsdsocket_thread[%lx]: unknown message type %d\n", msg->type);
      }
#endif

      ReplyMsg ((struct Message *) msg);

      if(error) {
#if 0
	    gui_message_with_title("ERROR",
				   "Failed to start \"%s\".\n\nThis path is not available inside of amigaOS.\n\nYou need to add the AROS directory\n\n(or one of its parents)\n\nas an amigaOS device.\n\nBest way to do this is the \"Harddisk\" tab in the J-UAE GUI.", msg->ln_Name);
#endif
      }
    }
  }


  JWLOG("aros_bsdsocket_thread[%lx]: EXIT\n",thread);
  /* remove port */
  Forbid();
  /* We hope, there are no messages waiting anymore.
   * As this port is not really busy, this is not unlikely (hopefully).
   * Worst case is, that we loose the memory of the waiting messages?
   */
  RemPort(port);
  Permit();

  if(port->mp_Node.ln_Name) {
    FreeVec(port->mp_Node.ln_Name);
  }
  port=NULL;

  /* end thread */
EXIT:
  aros_cli_task=NULL;
  JWLOG("aros_bsdsocket_thread[%lx]: dies..\n", thread);
}

int aros_bsdsocket_start_thread (struct socketbase *sb) {

  JWLOG("aros_bsdsocket_start_thread()\n");

  if(!sb) {
    JWLOG("ERROR: sb is NULL !?\n");
    return 0;
  }

  sb->mempool=CreatePool(MEMF_CLEAR|MEMF_SEM_PROTECTED, 0xC000, 0x8000);

  if(!sb->mempool) {
    JWLOG("ERROR: no memory for pool!?\n");
    return 0;
  }

  sb->name=AllocVecPooled(sb->mempool, 8+strlen(BSDSOCKET_PREFIX_NAME)+1);
  sprintf(sb->name,"%s%lx", BSDSOCKET_PREFIX_NAME, sb->ownertask);

  sb->aros_task = (struct Task *)
	    myCreateNewProcTags ( //NP_Output,      Output (),
				  //NP_Input,       Input (),
				  NP_Name,        (ULONG) sb->name,
				  //NP_CloseOutput, FALSE,
				  //NP_CloseInput,  FALSE,
				  NP_StackSize,   49152,
				  NP_Priority,    0,
				  NP_Entry,       (ULONG) aros_bsdsocket_thread,
				  TAG_DONE);

  if(sb->aros_task) {
    JWLOG("bsdsocket thread %lx created (%s)\n", sb->aros_task, sb->name);
  }
  else {
    JWLOG("ERROR: failed to create new process!\n");
  }

  return aros_launch_task != 0;
}

void aros_bsdsocket_kill_thread(void) {
  struct JUAE_Sync_Launch_Message  *die_msg;
  struct Message                   *back_msg;
  struct MsgPort                   *port;
  struct MsgPort                   *back_port;

  JWLOG("aros_cli_kill_thread()\n");

  if(!aros_cli_task) {
    JWLOG("no aos3 cli task\n");
    return;
  }

  if(!FindPort((STRPTR) BSD_PORT_NAME)) {
    JWLOG("port %s not open\n", BSD_PORT_NAME);
    return;
  }

  if(! ((back_port=CreateMsgPort() )) ) {
    JWLOG("ERROR: could not create message port!\n");
    return;
  }

#if 0
  die_msg=AllocVec(sizeof(struct JUAE_Sync_Launch_Message), MEMF_CLEAR);
  if(die_msg) {
    Forbid();
    if(( port=FindPort((STRPTR) BSD_PORT_NAME))) {

      ((struct Message *) die_msg)->mn_ReplyPort=back_port;
      die_msg->type=BSD_TYPE_DIE;

      JWLOG("send DIE message to %s..\n", BSD_PORT_NAME);
      PutMsg(port, die_msg); /* two way */

      JWLOG("wait for answer from %s ..\n", BSD_PORT_NAME);
      back_msg=WaitPort(back_port);
    }
    Permit();
    FreeVec(die_msg);
  }
#endif

  if(back_port) {
    DeleteMsgPort(back_port);
    back_port=NULL;
  }

  JWLOG("send DIE to %s done.\n", BSD_PORT_NAME);
}
