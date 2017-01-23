/************************************************************************ 
 *
 * fsusage.c - return space usage of mounted filesystem
 *
 * Copyright 2017 Oliver Brunner <aros<at>oliver-brunner.de>
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

//#define JUAE_DEBUG

#include "sysconfig.h"
#include "sysdeps.h"

#include "fsusage.h"

#include <proto/dos.h>

/*
 * we need to fill in:
 *   unsigned long fsu_blocks;   Total blocks. 
 *   unsigned long fsu_bfree;    Free blocks available to superuser. 
 *   unsigned long fsu_bavail;   Free blocks available to non-superuser. 
 * never used anywhere:
 *   unsigned long fsu_files;    Total file nodes. 
 *   unsigned long fsu_ffree;    Free file nodes. 
 *
 * Blocks are 512-byte, WinUAE fsdb expects this!
 */
int get_fs_usage (const TCHAR *path, const TCHAR *disk, struct fs_usage *fsp) {

  struct InfoData infodata;
  BPTR lock;
  int ret=1;

  DebOut("path: %s, disk: %s\n", path, disk);

  lock=Lock(path, SHARED_LOCK);
  if(lock) {
    if(Info(lock, &infodata)) {
      DebOut("infodata.id_NumBlocks:     %d\n", infodata.id_NumBlocks);
      DebOut("infodata.id_NumBlocksUsed: %d\n", infodata.id_NumBlocksUsed);
      fsp->fsu_blocks = infodata.id_NumBlocks;
      fsp->fsu_bfree  = infodata.id_NumBlocks - infodata.id_NumBlocksUsed;

      DebOut("BytesPerBlock: %d\n", infodata.id_BytesPerBlock);
      if(infodata.id_BytesPerBlock != 512) {
        /* adjust to different blocksize */
        fsp->fsu_blocks = fsp->fsu_blocks * infodata.id_BytesPerBlock / 512;
        fsp->fsu_bfree  = fsp->fsu_bfree  * infodata.id_BytesPerBlock / 512;
      }
      /* no difference here */
      fsp->fsu_bavail = fsp->fsu_bfree;

      ret=0;
    }
    UnLock(lock);
  }

	return ret;
}

