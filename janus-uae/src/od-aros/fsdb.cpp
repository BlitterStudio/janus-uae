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
