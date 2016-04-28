/************************************************************************ 
 *
 * posixemu.cpp
 *
 * Copyright 2014 Oliver Brunner - aros<at>oliver-brunner.de
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

#include "sysconfig.h"
#include "sysdeps.h"

#include <exec/exec.h>
#include <exec/ports.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>


#include <proto/exec.h>
#include <proto/dos.h>

#include "threaddep/thread.h"


struct startupmsg
{
    struct Message msg;
    void           *(*func) (void *);
    void           *arg;
};

static void do_thread (void)
{
    struct Process *pr = (struct Process *) FindTask (NULL);
    struct startupmsg *msg;
    void *(*func) (void *);
    void *arg;

    bug("[JUAE:PX] %s()\n", __PRETTY_FUNCTION__);
    bug("[JUAE:PX] %s: task = %p\n", __PRETTY_FUNCTION__, pr);

    WaitPort (&pr->pr_MsgPort);
    msg = (struct startupmsg *) GetMsg(&pr->pr_MsgPort);
    func = msg->func;
    arg  = msg->arg;
    ReplyMsg ((struct Message*)msg);

    func (arg);
}

static char default_name[]="JUAE thread";

int uae_start_thread (const TCHAR *name, void *(*f)(void *), void *arg, uae_thread_id *tid)
{
    struct MsgPort *replyport;
    struct Process *newtask = NULL;
    struct TagItem procTags[] =
    {
        { NP_Name,	   (IPTR) NULL},
        { NP_StackSize,	   (IPTR)(128 * 1024)}, /* this is just a guess. hardfile thread needs more than 64k */
        { NP_Entry,	   (IPTR) do_thread},
        //{ NP_Output,		   Output ()},
        //{ NP_Input,		   Input ()},
        //{ NP_CloseOutput,	   FALSE},
        //{ NP_CloseInput,	   FALSE},
        { TAG_DONE, 0}
    };

// warning Do we need to care for priorities here? WinUAE does..
    if(!name)
        procTags[0].ti_Data = (IPTR)default_name;
    else
        procTags[0].ti_Data = (IPTR)name;

    bug("[JUAE:PX] %s('%s', f %lx, arg %lx, tid %lx)\n", __PRETTY_FUNCTION__, procTags[0].ti_Data, f, arg, tid);

    replyport = CreateMsgPort();
    if(!replyport) {
        write_log("ERROR: Unable to create MsgPort!\n");
    }

    newtask = CreateNewProc (procTags);

    bug("[JUAE:PX] %s: Process @ %p, MsgPort @ %p\n", __PRETTY_FUNCTION__, newtask, newtask->pr_MsgPort);

    if(newtask) {
        struct startupmsg msg;

        msg.msg.mn_ReplyPort = replyport;
        msg.msg.mn_Length    = sizeof msg;
        msg.func             = f;
        msg.arg              = arg;
        PutMsg (&newtask->pr_MsgPort, (struct Message*)&msg);
        WaitPort (replyport);
    }
    DeleteMsgPort (replyport);

    if(tid) {
        *tid=newtask;
    }

    return (newtask != 0);
}

int uae_start_thread_fast (void *(*f)(void *), void *arg, uae_thread_id *tid) {
    bug("[JUAE:PX] %s(%lx, %lx, %lx)\n", __PRETTY_FUNCTION__, f, arg, tid);

    return uae_start_thread(NULL, f, arg, tid);
}
