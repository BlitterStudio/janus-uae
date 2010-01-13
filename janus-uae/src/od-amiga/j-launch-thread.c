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

#include "j.h"
#include "memory.h"


/***********************************************************
 * This is the process, which watches the Execute
 * Port of J-UAE, so wanderer can send us messages.
 ***********************************************************/  
static void aros_launch_thread (void) {

  struct Process *thread = (struct Process *) FindTask (NULL);
  BOOL            done   = FALSE;
  ULONG           s;
  struct MsgPort *port   = NULL;

  /* There's a time to live .. */

  JWLOG("aros_launch_thread[%lx]: thread running \n",thread);

  JWLOG("aros_launch_thread[%lx]: open port \n",thread);

  port=CreateMsgPort();
  if(!port) {
    JWLOG("aros_launch_thread[%lx]: ERROR: failed to open port \n",thread);
    goto EXIT;
  }

  port->mp_Node.ln_Name = LAUNCH_PORT_NAME;
  port->mp_Node.ln_Type = NT_MSGPORT;
  port->mp_Node.ln_Pri  = 1; /* not too quick search */

  Forbid();
  if( !FindPort(port->mp_Node.ln_Name) ) {
    AddPort(port);
    Permit();
    JWLOG("aros_launch_thread[%lx]: new port %lx\n",thread, port);
  }
  else {
    Permit();
    JWLOG("aros_launch_thread[%lx]: port was already open (other j-uae running?)\n",thread);
    goto EXIT;
  }


  while(!done) {
    s=Wait(SIGBREAKF_CTRL_C);
    done=TRUE;

    /* Ctrl-C */
    if(s & SIGBREAKF_CTRL_C) {
      JWLOG("aros_launch_thread[%lx]: SIGBREAKF_CTRL_C received\n", thread);
      //done=TRUE;
    }
  }

  /* ... and a time to die. */

EXIT:
  JWLOG("aros_launch_thread[%lx]: EXIT\n",thread);

  JWLOG("aros_launch_thread[%lx]: dies..\n", thread);
}

int aros_launch_start_thread (void) {

    JWLOG("aros_launch_start_thread(x)\n");

    aros_launch_task = (struct Task *)
	    myCreateNewProcTags ( NP_Output, Output (),
				  NP_Input, Input (),
				  NP_Name, (ULONG) "j-uae launch proxy",
				  NP_CloseOutput, FALSE,
				  NP_CloseInput, FALSE,
				  NP_StackSize, 4096,
				  NP_Priority, 0,
				  NP_Entry, (ULONG) aros_launch_thread,
				  TAG_DONE);

    JWLOG("launch thread %lx created\n", aros_launch_task);

    return aros_launch_task != 0;
}

