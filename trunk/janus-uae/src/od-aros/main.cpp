/************************************************************************ 
 *
 * main
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

#define OLI_DEBUG

#include "sysconfig.h"
#include "sysdeps.h"

#include "uae.h"
#include "options.h"
#include "aros.h"

#include "gfx.h"
#include "memory.h"

TCHAR VersionStr[256];
TCHAR BetaStr[64];


/************************************************************************ 
 * makeverstr
 * 
 * create a nice version
 ************************************************************************/
void makeverstr (TCHAR *s) {

	if (_tcslen (WINUAEBETA) > 0) {
		_stprintf (BetaStr, " (%sBeta %s, %d.%02d.%02d)", WINUAEPUBLICBETA > 0 ? "Public " : "", WINUAEBETA,
			GETBDY(WINUAEDATE), GETBDM(WINUAEDATE), GETBDD(WINUAEDATE));
		_stprintf (s, "WinUAE %d.%d.%d%s%s",
			UAEMAJOR, UAEMINOR, UAESUBREV, WINUAEREV, BetaStr);
	} else {
		_stprintf (s, "WinUAE %d.%d.%d%s (%d.%02d.%02d)",
			UAEMAJOR, UAEMINOR, UAESUBREV, WINUAEREV, GETBDY(WINUAEDATE), GETBDM(WINUAEDATE), GETBDD(WINUAEDATE));
	}
	if (_tcslen (WINUAEEXTRA) > 0) {
		_tcscat (s, " ");
		_tcscat (s, WINUAEEXTRA);
	}
}

extern int log_scsi;

/************************************************************************ 
 * main
 *
 * the AROS main must init / deinit all stuff around real_main
 *
 * This is the equivalent to WinMain2 in win32.cpp (hopefully)
 ************************************************************************/
int main (int argc, TCHAR **argv) {
  /* win32:
  DEVMODE devmode;
   * The DEVMODE data structure contains information about the initialization 
   * and environment of a printer or a display device. 
   */

	//if (doquit)
	//	return 0;

	//getstartpaths ();

  DebOut("main(%d, ..)\n", argc);

	makeverstr(VersionStr);
	DebOut("%s", VersionStr);
	logging_init();
	log_scsi=1;

  if(preinit_shm() /* && WIN32_RegisterClasses () && WIN32_InitLibraries ()*/ ) {
    write_log (_T("Enumerating display devices.. \n"));
    enumeratedisplays (FALSE /* multi_display*/);
  }
  else {
    write_log(_T("preinit_shm FAILED\n"));
  }




  // sortdisplays (); only for multi_display

  // TODO: Command line parsing here!
  // some process_arg()/argv magic
#if 0
	enumerate_sound_devices ();
	for (i = 0; sound_devices[i].name; i++) {
		int type = sound_devices[i].type;
		write_log (L"%d:%s: %s\n", i, type == SOUND_DEVICE_DS ? L"DS" : (type == SOUND_DEVICE_AL ? L"AL" : (type == SOUND_DEVICE_WASAPI ? L"WA" : L"PA")), sound_devices[i].name);
	}
	write_log (L"Enumerating recording devices:\n");
	for (i = 0; record_devices[i].name; i++) {
		int type = record_devices[i].type;
		write_log (L"%d:%s: %s\n", i, type == SOUND_DEVICE_DS ? L"DS" : (type == SOUND_DEVICE_AL ? L"AL" : (type == SOUND_DEVICE_WASAPI ? L"WA" : L"PA")), record_devices[i].name);
	}
	write_log (L"done\n");
#endif

/* this tries to get the dmDisplayFrequency from Windows, we don't use that for now,
   as only p96 needs it anyways
	memset (&devmode, 0, sizeof (devmode));
	devmode.dmSize = sizeof (DEVMODE);
	if (EnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &devmode)) {
		default_freq = devmode.dmDisplayFrequency;
		if (default_freq >= 70)
			default_freq = 70;
		else
			default_freq = 60;
	}
*/

	//WIN32_InitLang ();
	//WIN32_InitHtmlHelp ();
	//DirectDraw_Release ();

	DebOut("keyboard_settrans ..\n");
	keyboard_settrans ();
#ifdef CATWEASEL
	//catweasel_init ();
#endif
#ifdef PARALLEL_PORT
	//paraport_mask = paraport_init ();
#endif
	//globalipc = createIPC (L"WinUAE", 0);
	//serialipc = createIPC (COMPIPENAME, 1);
	//enumserialports ();
	//enummidiports ();

	DebOut("calling real_main..\n");
	real_main (argc, argv);

	return 0;
}

