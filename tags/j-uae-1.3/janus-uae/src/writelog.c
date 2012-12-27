/************************************************************************ 
 *
 * Copyright 2001 Bernd Schmidt
 * Copyright 2006 Richard Drummond
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * Standard write_log that writes to the console or to a file.
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
 * ATTENTION: writelog.o is used both for the uae executeable and 
 *            to build the native tools in tools/.
 *
 * $Id$
 *
 ************************************************************************/
 
#include "sysconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "uae_string.h"
#include "uae_types.h"
#include "writelog.h"

static FILE *logfile=NULL;

/*
 * By default write-log and friends access the stderr stream.
 * This function allows you to specify a file to be used for logging
 * instead.
 *
 * Call with NULL to close a previously opened log file.
 */

void set_logfile (const char *logfile_name) {

  FILE *newfile;

  if (logfile_name && strlen (logfile_name)) {
    newfile = fopen (logfile_name, "w");

    if (newfile) {
      logfile = newfile;
    }
  } 
  else {
    if (logfile) {
      fclose (logfile);

      logfile = 0;
    }
  }
}

void write_log (const char *fmt, ...) {

  va_list ap;
  va_start (ap, fmt);
  
#ifdef HAVE_VFPRINTF
  vfprintf (logfile ? logfile : stderr, fmt, ap);
#else
  /* Technique stolen from GCC.  */
  {
    int x1, x2, x3, x4, x5, x6, x7, x8;
    x1 = va_arg (ap, int);
    x2 = va_arg (ap, int);
    x3 = va_arg (ap, int);
    x4 = va_arg (ap, int);
    x5 = va_arg (ap, int);
    x6 = va_arg (ap, int);
    x7 = va_arg (ap, int);
    x8 = va_arg (ap, int);

    if(logfile == NULL) {
      fprintf (stdout, fmt, x1, x2, x3, x4, x5, x6, x7, x8);
    }
    else {
      fprintf (logfile, fmt, x1, x2, x3, x4, x5, x6, x7, x8);
    }
  }
#endif
}

void flush_log (void) {

  fflush (logfile ? logfile : stderr);
}
