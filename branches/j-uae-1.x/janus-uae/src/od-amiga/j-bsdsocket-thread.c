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

#include "j.h"
#include "memory.h"
#include "include/filesys.h"
#include "include/filesys_internal.h"
#include "bsdsocket.h"


#define BSDLOG_ENABLED 1
#if BSDLOG_ENABLED
#define BSDLOG(...)   do { kprintf("[%lx]%s:%d %s(): ",FindTask(NULL),__FILE__,__LINE__,__func__);kprintf(__VA_ARGS__); } while(0)
#else
#define BSDLOG(...)     do { ; } while(0)
#endif

/***********************************************************
 * 
 * 
 ***********************************************************/  
static void aros_bsdsocket_thread (void) {

  struct Process                *thread = (struct Process *) FindTask (NULL);
  struct MsgPort                *port   = NULL;
  struct JUAE_bsdsocket_Message *msg;
  BOOL                           done   = FALSE;

  ULONG           error;

  /* There's a time to live .. */

  BSDLOG("aros_bsdsocket_thread[%lx]: thread running \n",thread);

  port=(struct MsgPort *) &thread->pr_MsgPort;

  while(!done) {
    BSDLOG("while(!done)\n");
    WaitPort(port);
    while( (msg = (struct JUAE_bsdsocket_Message *) GetMsg(port)) ) {
      BSDLOG("+++++++++++++++++++ got JUAE_bsdsocket_Message +++++++++++++++++++\n");

      switch (msg->cmd) {
        case BSD_gethostbynameaddr:
          host_gethostbynameaddr_real((TrapContext *)msg->a, 
                                      (struct socketbase *)msg->b,
                                      (uae_u32) msg->c,
                                      (uae_u32) msg->d,
                                      (long) msg->e);
          break;
        case BSD_socket:
          msg->ret=host_socket_real((struct socketbase *)msg->a,
                                    (int)msg->b,
                                    (int)msg->c,
                                    (int)msg->d);
          break;
        case BSD_setsockopt:
          host_setsockopt_real((struct socketbase *)msg->a,
                               (uae_u32)msg->b,
                               (uae_u32)msg->c,
                               (uae_u32)msg->d,
                               (uae_u32)msg->e,
                               (uae_u32)msg->f);
          break;
        case BSD_connect:
          host_connect_real((TrapContext *)msg->a,
                               (struct socketbase *)msg->b,
                               (uae_u32)msg->c,
                               (uae_u32)msg->d,
                               (uae_u32)msg->e);
          break;
        case BSD_sendto:
          host_sendto_real((TrapContext *)msg->a,
                           (struct socketbase *)msg->b,
                           (uae_u32)msg->c,
                           (uae_u32)msg->d,
                           (uae_u32)msg->e,
                           (uae_u32)msg->f,
                           (uae_u32)msg->g,
                           (uae_u32)msg->h);
          break;
        case BSD_recvfrom:
          host_recvfrom_real((TrapContext *)msg->a,
                           (struct socketbase *)msg->b,
                           (uae_u32)msg->c,
                           (uae_u32)msg->d,
                           (uae_u32)msg->e,
                           (uae_u32)msg->f,
                           (uae_u32)msg->g,
                           (uae_u32)msg->h);
          break;
        case BSD_inet_addr:
          msg->ret=host_inet_addr_real((TrapContext *)msg->a,
                                       (struct socketbase *)msg->b,
                                       (uae_u32)msg->c);
          break;
        case BSD_getprotobyname:
          host_getprotobyname_real((TrapContext *)msg->a,
                           (struct socketbase *)msg->b,
                           (uae_u32)msg->c);
          break;
        case BSD_Inet_NtoA:
          msg->ret=host_Inet_NtoA_real((TrapContext *)msg->a,
                                       (struct socketbase *)msg->b,
                                       (uae_u32)msg->c);
          break;
        case BSD_CloseSocket:
          msg->ret=host_CloseSocket_real((struct socketbase *)msg->a,
                                         (int)msg->b);
          break;
        case BSD_dup2socket:
          msg->ret=host_dup2socket_real((struct socketbase *)msg->a,
                                         (int)msg->b,
                                         (int)msg->c);
          break;
        case BSD_bind:
          msg->ret=host_bind_real((struct socketbase *)msg->a,
                                         (uae_u32)msg->b,
                                         (uae_u32)msg->c,
                                         (uae_u32)msg->d);
          break;
        case BSD_gethostname:
          msg->ret=host_gethostname_real((struct socketbase *)msg->a,
                                         (uae_u32)msg->b,
                                         (uae_u32)msg->c);
          break;


        case BSD_killme:
          aros_bsdsocket_kill_thread_real((struct socketbase *)msg->a);
          done=TRUE;
          /* ATTENTION: don't touch anything now. Just replay the last message and exit at once! */
          break;

        default:
          BSDLOG("ERROR: command %d NOT KNOWN TO ME!?\n",msg->cmd);
      }

      BSDLOG("ReplyMsg ..\n");
      ReplyMsg ((struct Message *) msg);
      BSDLOG("ReplyMsg done\n");
    }
  }
  /* end thread */

EXIT:
  BSDLOG("aros_bsdsocket_thread[%lx]: dies..\n", thread);
}

int aros_bsdsocket_start_thread (struct socketbase *sb) {

  BSDLOG("aros_bsdsocket_start_thread()\n");

  if(!sb) {
    BSDLOG("ERROR: sb is NULL !?\n");
    return 0;
  }

  BSDLOG("sb: %lx\n", sb);

  sb->mempool=CreatePool(MEMF_CLEAR|MEMF_SEM_PROTECTED, 0xC000, 0x8000);
  BSDLOG("mempool: %lx\n", sb->mempool);

  if(!sb->mempool) {
    BSDLOG("ERROR: no memory for pool!?\n");
    return 0;
  }

  sb->name=AllocVecPooled(sb->mempool, 8+strlen(BSDSOCKET_PREFIX_NAME)+1);
  sprintf(sb->name,"%s%lx", BSDSOCKET_PREFIX_NAME, sb->ownertask);

  sb->aros_task = (struct Process *)
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
    BSDLOG("bsdsocket thread %lx created (%s)\n", sb->aros_task, sb->name);
    sb->aros_port=(struct MsgPort *) &(sb->aros_task)->pr_MsgPort;
    //sb->reply_port=CreateMsgPort(); /* TODO: check for error */
    //BSDLOG("sb->reply_port=%lx\n", sb->reply_port);
  }
  else {
    BSDLOG("ERROR: failed to create new process!\n");
  }

  return aros_launch_task != 0;
}


