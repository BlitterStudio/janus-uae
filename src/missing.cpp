/*
* UAE - The Un*x Amiga Emulator
*
* Various stuff missing in some OSes.
*
* These are functions are missing on the build host.
*
* Copyright 1997 Bernd Schmidt
* Copyright 2011 Oliver Brunner
*
* $Id$
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "uae.h"

#ifndef HAVE_STRDUP

TCHAR *my_strdup (const TCHAR *s)
{
	TCHAR *x = (char*)xmalloc(strlen((TCHAR *)s) + 1);
	strcpy(x, (TCHAR *)s);
	return x;
}

#endif

#if 0

void *xmalloc (size_t n)
{
	void *a = malloc (n);
	return a;
}

void *xcalloc (size_t n, size_t size)
{
	void *a = calloc (n, size);
	return a;
}

void xfree (const void *p)
{

	free (p);
}

#endif


char *ua (const TCHAR *s) {
	return strdup(s);
}

TCHAR *au (const char *s) {
	return strdup(s);
}
