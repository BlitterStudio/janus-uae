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
#include <proto/utility.h>
#include "j.h"
#include "memory.h"
#include "include/filesys.h"

struct JUAE_Launch_Message {
  struct Message ExecMessage;
  STRPTR ln_Name;
};

/* from filesys.c */
#define DEVNAMES_PER_HDF 32
typedef struct {
    char *devname; /* device name, e.g. UAE0: */
    uaecptr devname_amiga;
    uaecptr startup;
    char *volname; /* volume name, e.g. CDROM, WORK, etc. */
    char *rootdir; /* root unix directory */
    int readonly; /* disallow write access? */
    int bootpri; /* boot priority */
    int devno;

    struct hardfiledata hf;

    /* Threading stuff */
    smp_comm_pipe *volatile unit_pipe, *volatile back_pipe;
    uae_thread_id tid;
    struct _unit *self;
    /* Reset handling */
    volatile uae_sem_t reset_sync_sem;
    volatile int reset_state;

    /* RDB stuff */
    uaecptr rdb_devname_amiga[DEVNAMES_PER_HDF];
    int rdb_lowcyl;
    int rdb_highcyl;
    int rdb_cylblocks;
    uae_u8 *rdb_filesysstore;
    int rdb_filesyssize;
    char *filesysdir;

} UnitInfo;


struct uaedev_mount_info {
    int num_units;
    UnitInfo ui[MAX_FILESYSTEM_UNITS];
};

/***********************************************************
 * check_and_convert_path
 *
 * We check, if aros_exe_full_path is within
 * - our aros_device_mounted and
 * - our aros_path_mounted.
 *
 * If this is the case, substitute 
 * "aros_device_mounted:aros_path_mounted" with
 * "aos_mount_point" and return the result.
 *
 * The caller has to FreeVec the result!
 ***********************************************************/
char *check_and_convert_path(char *aros_exe_full_path, 
                             char *aos_mount_point, 
			     char *aros_device_mounted, 
			     char *aros_path_mounted) {

  char *aros_exe_path;
  char *aos_file;
  char *result;

  JWLOG("entered(%s, %s, %s, %s)\n", aros_exe_full_path, aos_mount_point, aros_device_mounted, aros_path_mounted);
  
  if(Strnicmp(aros_exe_full_path, aros_device_mounted, strlen(aros_device_mounted))) {
    JWLOG("Strnicmp(%s, %s) failed!\n", aros_exe_full_path, aros_device_mounted);
    return NULL;  /* aros_exe_full_path is not within this mounted path */
  }

  if(aros_exe_full_path[strlen(aros_device_mounted)] != ':') {
    JWLOG("no : at %d: >%c<\n", strlen(aros_device_mounted), aros_exe_full_path[strlen(aros_device_mounted)]);
    return NULL;
  }

  /* path without device name */
  aros_exe_path=aros_exe_full_path + strlen(aros_device_mounted) + 1;
  JWLOG("aros_exe_path: >%s<\n", aros_exe_path);

  if(Strnicmp(aros_exe_path, aros_path_mounted, strlen(aros_path_mounted))) {
    JWLOG("Strnicmp 2(%s, %s) failed!\n", aros_exe_path, aros_path_mounted);
    return NULL;
  }

  if(aros_exe_path[strlen(aros_path_mounted)] != '/') {
    JWLOG("no / at %d: >%c<\n", strlen(aros_path_mounted), aros_exe_path[strlen(aros_path_mounted)]);
    return NULL;
  }

  aos_file=aros_exe_path + strlen(aros_path_mounted) + 1;
  JWLOG("aos_file: >%s<\n", aos_file);

  JWLOG("result: >%s:%s<\n", aos_mount_point, aos_file);

  result=(char *) AllocVec(strlen(aos_mount_point)+strlen(aos_file)+2, MEMF_ANY);
  sprintf(result, "%s:%s", aos_mount_point, aos_file);

  return result;
}

extern struct uaedev_mount_info current_mountinfo;

/***********************************************************
 * aros_path_to_amigaos(aros_path)
 *
 * tries to map the aros file with full path to an 
 * amigaos native file with full path.
 *
 * return NULL, if the aros_path is not mounted at
 * any point in amigaos.
 ***********************************************************/
static char *aros_path_to_amigaos(char *aros_path) {

  char           *result;
  UnitInfo *uip = options_mountinfo.ui;
  LONG      i;
  char     *str;
  char     *aos_path;

  JWLOG("entered (%s)\n", aros_path);

  aos_path=NULL;
  for(i=0; (i < options_mountinfo.num_units) && (aos_path==NULL); i++) {
    str = cfgfile_subst_path (prefs_get_attr ("hardfile_path"), "$(FILE_PATH)", uip[i].rootdir);
    JWLOG("uip[%d].devname: %s\n", i, uip[i].devname);
    JWLOG("uip[%d].volname: %s\n", i, uip[i].volname);
    JWLOG("uip[%d].str: %s\n", i, str);
    aos_path=check_and_convert_path(aros_path, uip[i].devname, uip[i].volname, str);
    JWLOG("RESULT: %s\n", aos_path);
    xfree(str);
  }

  return NULL;
}

/***********************************************************
 * This is the process, which watches the Execute
 * Port of J-UAE, so wanderer can send us messages.
 ***********************************************************/  
static void aros_launch_thread (void) {

  struct Process *thread = (struct Process *) FindTask (NULL);
  BOOL            done   = FALSE;
  ULONG           s;
  struct MsgPort *port   = NULL;
  struct JUAE_Launch_Message *msg;

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

    WaitPort(port);
    JWLOG("aros_launch_thread[%lx]: WaitPort done\n",thread);
    while( (msg = (struct JUAE_Launch_Message *) GetMsg(port)) ) {
      JWLOG("msg %lx received!\n");
      JWLOG("msg->ln_Name: >%s< \n", msg->ln_Name);
      aros_path_to_amigaos(msg->ln_Name);
      //fsdb_search_dir(I//
      /* we need to free it ourselves */
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

