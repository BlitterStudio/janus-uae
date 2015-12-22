/************************************************************************ 
 *
 * GUIO helper
 *
 * Copyright 2011 Oliver Brunner <aros<at>oliver-brunner.de>
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
#include <stdlib.h>

#include <proto/dos.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "registry.h"

bool scan_rom_hook (const TCHAR *name, int line);
int scan_rom (const TCHAR *path, UAEREG *fkey, bool deepscan);

int scan_roms_2 (UAEREG *fkey, const TCHAR *path, bool deepscan)
{
	TCHAR buf[MAX_DPATH];
	int ret=0;
  struct FileLock *lock;
  struct FileInfoBlock *fib_ptr;
  TCHAR tmppath[MAX_DPATH];
#if 0
	WIN32_FIND_DATA find_data;
	HANDLE handle;
#endif

    bug("[JUAE:GUI] %s()\n", __PRETTY_FUNCTION__);

  fib_ptr=(struct FileInfoBlock *) AllocMem(sizeof(struct FileInfoBlock), MEMF_PUBLIC | MEMF_CLEAR);
  if(!fib_ptr) {
    write_log (_T("not enough memory for FileInfoBlock!?\n"));
    exit(1);
  }

  lock=(struct FileLock *) Lock(path , SHARED_LOCK );

  if(!lock) {
    bug("unable to lock %s\n", path);
    goto SDONE;
  }

  if(!Examine( (BPTR) lock, fib_ptr)) {
    bug("Examine failed!\n");
    goto SDONE;
  }

  if(fib_ptr->fib_DirEntryType==0) {
    bug("no directory!!\n");
    goto SDONE;
  }

  bug("ini: %s, path: %s, deepscan: %d\n", fkey->inipath, path, deepscan);

	if (!path)
		return 0;

	write_log (_T("ROM scan directory '%s'\n"), path);
	bug (_T("ROM scan directory '%s'\n"), path);
#if 0
	_tcscpy (buf, path);
	_tcscat (buf, _T("*.*"));
	ret = 0;
	handle = FindFirstFile (buf, &find_data);
	if (handle == INVALID_HANDLE_VALUE)
		return 0;
#endif
	scan_rom_hook (path, 1);

#if 0
	for (;;) {
    bug("..\n");
#endif

    while(ExNext((BPTR) lock, fib_ptr)) {
      _tcscpy (tmppath, path);
      AddPart(tmppath, (CONST_STRPTR) fib_ptr->fib_FileName, MAX_DPATH);
      bug("tmppath: %s\n", tmppath);
      if(fib_ptr->fib_DirEntryType < 0) {
        /* is a file */
        bug("size: %d\n", fib_ptr->fib_Size);
        if(fib_ptr->fib_Size < 10000000) {
          if (scan_rom (tmppath, fkey, deepscan)) {
            ret = 1;
          }
        }
      }
      else {
        bug("no file: %s\n", tmppath);
      }

      //_tcscat (tmppath, fib_ptr->fib_FileName);
    }

#if 0


		_tcscpy (tmppath, path);
		_tcscat (tmppath, find_data.cFileName);
		if (!(find_data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |FILE_ATTRIBUTE_SYSTEM)) && find_data.nFileSizeLow < 10000000) {
			if (scan_rom (tmppath, fkey, deepscan))
				ret = 1;
		}
		if (!scan_rom_hook (NULL, 0) || FindNextFile (handle, &find_data) == 0) {
			FindClose (handle);
			break;
		}
	}
#endif

  bug("done scanning roms (ret: %d)\n", ret);
  bug("----------------------------\n");

SDONE:
  if(lock) {
    UnLock((BPTR) lock);
    lock=NULL;
  }
  if(fib_ptr) {
    FreeMem(fib_ptr, sizeof(struct FileInfoBlock));
    fib_ptr=NULL;
  }

	return ret;
}

