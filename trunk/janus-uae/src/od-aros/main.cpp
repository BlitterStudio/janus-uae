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
#include "gui.h"
#include "registry.h"

TCHAR VersionStr[256];
TCHAR BetaStr[64];


static void getstartpaths (void)
{
#if 0
	TCHAR *posn, *p;
	TCHAR tmp[MAX_DPATH], tmp2[MAX_DPATH], prevpath[MAX_DPATH];
	DWORD v;
	UAEREG *key;
	TCHAR xstart_path_uae[MAX_DPATH], xstart_path_old[MAX_DPATH];
	TCHAR xstart_path_new1[MAX_DPATH], xstart_path_new2[MAX_DPATH];

	path_type = PATH_TYPE_DEFAULT;
	prevpath[0] = 0;
	xstart_path_uae[0] = xstart_path_old[0] = xstart_path_new1[0] = xstart_path_new2[0] = 0;
#if 0
	key = regcreatetree (NULL, NULL);
	if (key)  {
		int size = sizeof (prevpath) / sizeof (TCHAR);
		if (!regquerystr (key, _T("PathMode"), prevpath, &size))
			prevpath[0] = 0;
		regclosetree (key);
	}
#endif
	if (!_tcscmp (prevpath, _T("WinUAE")))
		path_type = PATH_TYPE_WINUAE;
	if (!_tcscmp (prevpath, _T("WinUAE_2")))
		path_type = PATH_TYPE_NEWWINUAE;
	if (!_tcscmp (prevpath, _T("AF2005")) || !_tcscmp (prevpath, _T("AmigaForever")))
		path_type = PATH_TYPE_NEWAF;
	if (!_tcscmp (prevpath, _T("AMIGAFOREVERDATA")))
		path_type = PATH_TYPE_AMIGAFOREVERDATA;

#if 0
	GetFullPathName (_wpgmptr, sizeof start_path_exe / sizeof (TCHAR), start_path_exe, NULL);
	if((posn = _tcsrchr (start_path_exe, '\\')))
		posn[1] = 0;
#endif

#if 0
	if (path_type == PATH_TYPE_DEFAULT && inipath) {
#endif
		path_type = PATH_TYPE_WINUAE;
		_tcscpy (xstart_path_uae, start_path_exe);
		relativepaths = 1;
#if 0
	} else if (path_type == PATH_TYPE_DEFAULT && start_data == 0 && key) {
		bool ispath = false;
		_tcscpy (tmp2, start_path_exe);
		_tcscat (tmp2, _T("configurations\\configuration.cache"));
		v = GetFileAttributes (tmp2);
		if (v != INVALID_FILE_ATTRIBUTES && !(v & FILE_ATTRIBUTE_DIRECTORY))
			ispath = true;
		_tcscpy (tmp2, start_path_exe);
		_tcscat (tmp2, _T("roms"));
		v = GetFileAttributes (tmp2);
		if (v != INVALID_FILE_ATTRIBUTES && (v & FILE_ATTRIBUTE_DIRECTORY))
			ispath = true;
		if (!ispath) {
			if (SUCCEEDED (SHGetFolderPath (NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, tmp))) {
				GetFullPathName (tmp, sizeof tmp / sizeof (TCHAR), tmp2, NULL);
				// installed in Program Files?
				if (_tcsnicmp (tmp, tmp2, _tcslen (tmp)) == 0) {
					if (SUCCEEDED (SHGetFolderPath (NULL, CSIDL_COMMON_DOCUMENTS, NULL, SHGFP_TYPE_CURRENT, tmp))) {
						fixtrailing (tmp);
						_tcscpy (tmp2, tmp);
						_tcscat (tmp2, _T("Amiga Files"));
						CreateDirectory (tmp2, NULL);
						_tcscat (tmp2, _T("\\WinUAE"));
						CreateDirectory (tmp2, NULL);
						v = GetFileAttributes (tmp2);
						if (v != INVALID_FILE_ATTRIBUTES && (v & FILE_ATTRIBUTE_DIRECTORY)) {
							_tcscat (tmp2, _T("\\"));
							path_type = PATH_TYPE_NEWWINUAE;
							_tcscpy (tmp, tmp2);
							_tcscat (tmp, _T("Configurations"));
							CreateDirectory (tmp, NULL);
							_tcscpy (tmp, tmp2);
							_tcscat (tmp, _T("Screenshots"));
							CreateDirectory (tmp, NULL);
							_tcscpy (tmp, tmp2);
							_tcscat (tmp, _T("Savestates"));
							CreateDirectory (tmp, NULL);
							_tcscpy (tmp, tmp2);
							_tcscat (tmp, _T("Screenshots"));
							CreateDirectory (tmp, NULL);
						}
					}
				}
			}
		}
	}
#endif

	_tcscpy (tmp, start_path_exe);
	_tcscat (tmp, _T("roms"));
#if 0
	if (isfilesindir (tmp)) {
		_tcscpy (xstart_path_uae, start_path_exe);
	}
#endif
	_tcscpy (tmp, start_path_exe);
	_tcscat (tmp, _T("configurations"));
#if 0
	if (isfilesindir (tmp)) {
		_tcscpy (xstart_path_uae, start_path_exe);
	}
#endif

#if 0
	p = _wgetenv (_T("AMIGAFOREVERDATA"));
	if (p) {
		_tcscpy (tmp, p);
		fixtrailing (tmp);
		_tcscpy (start_path_new2, p);
		v = GetFileAttributes (tmp);
		if (v != INVALID_FILE_ATTRIBUTES && (v & FILE_ATTRIBUTE_DIRECTORY)) {
			_tcscpy (xstart_path_new2, start_path_new2);
			_tcscpy (xstart_path_new2, _T("WinUAE\\"));
			af_path_2005 |= 2;
		}
	}

	if (SUCCEEDED (SHGetFolderPath (NULL, CSIDL_COMMON_DOCUMENTS, NULL, SHGFP_TYPE_CURRENT, tmp))) {
		fixtrailing (tmp);
		_tcscpy (tmp2, tmp);
		_tcscat (tmp2, _T("Amiga Files\\"));
		_tcscpy (tmp, tmp2);
		_tcscat (tmp, _T("WinUAE"));
		v = GetFileAttributes (tmp);
		if (v != INVALID_FILE_ATTRIBUTES && (v & FILE_ATTRIBUTE_DIRECTORY)) {
			TCHAR *p;
			_tcscpy (xstart_path_new1, tmp2);
			_tcscat (xstart_path_new1, _T("WinUAE\\"));
			_tcscpy (xstart_path_uae, start_path_exe);
			_tcscpy (start_path_new1, xstart_path_new1);
			p = tmp2 + _tcslen (tmp2);
			_tcscpy (p, _T("System"));
			if (isfilesindir (tmp2)) {
				af_path_2005 |= 1;
			} else {
				_tcscpy (p, _T("Shared"));
				if (isfilesindir (tmp2)) {
					af_path_2005 |= 1;
				}
			}
		}
	}
#endif

	if (start_data == 0) {
		start_data = 1;
		if (path_type == PATH_TYPE_WINUAE && xstart_path_uae[0]) {
			_tcscpy (start_path_data, xstart_path_uae);
		} else if (path_type == PATH_TYPE_NEWWINUAE && xstart_path_new1[0]) {
			_tcscpy (start_path_data, xstart_path_new1);
			create_afnewdir (0);
		} else if (path_type == PATH_TYPE_NEWAF && (af_path_2005 & 1) && xstart_path_new1[0]) {
			_tcscpy (start_path_data, xstart_path_new1);
			create_afnewdir (0);
		} else if (path_type == PATH_TYPE_AMIGAFOREVERDATA && (af_path_2005 & 2) && xstart_path_new2[0]) {
			_tcscpy (start_path_data, xstart_path_new2);
		} else if (path_type == PATH_TYPE_DEFAULT) {
			_tcscpy (start_path_data, xstart_path_uae);
			if (af_path_2005 & 1) {
				path_type = PATH_TYPE_NEWAF;
				create_afnewdir (1);
				_tcscpy (start_path_data, xstart_path_new1);
			}
			if (af_path_2005 & 2) {
				_tcscpy (tmp, xstart_path_new2);
				_tcscat (tmp, _T("system\\rom"));
#if 0
				if (isfilesindir (tmp)) {
					path_type = PATH_TYPE_AMIGAFOREVERDATA;
				} else {
#endif
					_tcscpy (tmp, xstart_path_new2);
					_tcscat (tmp, _T("shared\\rom"));
#if 0
					if (isfilesindir (tmp)) {
						path_type = PATH_TYPE_AMIGAFOREVERDATA;
					} else {
#endif
						path_type = PATH_TYPE_NEWWINUAE;
#if 0
					}
				}
#endif
				_tcscpy (start_path_data, xstart_path_new2);
			}
		}
	}

	v = GetFileAttributes (start_path_data);
	if (v == INVALID_FILE_ATTRIBUTES || !(v & FILE_ATTRIBUTE_DIRECTORY) || start_data == 0 || start_data == -2) {
		_tcscpy (start_path_data, start_path_exe);
	}
	fixtrailing (start_path_data);
	fullpath (start_path_data, sizeof start_path_data / sizeof (TCHAR));
	SetCurrentDirectory (start_path_data);

	// use path via command line?
	if (!start_path_plugins[0]) {
		// default path
		_tcscpy (start_path_plugins, start_path_data);
		_tcscat (start_path_plugins, WIN32_PLUGINDIR);
	}
	v = GetFileAttributes (start_path_plugins);
	if (!start_path_plugins[0] || v == INVALID_FILE_ATTRIBUTES || !(v & FILE_ATTRIBUTE_DIRECTORY)) {
		// not found, exe path?
		_tcscpy (start_path_plugins, start_path_exe);
		_tcscat (start_path_plugins, WIN32_PLUGINDIR);
		v = GetFileAttributes (start_path_plugins);
		if (v == INVALID_FILE_ATTRIBUTES || !(v & FILE_ATTRIBUTE_DIRECTORY))
			_tcscpy (start_path_plugins, start_path_data); // not found, very default path
	}
	fixtrailing (start_path_plugins);
	fullpath (start_path_plugins, sizeof start_path_plugins / sizeof (TCHAR));
	setpathmode (path_type);
#endif
}

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

static TCHAR *inipath = NULL;

static TCHAR *getdefaultini (void) {
  return strdup("PROGDIR:winuae.ini");
}
/************************************************************************ 
 * main
 *
 * the AROS main must init / deinit all stuff around real_main
 *
 * This is the equivalent to WinMain2 in win32.cpp (hopefully)
 ************************************************************************/
void WIN32_HandleRegistryStuff (void);

int main (int argc, TCHAR **argv) {
  /* win32:
  DEVMODE devmode;
   * The DEVMODE data structure contains information about the initialization 
   * and environment of a printer or a display device. 
   */

	//if (doquit)
	//	return 0;

  DebOut("main(%d, ..)\n", argc);

  inipath=getdefaultini();
  reginitializeinit(&inipath);
  getstartpaths ();
	makeverstr(VersionStr);
	DebOut("%s", VersionStr);
	logging_init();
	log_scsi=1;

#ifdef NATMEM_OFFSET
  if(preinit_shm() /* && WIN32_RegisterClasses () && WIN32_InitLibraries ()*/ ) {
    write_log (_T("Enumerating display devices.. \n"));
#endif
    enumeratedisplays (FALSE /* multi_display*/);
#ifdef NATMEM_OFFSET
  }
  else {
    write_log(_T("preinit_shm FAILED\n"));
  }
#endif
  WIN32_HandleRegistryStuff ();




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

  regflushfile();

	return 0;
}

