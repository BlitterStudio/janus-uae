/************************************************************************ 
 *
 * dummy unicode functions
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


#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"


char *uutf8 (const WCHAR *s) {

	DebOut("s=%s\n", s);

	if(s==NULL) {
		return NULL;
	}
	
	return strdup(s);
}

WCHAR *au_copy (TCHAR *dst, int maxlen, const char *src) {
	DebOut("src=%s\n", src);

	return strncpy(dst, src, maxlen);
}

WCHAR *utf8u (const char *s) {

	DebOut("s=%s\n", s);

	if(s==NULL) {
		return NULL;
	}
	
	return strdup(s);
}
