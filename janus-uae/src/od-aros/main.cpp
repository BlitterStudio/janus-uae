/************************************************************************ 
 *
 * main
 *
 * Copyright 2011 Oliver Brunner - aros<at>oliver-brunner.de
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

#include "uae.h"
#include "options.h"
#include "aros.h"

TCHAR VersionStr[256];
TCHAR BetaStr[64];


void makeverstr (TCHAR *s)
{
	if (_tcslen (WINUAEBETA) > 0) {
		_stprintf (BetaStr, " (%sBeta %s, %d.%02d.%02d)", WINUAEPUBLICBETA > 0 ? "Public " : "", WINUAEBETA,
			GETBDY(WINUAEDATE), GETBDM(WINUAEDATE), GETBDD(WINUAEDATE));
		_stprintf (s, "janus-UAE %d.%d.%d%s%s",
			UAEMAJOR, UAEMINOR, UAESUBREV, WINUAEREV, BetaStr);
	} else {
		_stprintf (s, "janus-UAE %d.%d.%d%s (%d.%02d.%02d)",
			UAEMAJOR, UAEMINOR, UAESUBREV, WINUAEREV, GETBDY(WINUAEDATE), GETBDM(WINUAEDATE), GETBDD(WINUAEDATE));
	}
	if (_tcslen (WINUAEEXTRA) > 0) {
		_tcscat (s, " ");
		_tcscat (s, WINUAEEXTRA);
	}
}

int main (int argc, TCHAR **argv) {

	//getstartpaths ();

	makeverstr(VersionStr);

	real_main (argc, argv);
	return 0;
}

