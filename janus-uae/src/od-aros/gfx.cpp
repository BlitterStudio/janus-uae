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

#include <proto/intuition.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "gfx.h"


struct winuae_currentmode {
	unsigned int flags;
	int native_width, native_height, native_depth, pitch;
	int current_width, current_height, current_depth;
	int amiga_width, amiga_height;
	int frequency;
	int initdone;
	int fullfill;
	int vsync;
};

static struct winuae_currentmode currentmodestruct;
static struct winuae_currentmode *currentmode = &currentmodestruct;

struct Window *hAmigaWnd;

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

static int create_windows_2 (void) {
	/* window borders */
	int cy_top_border    = 10; /* TODO: get correct values!? */
	int cy_bottom_border = 10;
	int cx_left_border   = 10;
	int cx_right_border  = 10;

	if(hAmigaWnd) {
		/* we already have a window.. TODO */
		TODO();
		return 1;
	}

	currentmode->amiga_width=600;
	currentmode->amiga_height=400;

	hAmigaWnd= OpenWindowTags (NULL,
			  WA_Title,        (ULONG)PACKAGE_NAME,
			  WA_AutoAdjust,   TRUE,
			  WA_InnerWidth,   currentmode->amiga_width,
			  WA_InnerHeight,  currentmode->amiga_height,
			  //WA_PubScreen,    (ULONG)S,
			  //WA_Zoom,         (ULONG)ZoomArray,
			  WA_IDCMP,        IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY
					 | IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW
					 | IDCMP_MOUSEMOVE    | IDCMP_DELTAMOVE
					 | IDCMP_CLOSEWINDOW  | IDCMP_REFRESHWINDOW
					 | IDCMP_NEWSIZE      | IDCMP_INTUITICKS,
			  WA_Flags,	   WFLG_DRAGBAR       | WFLG_DEPTHGADGET
					 | WFLG_REPORTMOUSE   | WFLG_RMBTRAP
					 | WFLG_ACTIVATE      | WFLG_CLOSEGADGET
					 | WFLG_SMART_REFRESH,
			  TAG_DONE);

	DebOut("hAmigaWnd: %lx\n", hAmigaWnd);

	if(!hAmigaWnd) {
		return 0;
	}

	return 1;
}


static int open_windows (int full) {
	return create_windows_2();
}

int graphics_init(void) {

	DebOut("entered\n");

	gfxmode_reset ();

	DebOut("HERE WE ARE.. LOTS OF WORK TODO.......................\n");

	return open_windows (1);
}

