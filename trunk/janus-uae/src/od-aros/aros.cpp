
//#define JUAE_DEBUG

#include <proto/dos.h>
#include <proto/timer.h>
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

#include <proto/processor.h>
#include <resources/processor.h>

#include <SDL/SDL.h>


#include "sysconfig.h"
#include "sysdeps.h"

//#include "resource"

//#include <wintab.h>
//#include "wintablet.h"
//#include <pktdef.h>

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
//#include "muigui.h"
#include "autoconf.h"
#include "gui/gui_mui.h"
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
#include "rtgmodes.h"

#include "fsdb.h"
#include "winnt.h"

//#include "options.h"
//#include "include/scsidev.h"

#include "od-aros/tchar.h"
#include "gui/fs.h"

APTR ProcessorBase = NULL;

struct Window *hAmigaWnd;
/* not really used: */
HWND hHiddenWnd;
HMODULE hUIDLL = NULL;
HMODULE hInst = NULL;

/* visual C++ symbols ..*/

int _daylight=0; 
int _dstbias=0 ;
//long _timezone=0; 
char tzname[2][4]={"PST", "PDT"};

/* win32.cpp stuff */
int pissoff_value = 25000;

int log_scsi;
int log_net;

int os_admin, os_64bit, os_win7, os_vista, cpu_number, os_touch;

/* missing symbols from various od-win32 sources */
int tablet_log=0;
int seriallog =0;
int max_uae_width;
int max_uae_height;
int log_vsync, debug_vsync_min_delay, debug_vsync_forced_delay;
int extraframewait;
int recursiveromscan = 0;
int saveimageoriginalpath = 0;
/* end */

int pause_emulation;
int sleep_resolution;
int uaelib_debug;

int mouseactive=0;

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

static struct  contextcommand cc_disk[] = {
	{ _T("A500"), _T("-0 \"%1\" -s use_gui=no -cfgparam=quickstart=A500,0"), IDI_DISKIMAGE },
	{ _T("A1200"), _T("-0 \"%1\" -s use_gui=no -cfgparam=quickstart=A1200,0"), IDI_DISKIMAGE },
	{ NULL }
};
struct assext exts_gui[] = {
	{ _T(".uae"), _T("-f \"%1\""), _T("WinUAE configuration file"), IDI_CONFIGFILE, NULL },
	{ _T(".adf"), _T("-0 \"%1\" -s use_gui=no"), _T("WinUAE floppy disk image"), IDI_DISKIMAGE, cc_disk },
	{ _T(".adz"), _T("-0 \"%1\" -s use_gui=no"), _T("WinUAE floppy disk image"), IDI_DISKIMAGE, cc_disk },
	{ _T(".dms"), _T("-0 \"%1\" -s use_gui=no"), _T("WinUAE floppy disk image"), IDI_DISKIMAGE, cc_disk },
	{ _T(".fdi"), _T("-0 \"%1\" -s use_gui=no"), _T("WinUAE floppy disk image"), IDI_DISKIMAGE, cc_disk },
	{ _T(".ipf"), _T("-0 \"%1\" -s use_gui=no"), _T("WinUAE floppy disk image"), IDI_DISKIMAGE, cc_disk },
	{ _T(".uss"), _T("-s statefile=\"%1\" -s use_gui=no"), _T("WinUAE statefile"), IDI_APPICON, NULL },
	{ NULL }
};

static void createdir (const TCHAR *path) {
  CreateDir(path);
}

bool is_dir(const TCHAR *name) {

  struct FileInfoBlock *fib;
  BPTR file=NULL;
  bool ret=FALSE;

  bug("[JUAE:AROS] %s('%s')\n", __PRETTY_FUNCTION__, name);

  fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, TAG_END);
  if(!fib) 
  {
    bug("[JUAE:AROS] %s: no FileInfoBlock!\n", __PRETTY_FUNCTION__);
    goto NODIR;
  }

  file=Lock(name, SHARED_LOCK);
  if(!file) 
  {
    bug("[JUAE:AROS] %s: no lock..\n", __PRETTY_FUNCTION__);
    goto NODIR;
  }

  if (!Examine(file, fib)) 
  {
    bug("[JUAE:AROS] %s: Examine failed\n", __PRETTY_FUNCTION__);
    goto NODIR;
  }

  if(fib->fib_DirEntryType <0) 
  {
    bug("[JUAE:AROS] %s: FILE!\n", __PRETTY_FUNCTION__);
    goto NODIR;
  }
  else 
  {
    bug("[JUAE:AROS] %s: directory..\n", __PRETTY_FUNCTION__);
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
  fetch_path (_T("ConfigurationPath"), out, size);
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


void fetch_path (const TCHAR *name, TCHAR *out, int size) {
  TCHAR tmp[256];
  int old;

  DebOut("name: %s, size %d\n", name, size);
  out[0]=0;

  if (!name) {
    _tcscpy (out, start_path_data);
    return;
  }

  if (!_tcscmp (name, _T("FloppyPath")))
    _tcscat (out, _T("PROGDIR:adf"));
  else if (!_tcscmp (name, _T("CDPath")))
    _tcscat (out, _T("PROGDIR:cd"));
  else if (!_tcscmp (name, _T("TapePath")))
    _tcscat (out, _T("PROGDIR:tape/"));
  else if (!_tcscmp (name, _T("hdfPath")))
    _tcscat (out, _T("PROGDIR:hdf"));
  else if (!_tcscmp (name, _T("KickstartPath")))
    _tcscat (out, _T("PROGDIR:Roms/"));
  else if (!_tcscmp (name, _T("ConfigurationPath")))
    _tcscat (out, _T("PROGDIR:Configurations/"));
  else if (!_tcscmp (name, _T("LuaPath")))
    _tcscat (out, _T("PROGDIR:lua"));
  else if (!_tcscmp (name, _T("StatefilePath")))
    _tcscat (out, _T("PROGDIR:Savestates"));
  else if (!_tcscmp (name, _T("InputPath")))
    _tcscat (out, _T("PROGDIR:Inputrecordings/"));
  else {
    _tcscpy (out, start_path_data);
    return;
  }

  if (start_data >= 0)
    regquerystr (NULL, name, tmp, &size); 

  /* somehow I though it would be a good idea, to strip '.\', that WinUAE uses */
  if(tmp[0]=='.' && tmp[1]=='\\') {
    strcpy(out, tmp+2);
  }
  else {
    strcpy(out, tmp);
  }

  DebOut("out: >%s<\n", out);

  if (GetFileAttributes (out) == INVALID_FILE_ATTRIBUTES) {
    DebOut("directory %s was invalid ..\n", out);

    /* try again */
    out[0]=0;
    if (!_tcscmp (name, _T("FloppyPath")))
      _tcscat (out, _T("PROGDIR:adf"));
    else if (!_tcscmp (name, _T("CDPath")))
      _tcscat (out, _T("PROGDIR:cd"));
    else if (!_tcscmp (name, _T("TapePath")))
      _tcscat (out, _T("PROGDIR:tape\\"));
    else if (!_tcscmp (name, _T("hdfPath")))
      _tcscat (out, _T("PROGDIR:hdf"));
    else if (!_tcscmp (name, _T("KickstartPath")))
      _tcscat (out, _T("PROGDIR:Roms/"));
    else if (!_tcscmp (name, _T("ConfigurationPath")))
      _tcscat (out, _T("PROGDIR:Configurations\\"));
    else if (!_tcscmp (name, _T("LuaPath")))
      _tcscat (out, _T("PROGDIR:lua"));
    else if (!_tcscmp (name, _T("StatefilePath")))
      _tcscat (out, _T("PROGDIR:Savestates"));
    else if (!_tcscmp (name, _T("InputPath")))
      _tcscat (out, _T("PROGDIR:Inputrecordings\\"));

    DebOut(".. try default %s\n", out);

    if (GetFileAttributes (out) == INVALID_FILE_ATTRIBUTES) {
      /* still not ok !? */
      DebOut("default directory %s was invalid, too, use %s\n", out, start_path_data);
      _tcscpy (out, start_path_data);
    }
  }

  stripslashes (out);
  fixtrailing (out);

  /* please, no relative paths here.. */
  old=relativepaths;
  relativepaths=0;
  fullpath (out, size);
  relativepaths=old;
}

/* resides in win32.cpp normally
 *
 * loads amigaforever keyfiles ..?
 *
 * FS-UAE also has this as a pure stub
 */
uae_u8 *target_load_keyfile (struct uae_prefs *p, const TCHAR *path, int *sizep, TCHAR *name) {
  bug("[JUAE:AROS] %s('%s',%d,'%s')\n", __PRETTY_FUNCTION__, path, *sizep, name);
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

/* WinUAE converts path to absolute or relative 
 *
 * As relative paths always somehow fail on AROS, we only use absolute paths here
 */
void fullpath(TCHAR *inpath, int size) {
  char    abs_in  [MAX_DPATH];
  char    tmp     [MAX_DPATH];
  TCHAR  *path = inpath;
  BPTR    lock;
  BOOL    result;

  bug("[JUAE:AROS] %s('%s', %d)\n", __PRETTY_FUNCTION__, path, size);

  if(path[0] == '.' && (path[1] == '\\' || path[1] == '/')) {
    /* skip .\ or ./ */
    path=path+2;
  }

  bug("[JUAE:AROS] %s: using : '%s'\n", __PRETTY_FUNCTION__, path);

  tmp[0]     =(char) 0;
  abs_in[0]  =(char) 0;

  /* is input path relative? */
  if(!strchr(path, ':')) {
    strcpy(tmp, "PROGDIR:");
  }
  AddPart(tmp, path, MAX_DPATH);
  //strcat(tmp, path);
  DebOut("tmp: %s\n", tmp);

  bug("[JUAE:AROS] %s: ------------> '%s'\n", __PRETTY_FUNCTION__, tmp);

  lock=Lock(tmp, SHARED_LOCK);
  if(!lock) {
    bug("[JUAE:AROS] %s: failed to lock path '%s'\n", __PRETTY_FUNCTION__, tmp);
    return;
  }

  result=NameFromLock(lock, abs_in, MAX_DPATH);
  UnLock(lock);
  if(!result) {
    bug("[JUAE:AROS] %s: NameFromLock('%s') failed!\n", __PRETTY_FUNCTION__, tmp);
    return;
  }
  DebOut("NameFromLock(%s): %s\n", tmp, abs_in);
  /* now we have an absolute path!  */

  strcpy(inpath, abs_in);

  DebOut("result: >%s<\n", inpath);
}

/***************************************************************************************************
 * GetFullPathName
 *
 * see https://msdn.microsoft.com/en-us/library/windows/desktop/aa364963%28v=vs.85%29.aspx
 *
 *  lpFileName: input file name
 *  nBufferLength: size of the buffer to receive the null-terminated string for the drive and path
 *  lpBuffer: A pointer to a buffer that receives the null-terminated string for the drive and path.
 *  lpFilePart: A pointer to a buffer that receives the address (within lpBuffer) 
 *              of the final file name component in the path.
 *              This parameter can be NULL.
 *              if lpBuffer refers to a directory and not a file, lpFilePart receives zero.
 *
 * If the function succeeds, the return value is the length, in TCHARs, of the string copied to 
 * lpBuffer, not including the terminating null character.
 * If the lpBuffer buffer is too small to contain the path, the return value is the size, 
 *  in TCHARs, of the buffer that is required to hold the path and the terminating null character.
 *
 * If the function fails for any other reason, the return value is zero.
 ********************************************************************************************************/

DWORD GetFullPathName(const TCHAR *lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart) {

  TCHAR tmp_path[MAX_DPATH];
  TCHAR abs[MAX_DPATH];
  BPTR  lock;
  BOOL r;

  DebOut("lpFileName:%s\n", lpFileName);

  abs[0]=(char) 0;

  /* is input path relative? */
  if(!strchr(lpFileName, ':')) {
    strcpy(abs, "PROGDIR:");
  }
  strcat(abs, lpFileName);
  DebOut("abs: %s\n", abs);

  lock=Lock(abs, SHARED_LOCK);
  if(!lock) {
    DebOut("Lock('%s') failed!\n", abs);
    return 0;
  }

  r=NameFromLock(lock, tmp_path, MAX_DPATH);
  UnLock(lock);
  if(!r) {
    DebOut("NameFromLock('%s') failed!\n", abs);
    return 0;
  }
  DebOut("NameFromLock(%s): %s\n", lpFileName, abs);
  /* now we have an absolute path!  */

  if(lpBuffer==NULL || strlen(tmp_path)+1>nBufferLength) {
    DebOut("WARNING: lpBuffer is too small!?\n");
    return strlen(tmp_path)+1; /* add terminating null byte */
  }

  strcpy(lpBuffer, tmp_path);

  if(is_dir(lpBuffer)) {
    lpFilePart=NULL;
  }
  else {
    lpFilePart=(LPTSTR *) FilePart(lpBuffer);
  }

  DebOut("return: %s\n", lpBuffer);

  return strlen(lpFileName)+1;
}

/* taken from puae/misc.c */

static struct MultiDisplay *getdisplay2 (struct uae_prefs *p, int index)
{
#ifndef __AROS__
  int display = index < 0 ? p->gfx_apmode[screen_is_picasso ? APMODE_RTG : APMODE_NATIVE].gfx_display - 1 : index;
#endif

  write_log ("Multimonitor detection disabled\n");
  Displays[0].primary  = TRUE;
  Displays[0].adaptername     = (TCHAR *) "Display";
  Displays[0].monitorname     = (TCHAR *) "Display";
  //Displays[0].disabled = 0;
  if(Displays[0].DisplayModes == NULL) {
    bug("[JUAE:AROS] %s: alloc DisplayModes array. Should this be done here !? really !?\n", __PRETTY_FUNCTION__);
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
    bug("[JUAE:AROS] %s: \nERROR: win is NULL\n", __PRETTY_FUNCTION__);
    write_log("ERROR: win is NULL\n");
    return 0;
  }

  scr=win->WScreen;

  if(!scr) {
    bug("[JUAE:AROS] %s: \nERROR: win->WScreen is NULL\n", __PRETTY_FUNCTION__);
    write_log("\nERROR: win->WScreen is NULL\n");
    return 0;
  }

  if(!GetCyberMapAttr(scr->RastPort.BitMap, CYBRMATTR_ISCYBERGFX)) {
    bug("[JUAE:AROS] %s: \nERROR: !CYBRMATTR_ISCYBERGFX\n", __PRETTY_FUNCTION__);
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

/* milli sec is 1/1000 sec
 * delay waits for 1/50 sec
 *
 * As SDL provides millisecond timing, we us it, if <50ms are requested
 *
 * fs-uae authors also have no idea, what "main" means here..
 */
static void sleep_millis2 (int ms, bool main) {

    if(ms<50)
    {
        SDL_Delay(ms);
        return;
    }

    Delay(ms/20);
}

void sleep_millis_main (int ms) {

  sleep_millis2 (ms, true);
}

void sleep_millis (int ms) {

  sleep_millis2 (ms, false);
}


void target_restart (void)
{
    bug("[JUAE:AROS] %s()\n", __PRETTY_FUNCTION__);
  gui_restart ();
}


void fetch_rompath (TCHAR *out, int size)
{
    bug("[JUAE:AROS] %s()\n", __PRETTY_FUNCTION__);
  fetch_path (_T("KickstartPath"), out, size);
}

//static bool rawinput_enabled_mouse;
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
    bug("[JUAE:AROS] %s()\n", __PRETTY_FUNCTION__);
}


void unprotect_maprom (void) {
    bug("[JUAE:AROS] %s()\n", __PRETTY_FUNCTION__);
}
#endif


/* Loads a string resource from the executable file associated 
 * with a specified module, copies the string into a buffer, and 
 * appends a terminating null character.
 */

extern char *STRINGTABLE[];

int LoadString(APTR hInstance, uint32_t uID, TCHAR * lpBuffer, int nBufferMax) {
    int len = 0;
    bug("[JUAE:AROS] %s(uId %d)\n", __PRETTY_FUNCTION__, uID);
    if(uID != -1)
    {
      bug("[JUAE] %s: copying '%s' to buffer @ 0x%p\n", __PRETTY_FUNCTION__, STRINGTABLE[uID], lpBuffer);

      strncpy(lpBuffer, STRINGTABLE[uID], nBufferMax);
      len = strlen(lpBuffer)-1;
    }
    return len;
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

  struct mystat statbuf;
  int result=0;

  DebOut("path: %s\n", path);
  if(!my_stat(path, &statbuf)) {
    DebOut("unable to lock path %s\n", path);
    return INVALID_FILE_ATTRIBUTES;
  }

  if(is_dir(path)) {
    DebOut("FILE_ATTRIBUTE_DIRECTORY\n");
    return FILE_ATTRIBUTE_DIRECTORY;
  }

  if(my_isfilehidden(path)) {
    DebOut("FILE_ATTRIBUTE_HIDDEN\n");
    result=result|FILE_ATTRIBUTE_HIDDEN;
  }

  if(!(statbuf.mode && FILEFLAG_WRITE)) {
    DebOut("FILE_ATTRIBUTE_READONLY");
    result=result|FILE_ATTRIBUTE_READONLY;
  }

  if(result==0) {
    DebOut("FILE_ATTRIBUTE_NORMAL\n");
    result=FILE_ATTRIBUTE_NORMAL;
  }

  return result;
}

typedef struct {
  struct FileLock *lock;
  struct FileInfoBlock fib_ptr;
  char wildcard_tokens[1024];
} INTERNAL_HANDLE;

/* convert time_t to/from AmigaDOS time */
static const uae_s64 msecs_per_day = 24 * 60 * 60 * 1000;
static const uae_s64 diff = ((8 * 365 + 2) * (24 * 60 * 60)) * (uae_u64)1000;

void amiga_to_timeval (struct mytimeval *tv, int days, int mins, int ticks)
{
	uae_s64 t;

	if (days < 0)
		days = 0;
	if (days > 9900 * 365)
		days = 9900 * 365; // in future far enough?
	if (mins < 0 || mins >= 24 * 60)
		mins = 0;
	if (ticks < 0 || ticks >= 60 * 50)
		ticks = 0;

	t = ticks * 20;
	t += mins * (60 * 1000);
	t += ((uae_u64)days) * msecs_per_day;
	t += diff;

	tv->tv_sec = t / 1000;
	tv->tv_usec = (t % 1000) * 1000;
}

BOOL FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData) {

  INTERNAL_HANDLE *h=(INTERNAL_HANDLE*) hFindFile;
  char full_path[256];

next:
  if(!ExNext((BPTR) h->lock, &h->fib_ptr)) {
    DebOut("no more files ..\n");
    return FALSE;
  }

  if(h->wildcard_tokens[0]) {
    if(!MatchPatternNoCase(h->wildcard_tokens, (const char *) h->fib_ptr.fib_FileName)) {
      DebOut("no wildcard match!\n");
      goto next;
    }
  }

  strncpy(lpFindFileData->cFileName, (const char *)h->fib_ptr.fib_FileName, MAX_PATH-1);
  DebOut("lpFindFileData->cFileName: %s\n", lpFindFileData->cFileName);

  amiga_to_timeval(&lpFindFileData->ftCreationTime, h->fib_ptr.fib_Date.ds_Days, h->fib_ptr.fib_Date.ds_Minute, h->fib_ptr.fib_Date.ds_Tick);
  amiga_to_timeval(&lpFindFileData->ftLastAccessTime, h->fib_ptr.fib_Date.ds_Days, h->fib_ptr.fib_Date.ds_Minute, h->fib_ptr.fib_Date.ds_Tick);
  amiga_to_timeval(&lpFindFileData->ftLastWriteTime, h->fib_ptr.fib_Date.ds_Days, h->fib_ptr.fib_Date.ds_Minute, h->fib_ptr.fib_Date.ds_Tick);

  NameFromLock(h->lock, full_path, 255);
  DebOut("full_path: %s\n", full_path);
  AddPart(full_path, lpFindFileData->cFileName, 255);
  DebOut("full_path: %s\n", full_path);
  lpFindFileData->dwFileAttributes=GetFileAttributes(full_path);
  DebOut("lpFindFileData->dwFileAttributes: %lx\n", lpFindFileData->dwFileAttributes);

  return TRUE;
}

/* Searches a directory for a file or subdirectory with a 
 * name that matches a specific name (or partial name if wildcards are used).
 *
 * lpFindFileData [out]:
 * A pointer to the WIN32_FIND_DATA structure that receives information about 
 * a found file or directory.
 */
HANDLE FindFirstFile(const TCHAR *lpFileName, WIN32_FIND_DATA *lpFindFileData) {
  
  char wildcard[256];
  char path[256];
  INTERNAL_HANDLE *h=NULL;

  memset(lpFindFileData, 0, sizeof(WIN32_FIND_DATA));

  DebOut("lpFileName: %s\n", lpFileName);
  wildcard[0]=(char) 0;
  path[0]=(char) 0;
  h=(INTERNAL_HANDLE *) AllocVec(sizeof(INTERNAL_HANDLE), MEMF_CLEAR);
  h->wildcard_tokens[0]=(char) 0;

  if(is_dir(lpFileName)) {
    DebOut("lpFileName is a directory\n");
    strncpy(path, lpFileName, 255);
  }
  else {
    strncpy(wildcard, FilePart(lpFileName), 255);
    DebOut("wildcard: %s\n", wildcard);
    strncpy(path, lpFileName, strlen(lpFileName)-strlen(wildcard));
    path[strlen(lpFileName)-strlen(wildcard)]=(char) 0;
    ParsePatternNoCase(wildcard, h->wildcard_tokens, 1023);
  }

  DebOut("path: %s\n", path);

  h->lock=(struct FileLock *) Lock((STRPTR) path, ACCESS_READ);
  if(!h->lock) {
    DebOut("path %s is not valid directory!\n", path);
    goto error;
  }

  if(!Examine((BPTR) h->lock, &h->fib_ptr)) {
    UnLock((BPTR) h->lock);
    h->lock=NULL;
    goto error;
  }

  if(h->fib_ptr.fib_DirEntryType==0) {
    DebOut("no directory!?\n");
    TODO();
    goto error;
  }

  if(FindNextFile(h, lpFindFileData)) {
    return (HANDLE) h;
  }

error:
  FindClose(h);
  DebOut("return NULL\n");
  return NULL;
}

BOOL FindClose(HANDLE hFindFile) {

  INTERNAL_HANDLE *h=(INTERNAL_HANDLE*) hFindFile;

  DebOut("hFindFile: %lx\n", h);

  if(!h) {
    return TRUE;
  }

  if(h->lock) {
    UnLock(h->lock);
    h->lock=NULL;
  }

  if(h) {
    FreeVec(h);
    h=NULL;
  }

  return TRUE;
}


int get_rom_path (TCHAR *out, pathtype mode)
{
  TCHAR tmp[MAX_DPATH];

  bug("[JUAE:AROS] %s(%d)\n", __PRETTY_FUNCTION__, mode);
  bug("[JUAE:AROS] %s: start_path_data: %s\n", __PRETTY_FUNCTION__, start_path_data);
  bug("[JUAE:AROS] %s: start_path_exe: %s\n", __PRETTY_FUNCTION__, start_path_exe);

  tmp[0] = 0;
  switch (mode)
  {
  case PATH_TYPE_DEFAULT:
    {
      bug("[JUAE:AROS] %s: PATH_TYPE_DEFAULT\n", __PRETTY_FUNCTION__);
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
      bug("[JUAE:AROS] %s: PATH_TYPE_NEWAF\n", __PRETTY_FUNCTION__);
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
      bug("[JUAE:AROS] %s: PATH_TYPE_AMIGAFOREVERDATA\n", __PRETTY_FUNCTION__);
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
    bug("[JUAE:AROS] %s: default: return -1\n", __PRETTY_FUNCTION__);
    return -1;
  }
  if (isfilesindir (tmp)) {
    _tcscpy (out, tmp);
    fixtrailing (out);
  }
  if (out[0]) {
    fullpath (out, MAX_DPATH);
  }
  bug("[JUAE:AROS] %s: result: %s\n", __PRETTY_FUNCTION__, out);
  return out[0] ? 1 : 0;
}

int regexiststree (UAEREG *root, const TCHAR *name);
UAEREG *regcreatetree (UAEREG *root, const TCHAR *name);
int regenumstr (UAEREG *root, int idx, TCHAR *name, int *nsize, TCHAR *str, int *size);
void regclosetree (UAEREG *key);
int getregmode (void);


static void romlist_add2 (const TCHAR *path, struct romdata *rd)
{
  bug("[JUAE:AROS] %s('%s':0x%p)\n", __PRETTY_FUNCTION__, path, rd);

  if (getregmode ()) {
    int ok = 0;
    TCHAR tmp[MAX_DPATH];
    if (path[0] == '/' || path[0] == '\\')
      ok = 1;
    if (_tcslen (path) > 1 && path[_tcslen (path)] == ':') // "foo:", but not just ":"
      ok = 1;
    if (!ok) {
      _tcscpy (tmp, start_path_exe);
      myAddPart(tmp, path, MAX_DPATH);
      DebOut("tmp: %s\n", tmp);
      romlist_add (tmp, rd);
      return;
    }
  }
  DebOut("path: %s\n", path);
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

  bug("[JUAE:AROS] %s()\n", __PRETTY_FUNCTION__);

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
    DebOut("idx: %d\n", idx);
    if (!regenumstr (fkey, idx, tmp, &size, tmp2, &size2)) {
      DebOut("BREAK!\n");
      break;
    }

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

    bug("[JUAE:AROS] %s('%s', '%s', %d)\n", __PRETTY_FUNCTION__, name, path, mode);

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
  bug("[JUAE:AROS] %s: path '%s'\n", __PRETTY_FUNCTION__, tmp);
  regsetstr (NULL, name, tmp);
}

void set_path (const TCHAR *name, TCHAR *path)
{
    bug("[JUAE:AROS] %s('%s', '%s')\n", __PRETTY_FUNCTION__, name, path);

    set_path (name, path, PATH_TYPE_DEFAULT);
}

static void initpath (const TCHAR *name, TCHAR *path)
{
    bug("[JUAE:AROS] %s('%s', '%s')\n", __PRETTY_FUNCTION__, name, path);

    if (regexists (NULL, name)) {
      DebOut("name %s exists in registry\n", name);
      return;
    }
    DebOut("not in registry: %s\n", name);
    set_path (name, NULL);
}

void create_afnewdir (int remove) {
#if 0
  TCHAR tmp[MAX_DPATH], tmp2[MAX_DPATH];
#endif

    bug("[JUAE:AROS] %s(%d)\n", __PRETTY_FUNCTION__, remove);

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
  unsigned int i;

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
  //RGBFTYPE colortype = RGBFB_NONE;
  //DWORD dwType = REG_DWORD;
  //DWORD dwDisplayInfoSize = sizeof (colortype);
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
#ifndef __AROS__
  /* AROS paths have no trailing \\ */
  path[_tcslen (path) - 1] = 0;
#endif
  if (GetFileAttributes (path) == ~0) {
    TCHAR path2[MAX_DPATH];
    _tcscpy (path2, path);
    createdir (path);
    _tcscat (path, _T("/Host"));
    createdir (path);
    _tcscpy (path, path2);
    _tcscat (path, _T("/Hardware"));
    createdir (path);
  }
  fetch_path (_T("StatefilePath"), path, sizeof (path) / sizeof (TCHAR));
  createdir (path);
  TODO(); /*tcscat -> AddPart ? */
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

/* 
 * ATT: o1i: please check this. For my hosted env this gives reasonable MHz value .. 
 *
 * WinUAE falls back to the same trick, if the OS does not report the CPU speed,
 * even if it is not really reliable IMHO
 */
static int freqset=0;

static void figure_processor_speed_rdtsc (void)
{
  frame_time_t clockrate;
	BYTE oldpri;

	if (freqset)
		return;

	freqset = 1;
  /* make us highest priority */
  oldpri=SetTaskPri(FindTask(NULL), 125);
	clockrate=read_processor_time();
	sleep_millis(500);
	clockrate=(read_processor_time() - clockrate) * 2;
  SetTaskPri(FindTask(NULL), oldpri);

  /* hmm ?? */
  clockrate=clockrate*64;

	write_log (_T("CLOCKFREQ: RDTSC %.2fMHz\n"), clockrate / 1000000.0);
  DebOut("CLOCKFREQ: RDTSC %6.2f MHz\n", clockrate / 1000000.0);
	syncbase = clockrate >> 6;
}

void figure_processor_speed (void)
{
    UQUAD cpuspeed;
    struct TagItem tags [] = { {GCIT_SelectedProcessor, 0},
                               {GCIT_ProcessorSpeed, (IPTR)&cpuspeed},
                               {TAG_DONE, TAG_DONE} };

    if (freqset)
        return;

    ProcessorBase=OpenResource(PROCESSORNAME);
    if(ProcessorBase)
    {
        GetCPUInfo(tags);
        if(cpuspeed)
        {
            write_log (_T("CLOCKFREQ: AROS %.2fMHz\n"), cpuspeed / 1000000.0);
            syncbase = cpuspeed >> 6;
            freqset = 1;
            return;
        }
    }

    write_log(_T("CLOCKFREQ: unable to get cpu speed with AROS API\n"));
    return figure_processor_speed_rdtsc();
}

/* 
 * read_processor_time
 *
 * Return a "compressed" 32-bit unsigned integer generated from the 64-bit rdtsc 
 * value. read_processor_time works both on 32bit and 64bit intel AROS
 *
 * See also: rdtsc
 *   Read the number of CPU cycles since last reset.
 *   TSC is a 64-bit register present on all x86 processors since the Pentium. 
 */
frame_time_t read_processor_time (void) {

  unsigned long tsc_hi, tsc_lo=0;

//#if defined(i386) || defined(x86_64)
  asm volatile ("cpuid\n"
                "rdtsc" : "=a" (tsc_lo), "=d" (tsc_hi));

  //bug("[JUAE:RPT] tsc:  0x%08lx %08lx\n", tsc_hi, tsc_lo);

	tsc_lo >>= 6;            /* create space in 6 highest bits and       */
	tsc_lo |= tsc_hi << 26;  /* copy the 6 lower bits from tsc_hi there */
//#endif

	if (!tsc_lo)             /* never return 0 */
		tsc_lo++;

  //bug("[JUAE:RPT] result:        0x%08lx\n", tsc_lo);

  return tsc_lo;
}

/*
 * ShellExecute
 *
 * simulate Windows call 
 * ShellExecute: Performs an operation on a specified file.
 *
 * We use multiview here. But this will *not* handle all cases!
 */
void *ShellExecute(HWND hwnd, TCHAR *lpOperation, TCHAR *lpFile, TCHAR *lpParameters, TCHAR *lpDirectory, int nShowCmd) {
  
  char cmd[1024];
  char arg[1024];

  DebOut("lpOperation %s, lpFile %s, lpParameters %s, lpDirectory %s, nShowCmd %d\n", lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);

  /* lpOperation==NULL or "open" should work */
  if(lpOperation && strcmp(lpOperation, "open")) {
    DebOut("ERROR: unknow command >%s< for ShellExecute!\n", lpOperation);
    return NULL;
  }

  snprintf(arg, 1023, "%s", lpFile); /* yeah, strncpy, but this seems to be not reliable !? */
  fullpath(arg, 1023);

  snprintf(cmd, 1023, "multiview \"%s\"", arg);

  DebOut("execute >%s<\n", cmd);

  Execute(cmd, NULL, Output());

  return NULL;
}

/*
 * GetTempPath
 *
 * simulate Windows call
 * GetTempPath: retrieves the path of the directory designated for temporary files.
 *
 * If the function succeeds, the return value is the length, in TCHARs, of the 
 * string copied to lpBuffer, not including the terminating null character.
 */
DWORD GetTempPath(DWORD nBufferLength, TCHAR *lpBuffer) {

  snprintf(lpBuffer, nBufferLength-1, "T:");
  return 2;
}

static int pausemouseactive;
void resumesoundpaused (void)
{
	resume_sound ();
#ifdef AHI
	ahi_open_sound ();
	ahi2_pause_sound (0);
#endif
}
void setsoundpaused (void)
{
	pause_sound ();
#ifdef AHI
	ahi_close_sound ();
	ahi2_pause_sound (1);
#endif
}
bool resumepaused (int priority)
{
	//write_log (_T("resume %d (%d)\n"), priority, pause_emulation);
	if (pause_emulation > priority)
		return false;
	if (!pause_emulation)
		return false;
	resumesoundpaused ();
	blkdev_exitgui ();
	if (pausemouseactive)
		setmouseactive (-1);
	pausemouseactive = 0;
	pause_emulation = 0;
#ifdef RETROPLATFORM
	rp_pause (pause_emulation);
#endif
	setsystime ();
	return true;
}
bool setpaused (int priority)
{
	//write_log (_T("pause %d (%d)\n"), priority, pause_emulation);
	if (pause_emulation > priority)
		return false;
	pause_emulation = priority;
	setsoundpaused ();
	blkdev_entergui ();
	pausemouseactive = 1;
	if (isfullscreen () <= 0) {
		pausemouseactive = mouseactive;
		setmouseactive (0);
	}
#ifdef RETROPLATFORM
	rp_pause (pause_emulation);
#endif
	return true;
}


static int focus;

int mouseinside;

int isfocus (void)
{
	if (isfullscreen () > 0)
		return 2;
	if (currprefs.input_tablet >= TABLET_MOUSEHACK && currprefs.input_magic_mouse) {
		if (mouseinside)
			return 2;
		if (focus)
			return 1;
		return 0;
	}
	if (focus && mouseactive > 0)
		return 2;
	if (focus)
		return -1;
	return 0;
}

bool get_plugin_path (TCHAR *out, int len, const TCHAR *path) {
  TODO();

  if(path != NULL) {
    strncpy(out, path, len);
  }
  else {
    strncpy(out, "::TODO::", len);
  }

  return false;

}
