/************************************************************************ 
 *
 * nogui.cpp
 *
 * No User Interface for the AROS version
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


/*
 * do nothing without GUI
 * TODO: if we have a GUI 
 */
int machdep_init (void) {
	return 1;
}


void notify_user (int msg)
{
#if 0
	TCHAR tmp[MAX_DPATH];
	int c = 0;

	c = gettranslation (msg);
	if (c < 0)
		return;
	WIN32GUI_LoadUIString (c, tmp, MAX_DPATH);
	gui_message (tmp);
#endif
  DebOut("===============> notify_user: %d ==============\n", msg);
  write_log("==> notify_user: %d\n", msg);
  TODO();
}

