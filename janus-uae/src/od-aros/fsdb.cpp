/************************************************************************ 
 *
 * AROS specific file system functions
 *
 * Copyright 2011       Oliver Brunner - aros<at>oliver-brunner.de
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
#include <proto/dos.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"

int my_existsfile (const TCHAR *name) {

	struct FileInfoBlock fib;
	BPTR lock;
	int res;

	DebOut("name=%s\n", name);

	lock=Lock((CONST_STRPTR) name, SHARED_LOCK);
	if(!lock) {
		DebOut("could not lock %s\n", name);
		return 0;
	}

	res=0;
	if(Examine(lock, &fib)) {
		if(fib.fib_DirEntryType < 0) {
			res=1;
		}
	}

	UnLock(lock);

	DebOut("my_existsfile(%s)=%d\n", name, res);
	return res;
}

/* should open a textfile according to its encoding. (UTF-8, UTF-16LE or ascii)
 * on AROS we do not support UTF
 */
FILE *my_opentext (const TCHAR *name) {

	FILE *f;
	uae_u8 tmp[5];
	int v;

	DebOut("open %s\n", name);

	f = fopen (name, "rb");
	if (!f) {
		DebOut("unable to open %s\n", name);
		return NULL;
	}
	v = fread (tmp, 1, 4, f);
	fclose (f);
	if (v == 4) {
		if (tmp[0] == 0xef && tmp[1] == 0xbb && tmp[2] == 0xbf) {
			/* return fopen (name, L"r, ccs=UTF-8"); */
			DebOut("ERROR: tried to open UTF-8 file!\n");
			fprintf(stderr, "ERROR: tried to open UTF-8 file %s!\n", name);
			return NULL;
		}
		if (tmp[0] == 0xff && tmp[1] == 0xfe) {
			/* return fopen (name, L"r, ccs=UTF-16LE"); */
			DebOut("ERROR: tried to open UTF-16 file!\n");
			fprintf(stderr, "ERROR: tried to open UTF-16LE file %s!\n", name);
			return NULL;
		}

	}
	return fopen (name, "r");
}


int fsdb_exists (const TCHAR *nname) {
  BPTR lock;

  lock=Lock(nname, SHARED_LOCK);
  if(lock) {
    UnLock(lock);
    return TRUE;
  }

  return FALSE;
}

