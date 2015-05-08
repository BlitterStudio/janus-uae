/************************************************************************ 
 *
 * AROS Drawing interface
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

#define OLI_DEBUG
#include "sysconfig.h"

#include <stdlib.h>
#include <stdarg.h>

#include "sysdeps.h"

#include "options.h"
#include "custom.h"
#include "memory.h"
#include "gfxboard.h"

#include "gfx.h"
#include "picasso96_aros.h"
#include "aros.h"
#include "thread.h"


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

struct uae_filter *usedfilter;
int scalepicasso;

struct winuae_currentmode {
	unsigned int flags;
	int native_width, native_height, native_depth, pitch;
	int current_width, current_height, current_depth;
	int amiga_width, amiga_height;
	int initdone;
	int fullfill;
	int vsync;
	int freq;
};

struct MultiDisplay Displays[MAX_DISPLAYS+1];

static struct winuae_currentmode currentmodestruct;
static int screen_is_initialized;
static int display_change_requested;
int window_led_drives, window_led_drives_end;
int window_led_hd, window_led_hd_end;
int window_led_joys, window_led_joys_end, window_led_joy_start;
extern int console_logging;
int window_extra_width, window_extra_height;

static struct winuae_currentmode *currentmode = &currentmodestruct;
static int wasfullwindow_a, wasfullwindow_p;

static int vblankbasewait1, vblankbasewait2, vblankbasewait3, vblankbasefull, vblankbaseadjust;
static bool vblankbaselace;
static int vblankbaselace_chipset;
static bool vblankthread_oddeven, vblankthread_oddeven_got;
static int graphics_mode_changed;
int vsync_modechangetimeout = 10;

int screen_is_picasso = 0;

static bool panel_done, panel_active_done;

extern int reopen (int, bool);

#define VBLANKTH_KILL 0
#define VBLANKTH_CALIBRATE 1
#define VBLANKTH_IDLE 2
#define VBLANKTH_ACTIVE_WAIT 3
#define VBLANKTH_ACTIVE 4
#define VBLANKTH_ACTIVE_START 5
#define VBLANKTH_ACTIVE_SKIPFRAME 6
#define VBLANKTH_ACTIVE_SKIPFRAME2 7

static volatile bool vblank_found;
static volatile int flipthread_mode;
volatile bool vblank_found_chipset;
volatile bool vblank_found_rtg;
//static HANDLE flipevent, flipevent2, vblankwaitevent;
static volatile int flipevent_mode;
static CRITICAL_SECTION screen_cs;

static int dooddevenskip;

void gfx_lock (void)
{
	EnterCriticalSection (&screen_cs);
}
void gfx_unlock (void)
{
	LeaveCriticalSection (&screen_cs);
}

int vsync_busy_wait_mode;

static void vsync_sleep (bool preferbusy)
{
	struct apmode *ap = picasso_on ? &currprefs.gfx_apmode[1] : &currprefs.gfx_apmode[0];
	bool dowait;

	if (vsync_busy_wait_mode == 0) {
		dowait = ap->gfx_vflip || !preferbusy;
		//dowait = !preferbusy;
	} else if (vsync_busy_wait_mode < 0) {
		dowait = true;
	} else {
		dowait = false;
	}
	if (dowait && (currprefs.m68k_speed >= 0 || currprefs.m68k_speed_throttle < 0))
		sleep_millis_main (1);
}


void graphics_reset(void) {

	display_change_requested = 2;
}


const TCHAR *target_get_display_name (int num, bool friendlyname)
{
	if (num <= 0)
		return NULL;
#if 0
	struct MultiDisplay *md = getdisplay2 (NULL, num - 1);
	if (!md)
		return NULL;
	if (friendlyname)
		return md->monitorname;
	return md->monitorid;
#endif
  TODO();
  return "Default Monitor";
}

/* merged til here .. */
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

bool toggle_rtg (int mode) {

	if (mode == 0) {
		if (!picasso_on)
			return false;
	} else if (mode > 0) {
		if (picasso_on)
			return false;
	}
	if (currprefs.rtgmem_type >= GFXBOARD_HARDWARE) {
		return gfxboard_toggle (mode);
	} else {
		// can always switch from RTG to custom
		if (picasso_requested_on && picasso_on) {
			picasso_requested_on = false;
			return true;
		}
		if (picasso_on)
			return false;
		// can only switch from custom to RTG if there is some mode active
		if (picasso_is_active ()) {
			picasso_requested_on = true;
			return true;
		}
	}
	return false;
}

void show_screen (int mode) {
  TODO();
}

static volatile bool render_ok, wait_render;

#include <exec/execbase.h>

bool render_screen (bool immediate)
{
	bool v = false;
	int cnt;

	render_ok = false;
	if (/*minimized ||*/ picasso_on /*|| monitor_off || dx_islost ()*/)
		return render_ok;
	cnt = 0;
	while (wait_render) {
		sleep_millis (1);
		cnt++;
		if (cnt > 500)
			return render_ok;
	}

  /* This forced Permits get uae running on i386-32bit/v1 without lockups.
   * Still unclear, why this only happens on i386-32bit/v1. 
   * i386-32bit/v0 and 64bit/v1 don't have this bug..
   *
   * There are 3 Permits required to get multitasking going again.
   * 
   */

#if 0
  if(SysBase->TDNestCnt>=0) {
    bug("ERROR: Should not be in Forbid here!!\n");
    while(SysBase->TDNestCnt>=0) {
      bug("ERROR:   => call Permit.\n");
      Permit();
    }
  }
#endif
  TODO();
#if 0
	flushymin = 0;
	flushymax = currentmode->amiga_height;
	EnterCriticalSection (&screen_cs);
	if (currentmode->flags & DM_D3D) {
		v = D3D_renderframe (immediate);
	} else if (currentmode->flags & DM_SWSCALE) {
		S2X_render ();
		v = true;
	} else if (currentmode->flags & DM_DDRAW) {
		v = true;
	}
	render_ok = v;
	LeaveCriticalSection (&screen_cs);
#endif
	return render_ok;
}

static bool getvblankpos (int *vp, bool updateprev)
{
#ifndef __AROS__
	int sl;
#if 0
	frame_time_t t = read_processor_time ();
#endif
	*vp = -2;
	if (currprefs.gfx_api) {
		if (!D3D_getvblankpos (&sl))
			return false;
	} else {
		if (!DD_getvblankpos (&sl))
			return false;
	}
#if 0
	t = read_processor_time () - t;
	write_log (_T("(%d:%d)"), t, sl);
#endif
	if (updateprev && sl > prevvblankpos)
		prevvblankpos = sl;
	if (sl > maxscanline)
		maxscanline = sl;
	if (sl > 0) {
		if (sl < minscanline || minscanline < 0)
			minscanline = sl;
	}
	*vp = sl;
	return true;
#else
  return FALSE;
#endif
}

static bool isthreadedvsync (void)
{
	struct apmode *ap = picasso_on ? &currprefs.gfx_apmode[1] : &currprefs.gfx_apmode[0];
	return isvsync_chipset () <= -2 || isvsync_rtg () < 0 || ap->gfx_strobo;
}

static bool vblank_sync_started;

bool vsync_isdone (void)
{
	if (isvsync () == 0)
		return false;
	if (!isthreadedvsync ()) {
		int vp = -2;
		getvblankpos (&vp, true);
		if (!vblankthread_oddeven_got) {
			// need to get odd/even state early
			while (vp < 0) {
				if (!getvblankpos (&vp, true))
					break;
			}
			vblankthread_oddeven = (vp & 1) != 0;
			vblankthread_oddeven_got = true;
		}
	}
	if (dooddevenskip)
		return true;
	if (vblank_found_chipset)
		return true;
	return false;
}



bool vsync_switchmode (int hz)
{
	static struct PicassoResolution *oldmode;
	static int oldhz;
	int w = currentmode->native_width;
	int h = currentmode->native_height;
	int d = currentmode->native_depth / 8;
	struct MultiDisplay *md = getdisplay (&currprefs);
	struct PicassoResolution *found;
	int newh, i, cnt;
	bool preferdouble = 0, preferlace = 0;
	bool lace = false;

	if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate > 85) {
		preferdouble = 1;
	} else if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced) {
		preferlace = 1;
	}

	if (hz >= 55)
		hz = 60;
	else
		hz = 50;

	newh = h * (currprefs.ntscmode ? 60 : 50) / hz;

	found = NULL;
	for (cnt = 0; cnt <= abs (newh - h) + 1 && !found; cnt++) {
		for (int dbl = 0; dbl < 2 && !found; dbl++) {
			bool doublecheck = false;
			bool lacecheck = false;
			if (preferdouble && dbl == 0)
				doublecheck = true;
			else if (preferlace && dbl == 0)
				lacecheck = true;

			for (int extra = 1; extra >= -1 && !found; extra--) {
				for (i = 0; md->DisplayModes[i].depth >= 0 && !found; i++) {
					struct PicassoResolution *r = &md->DisplayModes[i];
					if (r->res.width == w && (r->res.height == newh + cnt || r->res.height == newh - cnt) && r->depth == d) {
						int j;
						for (j = 0; r->refresh[j] > 0; j++) {
							if (doublecheck) {
								if (r->refreshtype[j] & REFRESH_RATE_LACE)
									continue;
								if (r->refresh[j] == hz * 2 + extra) {
									found = r;
									hz = r->refresh[j];
									break;
								}
							} else if (lacecheck) {
								if (!(r->refreshtype[j] & REFRESH_RATE_LACE))
									continue;
								if (r->refresh[j] * 2 == hz + extra) {
									found = r;
									lace = true;
									hz = r->refresh[j];
									break;
								}
							} else {
								if (r->refresh[j] == hz + extra) {
									found = r;
									hz = r->refresh[j];
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	if (found == oldmode && hz == oldhz)
		return true;
	oldmode = found;
	oldhz = hz;
	if (!found) {
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsync = 0;
		if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsync != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsync) {
			set_config_changed ();
		}
		write_log (_T("refresh rate changed to %d%s but no matching screenmode found, vsync disabled\n"), hz, lace ? _T("i") : _T("p"));
		return false;
	} else {
		newh = found->res.height;
		changed_prefs.gfx_size_fs.height = newh;
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate = hz;
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced = lace;
		if (changed_prefs.gfx_size_fs.height != currprefs.gfx_size_fs.height ||
			changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate != currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate) {
			write_log (_T("refresh rate changed to %d%s, new screenmode %dx%d\n"), hz, lace ? _T("i") : _T("p"), w, newh);
			set_config_changed ();
		}
		return true;
	}
}

int default_freq = 0;


void gui_restart (void)
{
	panel_done = panel_active_done = false;
}


void updatedisplayarea (void) {
	if (!screen_is_initialized) {
    DebOut("screen_is_initialized==FALSE\n");
		return;
  }

  DX_Invalidate(0, 0, -1, -1);
}

bool target_graphics_buffer_update (void) {
  TODO();
  //updatedisplayarea();
  return TRUE;
}


void updatewinfsmode (struct uae_prefs *p)
{
	struct MultiDisplay *md;

	fixup_prefs_dimensions (p);
	if (isfullscreen_2 (p) != 0) {
		p->gfx_size = p->gfx_size_fs;
	} else {
		p->gfx_size = p->gfx_size_win;
	}
	md = getdisplay (p);
	set_config_changed ();
}



/* DirectX will fail with "Mode not supported" if we try to switch to a full
* screen mode that doesn't match one of the dimensions we got during enumeration.
* So try to find a best match for the given resolution in our list.  */
int WIN32GFX_AdjustScreenmode (struct MultiDisplay *md, int *pwidth, int *pheight, int *ppixbits)
{
	struct PicassoResolution *best;
	uae_u32 selected_mask = (*ppixbits == 8 ? RGBMASK_8BIT
		: *ppixbits == 15 ? RGBMASK_15BIT
		: *ppixbits == 16 ? RGBMASK_16BIT
		: *ppixbits == 24 ? RGBMASK_24BIT
		: RGBMASK_32BIT);
	int pass, i = 0, index = 0;

	for (pass = 0; pass < 2; pass++) {
		struct PicassoResolution *dm;
		uae_u32 mask = (pass == 0
			? selected_mask
			: RGBMASK_8BIT | RGBMASK_15BIT | RGBMASK_16BIT | RGBMASK_24BIT | RGBMASK_32BIT); /* %%% - BERND, were you missing 15-bit here??? */
		i = 0;
		index = 0;

		best = &md->DisplayModes[0];
		dm = &md->DisplayModes[1];

		while (dm->depth >= 0)  {

			/* do we already have supported resolution? */
			if (dm->res.width == *pwidth && dm->res.height == *pheight && dm->depth == (*ppixbits / 8))
				return i;

			if ((dm->colormodes & mask) != 0)  {
				if (dm->res.width <= best->res.width && dm->res.height <= best->res.height
					&& dm->res.width >= *pwidth && dm->res.height >= *pheight)
				{
					best = dm;
					index = i;
				}
				if (dm->res.width >= best->res.width && dm->res.height >= best->res.height
					&& dm->res.width <= *pwidth && dm->res.height <= *pheight)
				{
					best = dm;
					index = i;
				}
			}
			dm++;
			i++;
		}
		if (best->res.width == *pwidth && best->res.height == *pheight) {
			selected_mask = mask; /* %%% - BERND, I added this - does it make sense?  Otherwise, I'd specify a 16-bit display-mode for my
								  Workbench (using -H 2, but SHOULD have been -H 1), and end up with an 8-bit mode instead*/
			break;
		}
	}
	*pwidth = best->res.width;
	*pheight = best->res.height;
	if (best->colormodes & selected_mask)
		return index;

	/* Ordering here is done such that 16-bit is preferred, followed by 15-bit, 8-bit, 32-bit and 24-bit */
	if (best->colormodes & RGBMASK_16BIT)
		*ppixbits = 16;
	else if (best->colormodes & RGBMASK_15BIT) /* %%% - BERND, this possibility was missing? */
		*ppixbits = 15;
	else if (best->colormodes & RGBMASK_8BIT)
		*ppixbits = 8;
	else if (best->colormodes & RGBMASK_32BIT)
		*ppixbits = 32;
	else if (best->colormodes & RGBMASK_24BIT)
		*ppixbits = 24;
	else
		index = -1;

	return index;
}

/* call SDL .. */
void fill_DisplayModes(struct MultiDisplay *md);

void sortdisplays (void) {

  fill_DisplayModes(Displays);
}
