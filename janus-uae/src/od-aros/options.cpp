/************************************************************************ 
 *
 * AROS cfg options
 *
 * (win32 stores stuff like win32_active_priority etc with the help of
 *  those functions). TODO
 *
 * Copyright 2015       Oliver Brunner - aros<at>oliver-brunner.de
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
#include "audio.h"
#include "uae.h"
#include "memory.h"
#include "rommgr.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "traps.h"
#include "xwin.h"
#include "keyboard.h"
#include "inputdevice.h"
#include "keybuf.h"
#include "drawing.h"
#include "picasso96_aros.h"
#include "bsdsocket.h"
#include "registry.h"
#include "autoconf.h"
#include "gui.h"
#include "sys/mman.h"
#include "zfile.h"
#include "savestate.h"
#include "scsidev.h"
#include "disk.h"
#include "catweasel.h"
#include "uaeipc.h"
#include "ar.h"
#include "akiko.h"
#include "cdtv.h"
#include "blkdev.h"
#include "gfx.h"
#include "string_resource.h"

void target_fixup_options (struct uae_prefs *p) {
}

void target_default_options (struct uae_prefs *p, int type) {

	TCHAR buf[MAX_DPATH];
	if (type == 2 || type == 0 || type == 3) {
		p->win32_middle_mouse = 1;
		p->win32_logfile = 0;
		p->win32_active_nocapture_pause = 0;
		p->win32_active_nocapture_nosound = 0;
		p->win32_iconified_nosound = 1;
		p->win32_iconified_pause = 1;
		p->win32_inactive_nosound = 0;
		p->win32_inactive_pause = 0;
		p->win32_ctrl_F11_is_quit = 0;
		p->win32_soundcard = 0;
		p->win32_samplersoundcard = -1;
		p->win32_minimize_inactive = 0;
		p->win32_start_minimized = false;
		p->win32_start_uncaptured = false;
		p->win32_active_capture_priority = 1;
		//p->win32_active_nocapture_priority = 1;
		p->win32_inactive_priority = 2;
		p->win32_iconified_priority = 3;
		p->win32_notaskbarbutton = false;
		p->win32_nonotificationicon = false;
		p->win32_alwaysontop = false;
		p->win32_guikey = -1;
		p->win32_automount_removable = 0;
		p->win32_automount_drives = 0;
		p->win32_automount_removabledrives = 0;
		p->win32_automount_cddrives = 0;
		p->win32_automount_netdrives = 0;
		p->win32_kbledmode = 1;
		p->win32_uaescsimode = UAESCSI_CDEMU;
		p->win32_borderless = 0;
		p->win32_blankmonitors = false;
		p->win32_powersavedisabled = true;
		p->sana2 = 0;
		p->win32_rtgmatchdepth = 1;
		p->gf[APMODE_RTG].gfx_filter_autoscale = RTG_MODE_SCALE;
		p->win32_rtgallowscaling = 0;
		p->win32_rtgscaleaspectratio = -1;
		p->win32_rtgvblankrate = 0;
		p->rtg_hardwaresprite = true;
		p->win32_commandpathstart[0] = 0;
		p->win32_commandpathend[0] = 0;
		p->win32_statusbar = 1;
#if 0
		p->gfx_api = os_vista ? 1 : 0;
#endif
		if (p->gf[APMODE_NATIVE].gfx_filter == 0 && p->gfx_api)
			p->gf[APMODE_NATIVE].gfx_filter = 1;
		if (p->gf[APMODE_RTG].gfx_filter == 0 && p->gfx_api)
			p->gf[APMODE_RTG].gfx_filter = 1;
#if 0
		WIN32GUI_LoadUIString (IDS_INPUT_CUSTOM, buf, sizeof buf / sizeof (TCHAR));
		for (int i = 0; i < GAMEPORT_INPUT_SETTINGS; i++)
			_stprintf (p->input_config_name[i], buf, i + 1);
#endif
	}
	if (type == 1 || type == 0 || type == 3) {
		p->win32_uaescsimode = UAESCSI_CDEMU;
		p->win32_midioutdev = -2;
		p->win32_midiindev = 0;
		p->win32_midirouter = false;
		p->win32_automount_removable = 0;
		p->win32_automount_drives = 0;
		p->win32_automount_removabledrives = 0;
		p->win32_automount_cddrives = 0;
		p->win32_automount_netdrives = 0;
		p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5PC | RGBFF_B8G8R8A8;
		p->win32_filesystem_mangle_reserved_names = true;
	}

	/* TODO: we want janus-uae_log.txt */
	currprefs.win32_logfile=TRUE;
}

void target_save_options (struct zfile *f, struct uae_prefs *p) {
}

/* don't load any config for now */
int target_cfgfile_load (struct uae_prefs *p, const TCHAR *filename, int type, int isdefault)
{

	int v, i, type2;
	int ct, ct2, size;
	TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH];
	TCHAR fname[MAX_DPATH*2];

	DebOut("load %s\n", filename);

	strcpy(fname, filename);
#if 0
	_tcscpy (fname, filename);
	if (!zfile_exists (fname)) {
		fetch_configurationpath (fname, sizeof (fname) / sizeof (TCHAR));
		if (_tcsncmp (fname, filename, _tcslen (fname)))
			_tcscat (fname, filename);
		else
			_tcscpy (fname, filename);
	}
#endif

#if 0
	if (!isdefault)
		qs_override = 1;
#endif
	if (type < 0) {
		type = 0;
		cfgfile_get_description (fname, NULL, NULL, NULL, &type);
	}
	if (type == 0 || type == 1) {
		discard_prefs (p, 0);
	}
	type2 = type;
	if (type == 0) {
		default_prefs (p, type);
#if 0
		if (isdefault == 0) {
			fetch_configurationpath (tmp1, sizeof (tmp1) / sizeof (TCHAR));
			_tcscat (tmp1, OPTIONSFILENAME);
			cfgfile_load (p, tmp1, NULL, 0, 0);
		}
#endif
	}
		
#if 0
	regqueryint (NULL, L"ConfigFile_NoAuto", &ct2);
#endif
	v = cfgfile_load (p, fname, &type2, 0, isdefault ? 0 : 1);

	return v;
#if 0
	if (!v)
		return v;
	if (type > 0)
		return v;
	for (i = 1; i <= 2; i++) {
		if (type != i) {
			size = sizeof (ct);
			ct = 0;
#if 0
			regqueryint (NULL, configreg2[i], &ct);
#endif
			if (ct && ((i == 1 && p->config_hardware_path[0] == 0) || (i == 2 && p->config_host_path[0] == 0) || ct2)) {
				size = sizeof (tmp1) / sizeof (TCHAR);
				regquerystr (NULL, configreg[i], tmp1, &size);
				fetch_path (L"ConfigurationPath", tmp2, sizeof (tmp2) / sizeof (TCHAR));
				_tcscat (tmp2, tmp1);
				v = i;
				cfgfile_load (p, tmp2, &v, 1, 0);
			}
		}
	}
	v = 1;
	return v;
#endif
}

int target_parse_option (struct uae_prefs *p, const TCHAR *option, const TCHAR *value) 
//int target_parse_option (struct uae_prefs *p, char *option, char *value)
{
  TODO();
}

TCHAR *target_expand_environment (const TCHAR *path)
{
	return my_strdup(path);
}

