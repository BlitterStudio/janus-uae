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

/* we get a JUAE_Launch_Message from wanderer */
struct JUAE_Launch_Message {
  struct Message  ExecMessage;
  STRPTR          ln_Name;
  struct TagItem *tags;
  void           *mempool;
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
    aos_path=check_and_convert_path(aros_path, uip[i].devname, uip[i].volname, str);
    xfree(str);
  }

  JWLOG("RESULT: %s\n", aos_path);
  return aos_path;
}

/***********************************************************
 * convert_tags
 *
 * browse through the taglist and convert all paths to
 * amigaos paths, if possible.
 *
 * store resulting tags in an array.
 *
 * array and strings need to be FreeVec'ed somewhere!
 ***********************************************************/
static char **convert_tags_to_amigaos(struct TagItem *in) {
  struct TagItem *tstate;
  struct TagItem *tag;
  char           *amigaos_path;
  ULONG           nr;
  ULONG           i;
  char          **result;

  nr=0;
  tstate=in;
  while( NextTagItem(&tstate) ) {
    nr++;
  }

  if(!nr) {
    JWLOG("no arguments found\n");
    return NULL;
  }

  result=AllocVec(sizeof(char *)*(nr+1), MEMF_PUBLIC|MEMF_CLEAR);

  i=0;
  tstate=in;
  while( (tag=NextTagItem(&tstate)) ) {
    switch(tag->ti_Tag) {
      case WBOPENA_ArgName:
	JWLOG("WBOPENA_ArgName: %s\n", tag->ti_Data);
	amigaos_path=aros_path_to_amigaos(tag->ti_Data); /* AllocVec'ed */
	result[i++]=amigaos_path;
	break;

      default:
	JWLOG("WARNING: unknow tag %lx\n", tag->ti_Tag);
    }
  }

  result[i]=NULL;

  return result;
}

/* from gtkui.c */
void gui_message_with_title (const char *title, const char *format,...);

/***********************************************************
 * This is the process, which watches the Execute
 * Port of J-UAE, so wanderer can send us messages.
 ***********************************************************/  
static void aros_launch_thread (void) {

  struct Process *thread = (struct Process *) FindTask (NULL);
  BOOL            done   = FALSE;
  ULONG           s;
  struct MsgPort *port   = NULL;
  char           *amiga_exe;
  JanusLaunch    *jlaunch;
  void           *pool;
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
      amiga_exe=aros_path_to_amigaos(msg->ln_Name);

      if(amiga_exe) {
	if(aos3_launch_task) {
	  /* store it for the launchd to fetch and execute */
	  ObtainSemaphore(&sem_janus_launch_list);
	  jlaunch=(struct JanusLaunch *) AllocVec(sizeof(JanusLaunch),MEMF_CLEAR);
	  if(jlaunch) {
	    jlaunch->amiga_path=amiga_exe;
	    jlaunch->args      =convert_tags_to_amigaos(msg->tags);
	    janus_launch=g_slist_append(janus_launch,jlaunch);
	  }
  
	  ReleaseSemaphore(&sem_janus_launch_list);
	  uae_Signal(aos3_launch_task, aos3_launch_signal);
	}
	else {
	  JWLOG("ERROR: launchd is not running!\n");
	  gui_message_with_title("ERROR",
				 "Failed to start %s\n\nJ-UAE is running, but you need to start \"launchd\" inside of amigaOS!\n\nYou can find \"launchd\" in the amiga directory of your J-UAE package.\n\nBest thing is, to start it at the end of your s:user-startup.", msg->ln_Name);
	}
      }
      else {
	JWLOG("ERROR: volume %s is not mounted!\n", msg->ln_Name);
	gui_message_with_title("ERROR",
				 "Failed to start \"%s\".\n\nThis path is not available inside of amigaOS.\n\nYou need to add the AROS directory\n\n(or one of its parents) with an absolute path\n\nas an amigaOS device.\n\nBest way to do this is the \"Harddisk\" tab in the J-UAE GUI.", msg->ln_Name);
      }

      pool=msg->mempool;
      DeletePool(pool);
    }
  }


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

