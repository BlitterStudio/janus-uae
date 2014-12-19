#define OLI_DEBUG
#include <proto/dos.h>
#include <intuition/intuition.h>

#ifdef HAVE_LIBRARIES_CYBERGRAPHICS_H
# define CGX_CGX_H <libraries/cybergraphics.h>
# define USE_CYBERGFX           /* define this to have cybergraphics support */
#else
# ifdef HAVE_CYBERGRAPHX_CYBERGRAPHICS_H
#  define USE_CYBERGFX
#  define CGX_CGX_H <cybergraphx/cybergraphics.h>
# endif
#endif
#ifdef USE_CYBERGFX
# if defined __MORPHOS__ || defined __AROS__ || defined __amigaos4__
#  define USE_CYBERGFX_V41
# endif
#endif

#include <libraries/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <proto/dos.h>

#include "sysconfig.h"
#include "sysdeps.h"

//#include "resource"

//#include <wintab.h>
//#include "wintablet.h"
//#include <pktdef.h>

#include "sysdeps.h"
#include "options.h"
#include "audio.h"
#include "sounddep/sound.h"
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
//#include "dxwrap.h"
#include "picasso96_aros.h"
//#include "bsdsocket.h"
#include "aros.h"
//#include "arosgfx.h"
#include "registry.h"
//#include "win32gui.h"
#include "autoconf.h"
#include "gui.h"
//#include "sys/mman.h"
//#include "avioutput.h"
//#include "ahidsound.h"
//#include "ahidsound_new.h"
#include "zfile.h"
#include "savestate.h"
//#include "ioport.h"
//#include "parser.h"
#include "scsidev.h"
#include "disk.h"
#include "catweasel.h"
//#include "lcd.h"
#include "uaeipc.h"
#include "ar.h"
#include "akiko.h"
#include "cdtv.h"
//#include "direct3d.h"
//#include "clipboard_win32.h"
#include "blkdev.h"
#include "inputrecord.h"
#ifdef RETROPLATFORM
#include "rp.h"
#include "cloanto/RetroPlatformIPC.h"
#endif
#include "gfx.h"
#include "sys/mman.h"


//#include "options.h"
//#include "include/scsidev.h"

#include "od-aros/tchar.h"

extern struct Window *hAmigaWnd;
/* not really used: */
HWND hHiddenWnd;
HMODULE hUIDLL = NULL;
HMODULE hInst = NULL;

/* visual C++ symbols ..*/

int _daylight=0; 
int _dstbias=0 ;
long _timezone=0; 
char tzname[2][4]={"PST", "PDT"};

/* win32.cpp stuff */
int pissoff_value = 25000;

int log_scsi;
int log_net;

/* missing symbols from various od-win32 sources */
int tablet_log=0;
int seriallog =0;
int max_uae_width;
int max_uae_height;
int log_vsync, debug_vsync_min_delay, debug_vsync_forced_delay;
int extraframewait;

/* end */

int pause_emulation;
int sleep_resolution;
int uaelib_debug;

TCHAR start_path_data[MAX_DPATH];
TCHAR start_path_exe[MAX_DPATH];
TCHAR start_path_plugins[MAX_DPATH];
TCHAR start_path_new1[MAX_DPATH]; /* AF2005 */
TCHAR start_path_new2[MAX_DPATH]; /* AMIGAFOREVERDATA */
TCHAR help_file[MAX_DPATH];
TCHAR bootlogpath[MAX_DPATH];
TCHAR logpath[MAX_DPATH];
bool winuaelog_temporary_enable;
int af_path_2005;

int quickstart = 1, configurationcache = 1, relativepaths = 1; 

static int forceroms;

static void createdir (const TCHAR *path) {
  CreateDir(path);
}

bool is_dir(const TCHAR *name) {

  struct FileInfoBlock *fib;
  BPTR file=NULL;
  bool ret=FALSE;

  DebOut("name: %s\n", name);

  fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, TAG_END);
  if(!fib) 
  {
    DebOut("no FileInfoBlock!\n");
    goto NODIR;
  }

  file=Lock(name, SHARED_LOCK);
  if(!file) 
  {
    DebOut("no lock..\n");
    goto NODIR;
  }

  if (!Examine(file, fib)) 
  {
    DebOut("Examine failed\n");
    goto NODIR;
  }

  if(fib->fib_DirEntryType <0) 
  {
    DebOut("FILE!\n");
    goto NODIR;
  }
  else 
  {
    DebOut("directory..\n");
    ret=TRUE;
  }

NODIR:
  if(fib) 
  {
    FreeDosObject(DOS_FIB, fib);
  }
  if(file) 
  {
    UnLock(file);
  }

  return ret;
}


void stripslashes (TCHAR *p)
{
	while (_tcslen (p) > 0 && (p[_tcslen (p) - 1] == '\\' || p[_tcslen (p) - 1] == '/'))
		p[_tcslen (p) - 1] = 0;
}

void fetch_configurationpath (TCHAR *out, int size)
{
	strncpy(out, "configurations/", size);
}

void fetch_saveimagepath (TCHAR *out, int size, int dir)
{
  fetch_path (_T("SaveimagePath"), out, size);
  if (dir) {
    out[_tcslen (out) - 1] = (TCHAR) 0;
    createdir (out);
    fetch_path (_T("SaveimagePath"), out, size);
  }
}
#if 0
void fetch_screenshotpath (TCHAR *out, int size)
{
	fetch_path (_T("ScreenshotPath"), out, size);
}
void fetch_ripperpath (TCHAR *out, int size)
{
	fetch_path (_T("RipperPath"), out, size);
}
void fetch_statefilepath (TCHAR *out, int size)
{
	fetch_path (_T("StatefilePath"), out, size);
}
void fetch_inputfilepath (TCHAR *out, int size)
{
	fetch_path (_T("InputPath"), out, size);
}
void fetch_datapath (TCHAR *out, int size)
{
	fetch_path (NULL, out, size);
}
#endif


void fetch_path (const TCHAR *name, TCHAR *out, int size) {
	int size2 = size;

	_tcscpy (out, start_path_data);
	if (!name) {
		fullpath (out, size);
		return;
	}
	if (!_tcscmp (name, _T("FloppyPath")))
		_tcscat (out, _T("PROGDIR:adf"));
	if (!_tcscmp (name, _T("CDPath")))
		_tcscat (out, _T("PROGDIR:cd"));
	if (!_tcscmp (name, _T("TapePath")))
		_tcscat (out, _T("PROGDIR:tape\\"));
	if (!_tcscmp (name, _T("hdfPath")))
		_tcscat (out, _T("PROGDIR:hdf"));
	if (!_tcscmp (name, _T("KickstartPath")))
		_tcscat (out, _T("PROGDIR:Roms/"));
	if (!_tcscmp (name, _T("ConfigurationPath")))
		_tcscat (out, _T("PROGDIR:Configurations\\"));
	if (!_tcscmp (name, _T("LuaPath")))
		_tcscat (out, _T("PROGDIR:lua"));
	if (!_tcscmp (name, _T("StatefilePath")))
		_tcscat (out, _T("PROGDIR:Savestates"));
	if (!_tcscmp (name, _T("InputPath")))
		_tcscat (out, _T("PROGDIR:Inputrecordings\\"));
#if 0
	if (start_data >= 0)
		regquerystr (NULL, name, out, &size); 
	if (GetFileAttributes (out) == INVALID_FILE_ATTRIBUTES)
		_tcscpy (out, start_path_data);
#endif
#if 0
	if (out[0] == '\\' && (_tcslen (out) >= 2 && out[1] != '\\')) { /* relative? */
		_tcscpy (out, start_path_data);
		if (start_data >= 0) {
			size2 -= _tcslen (out);
			regquerystr (NULL, name, out, &size2);
		}
	}
#endif
	stripslashes (out);
#if 0
	if (!_tcscmp (name, _T("KickstartPath"))) {
		DWORD v = GetFileAttributes (out);
		if (v == INVALID_FILE_ATTRIBUTES || !(v & FILE_ATTRIBUTE_DIRECTORY))
			_tcscpy (out, start_path_data);
	}
#endif
	fixtrailing (out);
	fullpath (out, size);
}

/* resides in win32gui.cpp normally 
 *
 * copies the text of message number msg to out (?)
 */
int translate_message (int msg, TCHAR *out) {

	out[0] = (TCHAR) 0;
	return 0;
}

/* resides in win32.cpp normally
 *
 * loads amigaforever keyfiles ..?
 *
 * FS-UAE also has this as a pure stub
 */
uae_u8 *target_load_keyfile (struct uae_prefs *p, const TCHAR *path, int *sizep, TCHAR *name) {
	bug("target_load_keyfile(,%s,%d,%s)\n", path, *sizep, name);
	TODO();
  return NULL;
}


/* make sure, the path can be concatenated with a filename */
void fixtrailing (TCHAR *p) {

	if (!p)
		return;

	if (p[strlen(p) - 1] == '/' || p[strlen(p) - 1] == ':')
		return;

	strcat(p, "/");
}

/* convert path to absolute or relative */
void fullpath (TCHAR *path, int size) {
	BPTR    lock;
	TCHAR   tmp1[MAX_DPATH], tmp2[MAX_DPATH];
	BOOL    result;

	DebOut("path %s, %d (relativepaths is %d)\n", path, size, relativepaths);

	if (path[0] == 0) {
		return;
	}

	if (relativepaths) {
		GetCurrentDirName(tmp1, MAX_DPATH);
		//DebOut("GetCurrentDirName: %s\n", tmp1);

		lock=Lock(path, SHARED_LOCK);
		if(!lock) {
			//DebOut("failed to lock %s\n", path);
			return;
		}
		result=NameFromLock(lock, tmp2, MAX_DPATH);
		UnLock(lock);
		if(!result) {
			//DebOut("failed to NameFromLock(%s)\n", path);
			return;
		}
		//DebOut("NameFromLock(%s): %s\n", path, tmp2);


		if (strnicmp (tmp1, tmp2, strlen (tmp1)) == 0) { // tmp2 is inside tmp1
			//DebOut("tmp2 (%s) is inside tmp1 (%s)\n", tmp2, tmp1);
			strcpy(path, tmp2 + strlen (tmp1));
		}
		else {
			//DebOut("tmp2 (%s) is not inside tmp1 (%s)\n", tmp2, tmp1);
			strcpy(path, tmp2);
		}
	}
	else {
		lock=Lock(path, SHARED_LOCK);
		if(!lock) {
			//DebOut("failed to lock %s\n", path);
			return;
		}
		result=NameFromLock(lock, tmp1, MAX_DPATH);
		UnLock(lock);
		if(!result) {
			//DebOut("failed to NameFromLock(%s)\n", path);
			return;
		}
		//DebOut("NameFromLock(%s): %s\n", path, tmp1);
		strcpy(path, tmp1);
	}

	DebOut("result: %s\n", path);
}

/* taken from puae/misc.c */

static struct MultiDisplay *getdisplay2 (struct uae_prefs *p, int index)
{
	int max;
#ifndef __AROS__
	int display = index < 0 ? p->gfx_apmode[screen_is_picasso ? APMODE_RTG : APMODE_NATIVE].gfx_display - 1 : index;
#else
  int display=0;
#endif

	write_log ("Multimonitor detection disabled\n");
	Displays[0].primary  = TRUE;
	Displays[0].adaptername     = (TCHAR *) "Display";
	Displays[0].monitorname     = (TCHAR *) "Display";
  //Displays[0].disabled = 0;
  if(Displays[0].DisplayModes == NULL) {
    DebOut("alloc DisplayModes array. Should this be done here !? really !?\n");
    Displays[0].DisplayModes = xmalloc (struct PicassoResolution, MAX_PICASSO_MODES);
  }

	return &Displays[0];
}

struct MultiDisplay *getdisplay (struct uae_prefs *p)
{
	return getdisplay2 (p, -1);
}

/* starts one command in the shell, before emulation starts ..? */
void target_run (void)
{
	//shellexecute (currprefs.win32_commandpathstart);
	/* TODO ? */
}


#if 0
static int get_BytesPerPix(struct Window *win) {
  struct Screen *scr;
  int res;

  if(!win) {
    DebOut("\nERROR: win is NULL\n");
    write_log("ERROR: win is NULL\n");
    return 0;
  }

  scr=win->WScreen;

  if(!scr) {
    DebOut("\nERROR: win->WScreen is NULL\n");
    write_log("\nERROR: win->WScreen is NULL\n");
    return 0;
  }

  if(!GetCyberMapAttr(scr->RastPort.BitMap, CYBRMATTR_ISCYBERGFX)) {
    DebOut("\nERROR: !CYBRMATTR_ISCYBERGFX\n");
    write_log("\nERROR: !CYBRMATTR_ISCYBERGFX\n");
  }

  res=GetCyberMapAttr(scr->RastPort.BitMap, CYBRMATTR_BPPIX);

  return res;
}


static int get_BytesPerRow(struct Window *win) {
  WORD width;

#if 0
  if(!uae_main_window_closed) {
    width=W->Width;
    JWLOG("1: %d * %d = %d\n",width,get_BytesPerPix(win),width*get_BytesPerPix(win));
  }
  else {
#endif
    width=gfxvidinfo.width;
#if 0
    JWLOG("2: %d * %d = %d\n",picasso_vidinfo.width,get_BytesPerPix(win),picasso_vidinfo.width*get_BytesPerPix(win));
  }
#endif

  return width * get_BytesPerPix(win);
}
#endif

int WIN32GFX_IsPicassoScreen (void)
{
    return screen_is_picasso;
}

static void sleep_millis2 (int ms, bool main) {

  if(main) {
    DebOut("warning: main is %d (not cared for)\n", main);
  }

  Delay(ms);
}

void sleep_millis_main (int ms) {

	sleep_millis2 (ms, true);
}

void sleep_millis (int ms) {

	sleep_millis2 (ms, false);
}


void target_restart (void)
{
	gui_restart ();
}


void fetch_rompath (TCHAR *out, int size)
{
	fetch_path (_T("KickstartPath"), out, size);
}

static bool rawinput_enabled_mouse;
static bool rawinput_enabled_keyboard=FALSE;

int input_get_default_keyboard (int i)
{
  if (rawinput_enabled_keyboard) {
    return 1;
  } else {
    if (i == 0)
      return 1;
    return 0;
  }
}

#ifndef NATMEM_OFFSET
void protect_roms (bool protect) {
}


void unprotect_maprom (void) {
}
#endif


/* Loads a string resource from the executable file associated 
 * with a specified module, copies the string into a buffer, and 
 * appends a terminating null character.
 */
int LoadString(APTR hInstance, TCHAR *uID, TCHAR * lpBuffer, int nBufferMax) {

  DebOut("cp %s ..\n", uID);

  strncpy(lpBuffer, uID, nBufferMax);

  return strlen(lpBuffer)-1;
}


static int isfilesindir (const TCHAR *p)
{
#if 0
	WIN32_FIND_DATA fd;
	HANDLE h;
	TCHAR path[MAX_DPATH];
	int i = 0;
	DWORD v;

	v = GetFileAttributes (p);
	if (v == INVALID_FILE_ATTRIBUTES || !(v & FILE_ATTRIBUTE_DIRECTORY))
		return 0;
	_tcscpy (path, p);
	_tcscat (path, _T("\\*.*"));
	h = FindFirstFile (path, &fd);
	if (h != INVALID_HANDLE_VALUE) {
		for (i = 0; i < 3; i++) {
			if (!FindNextFile (h, &fd))
				break;
		}
		FindClose (h);
	}
	if (i == 3)
		return 1;
#endif
	return 0;
}

int GetFileAttributes(char *path) {
  BPTR    lock;
  BOOL    result;

  DebOut("path: %s\n", path);

  lock=Lock(path, SHARED_LOCK);
  if(!lock) {
    return INVALID_FILE_ATTRIBUTES;
  }

  UnLock(lock);
  return 1;
}

int get_rom_path (TCHAR *out, pathtype mode)
{
	TCHAR tmp[MAX_DPATH];

  DebOut("pathtype: %d\n", mode);
  DebOut("start_path_data: %s\n", start_path_data);
  DebOut("start_path_exe: %s\n", start_path_exe);

	tmp[0] = 0;
	switch (mode)
	{
	case PATH_TYPE_DEFAULT:
		{
      DebOut("PATH_TYPE_DEFAULT\n");
			if (!_tcscmp (start_path_data, start_path_exe))
				_tcscpy (tmp, _T(""));
			else
				_tcscpy (tmp, start_path_data);

			if (GetFileAttributes (tmp) != INVALID_FILE_ATTRIBUTES) {
				TCHAR tmp2[MAX_DPATH];
				_tcscpy (tmp2, tmp);
				_tcscat (tmp2, _T("rom"));
				if (GetFileAttributes (tmp2) != INVALID_FILE_ATTRIBUTES) {
					_tcscpy (tmp, tmp2);
				} else {
					_tcscpy (tmp2, tmp);
					_tcscpy (tmp2, _T("roms"));
					if (GetFileAttributes (tmp2) != INVALID_FILE_ATTRIBUTES) {
						_tcscpy (tmp, tmp2);
					} else {
						if (!get_rom_path (tmp, PATH_TYPE_NEWAF)) {
							if (!get_rom_path (tmp, PATH_TYPE_AMIGAFOREVERDATA)) {
								_tcscpy (tmp, start_path_data);
							}
						}
					}
				}
			}
		}
		break;
	case PATH_TYPE_NEWAF:
		{
      DebOut("PATH_TYPE_NEWAF\n");
			TCHAR tmp2[MAX_DPATH];
			_tcscpy (tmp2, start_path_new1);
			_tcscat (tmp2, _T("..\\system\\rom"));
			if (isfilesindir (tmp2)) {
				_tcscpy (tmp, tmp2);
				break;
			}
			_tcscpy (tmp2, start_path_new1);
			_tcscat (tmp2, _T("..\\shared\\rom"));
			if (isfilesindir (tmp2)) {
				_tcscpy (tmp, tmp2);
				break;
			}
		}
		break;
	case PATH_TYPE_AMIGAFOREVERDATA:
		{
      DebOut("PATH_TYPE_AMIGAFOREVERDATA\n");
			TCHAR tmp2[MAX_DPATH];
			_tcscpy (tmp2, start_path_new2);
			_tcscat (tmp2, _T("system\\rom"));
			if (isfilesindir (tmp2)) {
				_tcscpy (tmp, tmp2);
				break;
			}
			_tcscpy (tmp2, start_path_new2);
			_tcscat (tmp2, _T("shared\\rom"));
			if (isfilesindir (tmp2)) {
				_tcscpy (tmp, tmp2);
				break;
			}
		}
		break;
	default:
    DebOut("default: return -1\n");
		return -1;
	}
	if (isfilesindir (tmp)) {
		_tcscpy (out, tmp);
		fixtrailing (out);
	}
	if (out[0]) {
		fullpath (out, MAX_DPATH);
	}
  DebOut("result: %s\n", out);
	return out[0] ? 1 : 0;
}

int regexiststree (UAEREG *root, const TCHAR *name);
UAEREG *regcreatetree (UAEREG *root, const TCHAR *name);
int regenumstr (UAEREG *root, int idx, TCHAR *name, int *nsize, TCHAR *str, int *size);
void regclosetree (UAEREG *key);
int getregmode (void);


static void romlist_add2 (const TCHAR *path, struct romdata *rd)
{
  DebOut("path: %s\n", path);
	if (getregmode ()) {
		int ok = 0;
		TCHAR tmp[MAX_DPATH];
		if (path[0] == '/' || path[0] == '\\')
			ok = 1;
		if (_tcslen (path) > 1 && path[1] == ':')
			ok = 1;
		if (!ok) {
			_tcscpy (tmp, start_path_exe);
			_tcscat (tmp, path);
			romlist_add (tmp, rd);
			return;
		}
	}
	romlist_add (path, rd);
}

extern int scan_roms (HWND, int);
void read_rom_list (void)
{
	TCHAR tmp2[1000];
	int idx, idx2;
	UAEREG *fkey;
	TCHAR tmp[1000];
	int size, size2, exists;

  DebOut("entered\n");

	romlist_clear ();
	exists = regexiststree (NULL, _T("DetectedROMs"));
	fkey = regcreatetree (NULL, _T("DetectedROMs"));
	if (fkey == NULL)
		return;
	if (!exists || forceroms) {
		load_keyring (NULL, NULL);
		scan_roms (NULL, forceroms ? 0 : 1);
	}

	forceroms = 0;
	idx = 0;
	for (;;) {
		size = sizeof (tmp) / sizeof (TCHAR);
		size2 = sizeof (tmp2) / sizeof (TCHAR);
    DebOut(".........\n");
		if (!regenumstr (fkey, idx, tmp, &size, tmp2, &size2))
			break;
		if (_tcslen (tmp) == 7 || _tcslen (tmp) == 13) {
			int group = 0;
			int subitem = 0;
			idx2 = _tstol (tmp + 4);
			if (_tcslen (tmp) == 13) {
				group = _tstol (tmp + 8);
				subitem = _tstol (tmp + 11);
			}
			if (idx2 >= 0 && _tcslen (tmp2) > 0) {
				struct romdata *rd = getromdatabyidgroup (idx2, group, subitem);
				if (rd) {
					TCHAR *s = _tcschr (tmp2, '\"');
					if (s && _tcslen (s) > 1) {
						TCHAR *s2 = my_strdup (s + 1);
						s = _tcschr (s2, '\"');
						if (s)
							*s = 0;
						romlist_add2 (s2, rd);
						xfree (s2);
					} else {
						romlist_add2 (tmp2, rd);
					}
				}
			}
		}
		idx++;
	}
	romlist_add (NULL, NULL);
	regclosetree (fkey);
}

/* driveclick_win32.cpp! */
int driveclick_pcdrivemask;

void set_path (const TCHAR *name, TCHAR *path, pathtype mode)
{
  TCHAR tmp[MAX_DPATH];

  DebOut("name %s, path %s, pathtype %d\n", name, path, mode);

  if (!path) {
    if (!_tcscmp (start_path_data, start_path_exe))
      _tcscpy (tmp, _T(".\\"));
    else
      _tcscpy (tmp, start_path_data);
    if (!_tcscmp (name, _T("KickstartPath")))
      _tcscat (tmp, _T("Roms"));
    if (!_tcscmp (name, _T("ConfigurationPath")))
      _tcscat (tmp, _T("Configurations"));
    if (!_tcscmp (name, _T("LuaPath")))
      _tcscat (tmp, _T("lua"));
    if (!_tcscmp (name, _T("ScreenshotPath")))
      _tcscat (tmp, _T("Screenshots"));
    if (!_tcscmp (name, _T("StatefilePath")))
      _tcscat (tmp, _T("Savestates"));
    if (!_tcscmp (name, _T("SaveimagePath")))
      _tcscat (tmp, _T("SaveImages"));
    if (!_tcscmp (name, _T("InputPath")))
      _tcscat (tmp, _T("Inputrecordings"));
  } else {
    _tcscpy (tmp, path);
  }
  stripslashes (tmp);
  DebOut("tmp: %s\n");
  if (!_tcscmp (name, _T("KickstartPath"))) {
#if 0
    DWORD v = GetFileAttributes (tmp);
    if (v == INVALID_FILE_ATTRIBUTES || !(v & FILE_ATTRIBUTE_DIRECTORY))
#endif
    if(!is_dir(tmp)) 
      get_rom_path (tmp, PATH_TYPE_DEFAULT);
    if (mode == PATH_TYPE_NEWAF) {
      get_rom_path (tmp, PATH_TYPE_NEWAF);
    } else if (mode == PATH_TYPE_AMIGAFOREVERDATA) {
      get_rom_path (tmp, PATH_TYPE_AMIGAFOREVERDATA);
    }
  }
  fixtrailing (tmp);
  fullpath (tmp, sizeof tmp / sizeof (TCHAR));
  DebOut("tmp: %s\n", tmp);
  regsetstr (NULL, name, tmp);
}

void set_path (const TCHAR *name, TCHAR *path) {

  DebOut("name %s, path %s\n", name, path);
  set_path (name, path, PATH_TYPE_DEFAULT);
}

static void initpath (const TCHAR *name, TCHAR *path) {

  if (regexists (NULL, name))
    return;
  set_path (name, NULL);
}


void create_afnewdir (int remove) {
#if 0
	TCHAR tmp[MAX_DPATH], tmp2[MAX_DPATH];
#endif

  DebOut("remove: %d\n", remove);

#if 0
	if (SUCCEEDED (SHGetFolderPath (NULL, CSIDL_COMMON_DOCUMENTS, NULL, 0, tmp))) {
		fixtrailing (tmp);
		_tcscpy (tmp2, tmp);
		_tcscat (tmp2, _T("Amiga Files"));
		_tcscpy (tmp, tmp2);
		_tcscat (tmp, _T("\\WinUAE"));
		if (remove) {
			if (GetFileAttributes (tmp) != INVALID_FILE_ATTRIBUTES) {
				RemoveDirectory (tmp);
				RemoveDirectory (tmp2);
			}
		} else {
			CreateDirectory (tmp2, NULL);
			CreateDirectory (tmp, NULL);
		}
	}
#endif


}


void setpathmode (pathtype pt)
{
	TCHAR pathmode[32] = { 0 };
	if (pt == PATH_TYPE_WINUAE)
		_tcscpy (pathmode, _T("WinUAE"));
	if (pt == PATH_TYPE_NEWWINUAE)
		_tcscpy (pathmode, _T("WinUAE_2"));
	if (pt == PATH_TYPE_NEWAF)
		_tcscpy (pathmode, _T("AmigaForever"));
	if (pt == PATH_TYPE_AMIGAFOREVERDATA)
		_tcscpy (pathmode, _T("AMIGAFOREVERDATA"));
	regsetstr (NULL, _T("PathMode"), pathmode);
}

static int parseversion (TCHAR **vs)
{
	TCHAR tmp[10];
	int i;

	i = 0;
	while (**vs >= '0' && **vs <= '9') {
		if (i >= sizeof (tmp) / sizeof (TCHAR))
			return 0;
		tmp[i++] = **vs;
		(*vs)++;
	}
	if (**vs == '.')
		(*vs)++;
	tmp[i] = 0;
	return _tstol (tmp);
}

static int checkversion (TCHAR *vs)
{
	int ver;
	if (_tcslen (vs) < 10)
		return 0;
	if (_tcsncmp (vs, _T("WinUAE "), 7))
		return 0;
	vs += 7;
	ver = parseversion (&vs) << 16;
	ver |= parseversion (&vs) << 8;
	ver |= parseversion (&vs);
	if (ver >= ((UAEMAJOR << 16) | (UAEMINOR << 8) | UAESUBREV))
		return 0;
	return 1;
}

UAEREG *read_disk_history (int type);

void WIN32_HandleRegistryStuff (void) {
	RGBFTYPE colortype = RGBFB_NONE;
	//DWORD dwType = REG_DWORD;
	DWORD dwDisplayInfoSize = sizeof (colortype);
	int size;
	TCHAR path[MAX_DPATH] = _T("");
	TCHAR version[100];

	initpath (_T("FloppyPath"), start_path_data);
	initpath (_T("KickstartPath"), start_path_data);
	initpath (_T("hdfPath"), start_path_data);
	initpath (_T("ConfigurationPath"), start_path_data);
	initpath (_T("LuaPath"), start_path_data);
	initpath (_T("ScreenshotPath"), start_path_data);
	initpath (_T("StatefilePath"), start_path_data);
	initpath (_T("SaveimagePath"), start_path_data);
	initpath (_T("VideoPath"), start_path_data);
	initpath (_T("InputPath"), start_path_data);
#if 0
	if (!regexists (NULL, _T("MainPosX")) || !regexists (NULL, _T("GUIPosX"))) {
		int x = GetSystemMetrics (SM_CXSCREEN);
		int y = GetSystemMetrics (SM_CYSCREEN);
		x = (x - 800) / 2;
		y = (y - 600) / 2;
		if (x < 10)
			x = 10;
		if (y < 10)
			y = 10;
		/* Create and initialize all our sub-keys to the default values */
		regsetint (NULL, _T("MainPosX"), x);
		regsetint (NULL, _T("MainPosY"), y);
		regsetint (NULL, _T("GUIPosX"), x);
		regsetint (NULL, _T("GUIPosY"), y);
	}
#endif
	size = sizeof (version) / sizeof (TCHAR);
	if (regquerystr (NULL, _T("Version"), version, &size)) {
		if (checkversion (version))
			regsetstr (NULL, _T("Version"), VersionStr);
	} else {
		regsetstr (NULL, _T("Version"), VersionStr);
	}
	size = sizeof (version) / sizeof (TCHAR);
	if (regquerystr (NULL, _T("ROMCheckVersion"), version, &size)) {
		if (checkversion (version)) {
			if (regsetstr (NULL, _T("ROMCheckVersion"), VersionStr))
				forceroms = 1;
		}
	} else {
		if (regsetstr (NULL, _T("ROMCheckVersion"), VersionStr))
			forceroms = 1;
	}

#if 0
	regqueryint (NULL, _T("DirectDraw_Secondary"), &ddforceram);
	if (regexists (NULL, _T("SoundDriverMask"))) {
		regqueryint (NULL, _T("SoundDriverMask"), &sounddrivermask);
	} else {
		sounddrivermask = 3;
		regsetint (NULL, _T("SoundDriverMask"), sounddrivermask);
	}
#endif
	if (regexists (NULL, _T("ConfigurationCache")))
		regqueryint (NULL, _T("ConfigurationCache"), &configurationcache);
	else
		regsetint (NULL, _T("ConfigurationCache"), configurationcache);
	if (regexists (NULL, _T("RelativePaths")))
		regqueryint (NULL, _T("RelativePaths"), &relativepaths);
	else
		regsetint (NULL, _T("RelativePaths"), relativepaths);
	regqueryint (NULL, _T("QuickStartMode"), &quickstart);
#if 0
	reopen_console ();
#endif
	fetch_path (_T("ConfigurationPath"), path, sizeof (path) / sizeof (TCHAR));
	path[_tcslen (path) - 1] = 0;
	if (GetFileAttributes (path) == 0xffffffff) {
		TCHAR path2[MAX_DPATH];
		_tcscpy (path2, path);
		createdir (path);
		_tcscat (path, _T("\\Host"));
		createdir (path);
		_tcscpy (path, path2);
		_tcscat (path, _T("\\Hardware"));
		createdir (path);
	}
	fetch_path (_T("StatefilePath"), path, sizeof (path) / sizeof (TCHAR));
	createdir (path);
	_tcscat (path, _T("default.uss"));
	_tcscpy (savestate_fname, path);
	fetch_path (_T("InputPath"), path, sizeof (path) / sizeof (TCHAR));
	createdir (path);
	regclosetree (read_disk_history (HISTORY_FLOPPY));
	regclosetree (read_disk_history (HISTORY_CD));
#if 0
	associate_init_extensions ();
#endif
	read_rom_list ();
	load_keyring (NULL, NULL);
}

