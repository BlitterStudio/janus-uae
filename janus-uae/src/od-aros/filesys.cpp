/*
 * In WinUAE
 * this file is *included* from ../filesys.cpp
 *
 */

#include <stdio.h>
#include <proto/dos.h>

#define JUAE_DEBUG 

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"

int add_filesys_unit (struct uaedev_config_info *ci);

void filesys_addexternals (void) {

  struct DosList *dl;
  BPTR lock;
  STRPTR  taskname = NULL;  

  if (!currprefs.win32_automount_cddrives && !currprefs.win32_automount_netdrives
    && !currprefs.win32_automount_drives && !currprefs.win32_automount_removabledrives) {

    return;
  }

  dl = LockDosList(LDF_READ|LDF_VOLUMES);

  if(!dl) {
    DebOut("ERROR: LockDosList failed!\n");
    return;
  }

  struct DosList *tdl=dl;
  DebOut("doslist: %lx\n", dl);

  while ((tdl = NextDosEntry(tdl, LDF_VOLUMES))) {

    struct uaedev_config_info ci = { 0 };

    DebOut("Volume: %s\n", tdl->dol_Name);
    lock=Lock(tdl->dol_Name, SHARED_LOCK);
    DebOut("  lock: %lx\n", lock);
    if(!lock) continue;
    UnLock(lock);

    sprintf (ci.volname, "host_%s", tdl->dol_Name);
    if(tdl->dol_Type == DLT_VOLUME) {
      taskname=((struct Task *)tdl->dol_Task->mp_SigTask)->tc_Node.ln_Name;
      DebOut("  taskname: %s\n", taskname);
      sprintf (ci.rootdir, "%s:", taskname);
    }
    DebOut("  ci.volname: %s, ci.rootdir: %s\n", ci.volname, ci.rootdir);
    ci.readonly = 1;
    ci.bootpri = -20;
    add_filesys_unit (&ci);
  }

  UnLockDosList(LDF_READ|LDF_VOLUMES);

}
