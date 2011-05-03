/************************************************************************ 
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

#include <stdio.h>

#define WRITE_LOG_BUF_SIZE 4096
void write_log (const char *format, ...)
{

#warning write_log is TODO
#if 0
	int count;
	TCHAR buffer[WRITE_LOG_BUF_SIZE];
	va_list parms;
	va_start (parms, format);
	if (debug) {
		count = _vsntprintf (buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
		_tprintf (buffer);
	}
	va_end (parms);
#endif
}
