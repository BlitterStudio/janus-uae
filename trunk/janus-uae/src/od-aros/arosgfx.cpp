/************************************************************************ 
 *
 * AROS Drawing infterface
 *
 * Copyright 2011 Oliver Brunner <aros<at>oliver-brunner.de>
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


#include <stdlib.h>
#include <stdarg.h>


#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"

#include "aros.h"

struct uae_filter *usedfilter;
int default_freq = 0;

static int isfullscreen_2 (struct uae_prefs *p)
{
	int idx = screen_is_picasso ? 1 : 0;

	return p->gfx_apmode[idx].gfx_fullscreen == GFX_FULLSCREEN ? 1 : (p->gfx_apmode[idx].gfx_fullscreen == GFX_FULLWINDOW ? -1 : 0);
}

int isfullscreen (void)
{
	return isfullscreen_2 (&currprefs);
}


static int flushymin, flushymax;
#define FLUSH_DIFF 50

static void flushit (int lineno) {

  TODO();
#if 0
	if (!currprefs.gfx_api)
		return;
	if (currentmode->flags & DM_SWSCALE)
		return;
	if (flushymin > lineno) {
		if (flushymin - lineno > FLUSH_DIFF && flushymax != 0) {
			D3D_flushtexture (flushymin, flushymax);
			flushymin = currentmode->amiga_height;
			flushymax = 0;
		} else {
			flushymin = lineno;
		}
	}
	if (flushymax < lineno) {
		if (lineno - flushymax > FLUSH_DIFF && flushymax != 0) {
			D3D_flushtexture (flushymin, flushymax);
			flushymin = currentmode->amiga_height;
			flushymax = 0;
		} else {
			flushymax = lineno;
		}
	}
#endif
}

void flush_line (struct vidbuffer *vb, int lineno) {
  TODO();

	//flushit (lineno);
}

#if 0
void flush_block (int first, int last) {

	//flushit (first);
	//flushit (last);
}
#endif

void flush_screen (struct vidbuffer *vb, int a, int b) {

  TODO();
#if 0
	if (dx_islost ())
		return;
	flushymin = 0;
	flushymax = currentmode->amiga_height;
	if (currentmode->flags & DM_D3D) {
		D3D_flip ();
#ifdef GFXFILTER
	} else if (currentmode->flags & DM_SWSCALE) {
		S2X_render ();
		DirectDraw_Flip (1);
#endif
	} else if (currentmode->flags & DM_DDRAW) {
		DirectDraw_Flip (1);
	}
#endif
}

