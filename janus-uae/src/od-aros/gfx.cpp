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



#include "options.h"
#include "audio.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "traps.h"
#include "xwin.h"
#include "keyboard.h"
#include "drawing.h"
#include "gfx.h"
#include "picasso96_aros.h"
#include "inputdevice.h"
#include "gui.h"
#include "serial.h"
#include "gfxfilter.h"
#include "sampler.h"
#ifdef RETROPLATFORM
#include "rp.h"
#endif

#define AMIGA_WIDTH_MAX (752 / 2)
#define AMIGA_HEIGHT_MAX (574 / 2)

#define DM_DX_FULLSCREEN 1
#define DM_W_FULLSCREEN 2
#define DM_D3D_FULLSCREEN 16
#define DM_PICASSO96 32
#define DM_DDRAW 64
#define DM_DC 128
#define DM_D3D 256
#define DM_SWSCALE 1024

#define SM_WINDOW 0
#define SM_FULLSCREEN_DX 2
#define SM_OPENGL_WINDOW 3
#define SM_OPENGL_FULLWINDOW 9
#define SM_OPENGL_FULLSCREEN_DX 4
#define SM_D3D_WINDOW 5
#define SM_D3D_FULLWINDOW 10
#define SM_D3D_FULLSCREEN_DX 6
#define SM_FULLWINDOW 7
#define SM_NONE 11



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
int screen_is_picasso = 0;
static int scalepicasso;

static uae_u8 scrlinebuf[4096 * 4]; /* this is too large, but let's rather play on the safe side here */

static BOOL doInit (void);

int graphics_setup (void) {

	/* TODO(); */

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

	currentmode->amiga_width=currentmode->current_width;
	currentmode->amiga_height=currentmode->current_height;

	DebOut("opening window with %dx%d\n", currentmode->current_width, currentmode->current_height);

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

static void close_windows (void) {

	DebOut("entered\n");

	if(hAmigaWnd) {
		CloseWindow(hAmigaWnd);
		hAmigaWnd=NULL;
	}

}

static int create_windows (void) {
	if (!create_windows_2 ()) {
		return 0;
	}

	/* call set_ddraw here ..*/

	return 1;
}


static BOOL doInit (void) {
	BOOL ret;
	int size;

	/* LOT's of stuff to do here! */

	DebOut("LOT's of stuff to do here..\n");

//OOPS:
	if(!create_windows ()) {
		DebOut("unable to open windows!\n");
		goto OOPS;
	}

	for(;;) {
		DebOut("for(;;)..\n");
		if(screen_is_picasso) {
			break;
		}
		else {
			currentmode->amiga_width = currentmode->current_width;
			currentmode->amiga_height = currentmode->current_height;

			DebOut("currentmode->amiga_width: %d\n", currentmode->amiga_width);
			DebOut("currentmode->amiga_height: %d\n", currentmode->amiga_height);

			gfxvidinfo.pixbytes = currentmode->current_depth >> 3;
			size = currentmode->amiga_width * currentmode->amiga_height * gfxvidinfo.pixbytes;
			gfxvidinfo.realbufmem = xmalloc (uae_u8, size);;
			gfxvidinfo.bufmem = gfxvidinfo.realbufmem;
			gfxvidinfo.rowbytes = currentmode->amiga_width * gfxvidinfo.pixbytes;

			gfxvidinfo.linemem = 0;
			gfxvidinfo.width = (currentmode->amiga_width + 7) & ~7;
			gfxvidinfo.height = currentmode->amiga_height;
			gfxvidinfo.maxblocklines = 0; // flush_screen actually does everything
			//gfxvidinfo.rowbytes = currentmode->pitch;
			break;
		}
	} /* for */

	gfxvidinfo.emergmem = scrlinebuf; // memcpy from system-memory to video-memory

	return 1;

OOPS:
	close_windows();
	return ret;
}

static void update_modes(void) {
	WORD flags;

	DebOut("entered\n");

	/* HACK: */
	if(!currentmode->current_width) {
		DebOut("hardsetting currentmode->current_width to currprefs.gfx_size.width..\n");
		currentmode->current_width =currprefs.gfx_size.width;
		currentmode->current_height=currprefs.gfx_size.height;
	}
#if 0
	if (flags & DM_W_FULLSCREEN) {
		RECT rc = getdisplay (&currprefs)->rect;
		currentmode->native_width = rc.right - rc.left;
		currentmode->native_height = rc.bottom - rc.top;
		currentmode->current_width = currentmode->native_width;
		currentmode->current_height = currentmode->native_height;
	} else {
#endif
		currentmode->native_width = currentmode->current_width;
		currentmode->native_height = currentmode->current_height;
		DebOut("currentmode->current_width: %d\n", currentmode->current_width);
		DebOut("currentmode->native_width: %d\n", currentmode->native_width);
#if 0
	}
#endif
}


static void update_gfxparams (void) {

	DebOut("entered\n");

	//updatewinfsmode (&currprefs);
#ifdef PICASSO96
	currentmode->vsync = 0;
	if (screen_is_picasso) {
		currentmode->current_width = picasso96_state.Width;
		currentmode->current_height = picasso96_state.Height;
		currentmode->frequency = abs (currprefs.gfx_refreshrate > default_freq ? currprefs.gfx_refreshrate : default_freq);
		if (currprefs.gfx_pvsync)
			currentmode->vsync = 1;
	} else {
#endif
		currentmode->current_width = currprefs.gfx_size.width;
		DebOut("currprefs.gfx_size.width: %d\n", currprefs.gfx_size.width);
		DebOut("currprefs.gfx_size.width_windowed: %d\n", currprefs.gfx_size_win.width);
		currentmode->current_height = currprefs.gfx_size.height;
		currentmode->frequency = abs (currprefs.gfx_refreshrate);
		if (currprefs.gfx_avsync)
			currentmode->vsync = 1;
#ifdef PICASSO96
	}
#endif
	currentmode->current_depth = currprefs.color_mode < 5 ? 16 : 32;
	if (screen_is_picasso && currprefs.win32_rtgmatchdepth && isfullscreen () > 0) {
		int pbits = picasso96_state.BytesPerPixel * 8;
		if (pbits <= 8) {
			if (currentmode->current_depth == 32)
				pbits = 32;
			else
				pbits = 16;
		}
		if (pbits == 24)
			pbits = 32;
		currentmode->current_depth = pbits;
	}
	currentmode->amiga_width = currentmode->current_width;
	currentmode->amiga_height = currentmode->current_height;

	scalepicasso = 0;
	if (screen_is_picasso) {
		if (isfullscreen () < 0) {
			if ((currprefs.win32_rtgscaleifsmall || currprefs.win32_rtgallowscaling) && (picasso96_state.Width != currentmode->native_width || picasso96_state.Height != currentmode->native_height))
				scalepicasso = 1;
			if (!scalepicasso && currprefs.win32_rtgscaleaspectratio)
				scalepicasso = -1;
		} else if (isfullscreen () > 0) {
			if (!currprefs.win32_rtgmatchdepth) { // can't scale to different color depth
				if (currentmode->native_width > picasso96_state.Width && currentmode->native_height > picasso96_state.Height) {
					if (currprefs.win32_rtgscaleifsmall)
						scalepicasso = 1;
				}
				if (!scalepicasso && currprefs.win32_rtgscaleaspectratio)
					scalepicasso = -1;
			}
		} else if (isfullscreen () == 0) {
			if ((currprefs.gfx_size.width != picasso96_state.Width || currprefs.gfx_size.height != picasso96_state.Height) && currprefs.win32_rtgallowscaling)
				scalepicasso = 1;
			if ((currprefs.gfx_size.width > picasso96_state.Width || currprefs.gfx_size.height > picasso96_state.Height) && currprefs.win32_rtgscaleifsmall)
				scalepicasso = 1;
			if (!scalepicasso && currprefs.win32_rtgscaleaspectratio)
				scalepicasso = -1;
		}

		if (scalepicasso > 0 && (currprefs.gfx_size.width != picasso96_state.Width || currprefs.gfx_size.height != picasso96_state.Height)) {
			currentmode->current_width = currprefs.gfx_size.width;
			currentmode->current_height = currprefs.gfx_size.height;
		}
	}

}

static int init_round;

static int open_windows (int full) {

	int ret;

	inputdevice_unacquire ();
	//reset_sound ();

	init_round = 0; /* ? */
	ret = -2;
	do {
		DebOut("ret: %d, init_round: %d\n", ret, init_round);
		if(ret < -1) {
			update_modes ();
			update_gfxparams ();
		}
		ret = doInit ();
		init_round++;
	}
	while (ret < 0);

	if(!ret) {
		DebOut("open_windows failed: ret==0\n");
		return ret;
	}

	inputdevice_acquire (TRUE);

	//return create_windows_2();
	return ret;
}

int graphics_init(void) {

	DebOut("entered\n");

	gfxmode_reset ();

	/* in WinUAE a lot of stuff is configured by the GUI (read from PREFS->written to workin prefs etc.
	 * we do this here for the moment:
	 */
	currprefs.gfx_size.width=currprefs.gfx_size_win.width;
	currprefs.gfx_size.height=currprefs.gfx_size_win.height;

	DebOut("HERE WE ARE.. LOTS OF WORK TODO.......................\n");

	return open_windows (1);
}

int isscreen (void)
{
	return hAmigaWnd ? 1 : 0;
}

/* not sure, what this does..
 * winuae uses it to D3D_locktexture/DirectDraw_SurfaceLock stuff.
 * I doubt, we need this on AROS !?
 */
int lockscr (int fullupdate) {
	DebOut("fullupdate: %d\n", fullupdate);

	if(!isscreen()) {
		DebOut("no screen\n");
		return 0;
	}
	return 1;
}
