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

#include <proto/utility.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"


char *uutf8 (const WCHAR *s) {

	//DebOut("s=%s\n", s);

	if(s==NULL) {
		return NULL;
	}
	
	return strdup(s);
}

WCHAR *au_copy (TCHAR *dst, int maxlen, const char *src) {
	//DebOut("src=%s\n", src);

	return strncpy(dst, src, maxlen);
}

char *ua_copy (char *dst, int maxlen, const TCHAR *src) {

  dst[0] = 0;
	return strncpy(dst, src, maxlen);
}

char *au_fs_copy (char *dst, int maxlen, const char *src)
{
	int i;

  //DebOut("src: %s, len %d\n", src, maxlen);

	for (i = 0; src[i] && i < maxlen - 1; i++)
		dst[i] = src[i];
	dst[i] = 0;
	return dst;
}

/* we only have ASCII bytes, so.. */
char *ua_fs_copy (char *dst, int maxlen, const TCHAR *src, int defchar) {
  strncpy(dst, src, maxlen);
}

/* ua_fs
 *  filter unusual characters (?)
 */
char *ua_fs (const WCHAR *s, int defchar) {
  char *d;
  int len, i;
  BOOL dc;
  char def = 0;

  //DebOut("s: %s, defchar %d\n", s, defchar);

  if (s == NULL) {
    return NULL;
  }

  dc=FALSE;
  len=strlen(s);
  d=strdup(s);

  /* WinUAE uses >= 0 !? */
  if(defchar > 0) {
    for (i = 0; i < len; i++)  {
      if (d[i] == 0 || (d[i] < 32 || (d[i] >= 0x7f && d[i] <= 0x9f))) {
        d[i]=(char) defchar;
      }
    }
  }

  //DebOut("return: %s\n", d);
  return d;
}

WCHAR *utf8u (const char *s) {

	//DebOut("s=%s\n", s);

	if(s==NULL) {
		return NULL;
	}
	
	return strdup(s);
}

int same_aname (const TCHAR *an1, const TCHAR *an2) {

  if(Stricmp(an1, an2)) {
    return 0;
  }

  return 1;
}
