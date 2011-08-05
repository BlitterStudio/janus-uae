/************************************************************************ 
 *
 * AROS Drawing
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


int graphics_setup (void) {

	TODO();

#ifdef PICASSO96
	//InitPicasso96 ();
#endif

	/* j-uae calls
	init_pointer ();
	initpseudodevices ();
	 * here */
	return 1;
}

static void gfxmode_reset (void) {
	TODO();
}

int graphics_init(void) {

	DebOut("entered\n");

	gfxmode_reset ();

	DebOut("HERE WE ARE.. LOTS OF WORK TODO.......................\n");

	return open_windows (1);
}

