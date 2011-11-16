
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
#include "picasso96.h"
//#include "bsdsocket.h"
#include "aros.h"
//#include "arosgfx.h"
//#include "registry.h"
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


//#include "options.h"
//#include "include/scsidev.h"

#include "od-aros/tchar.h"

extern struct Window *hAmigaWnd;

/* visual C++ symbols ..*/

int _daylight=0; 
int _dstbias=0 ;
long _timezone=0; 
char tzname[2][4]={"PST", "PDT"};

/* win32.cpp stuff */
int pissoff_value = 25000;

int log_scsi;
int log_net;

int pause_emulation;
int sleep_resolution;
int uaelib_debug;

int quickstart = 1, configurationcache = 1, relativepaths = 0; 

void fetch_configurationpath (TCHAR *out, int size)
{
	strncpy(out, "configurations/", size);
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
 */
uae_u8 *target_load_keyfile (struct uae_prefs *p, const TCHAR *path, int *sizep, TCHAR *name) {
	bug("target_load_keyfile(,%s,%d,%s)\n", path, *sizep, name);
	TODO();
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
		DebOut("GetCurrentDirName: %s\n", tmp1);

		lock=Lock(path, SHARED_LOCK);
		if(!lock) {
			DebOut("failed to lock %s\n", path);
			return;
		}
		result=NameFromLock(lock, tmp2, MAX_DPATH);
		UnLock(lock);
		if(!result) {
			DebOut("failed to NameFromLock(%s)\n", path);
			return;
		}
		DebOut("NameFromLock(%s): %s\n", path, tmp2);


		if (strnicmp (tmp1, tmp2, strlen (tmp1)) == 0) { // tmp2 is inside tmp1
			DebOut("tmp2 (%s) is inside tmp1 (%s)\n", tmp2, tmp1);
			strcpy(path, tmp2 + strlen (tmp1));
		}
		else {
			DebOut("tmp2 (%s) is not inside tmp1 (%s)\n", tmp2, tmp1);
			strcpy(path, tmp2);
		}
	}
	else {
		lock=Lock(path, SHARED_LOCK);
		if(!lock) {
			DebOut("failed to lock %s\n", path);
			return;
		}
		result=NameFromLock(lock, tmp1, MAX_DPATH);
		UnLock(lock);
		if(!result) {
			DebOut("failed to NameFromLock(%s)\n", path);
			return;
		}
		DebOut("NameFromLock(%s): %s\n", path, tmp1);
		strcpy(path, tmp1);
	}

	DebOut("result: %s\n", path);
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

void handle_events(void) {
	DebOut("entered\n");


/*
    WritePixelArray (
	picasso_memory, 0, start, get_BytesPerRow(W),
	W->RPort, 
	W->BorderLeft, W->BorderTop + start,
	W->Width - W->BorderLeft - W->BorderRight, 
	W->BorderTop + start + i,
	RECTFMT_RAW
    );
*/
#if 0
	if(hAmigaWnd) {
		WritePixelArray(gfxvidinfo.bufmem, 0, 0, get_BytesPerRow(hAmigaWnd),
		                hAmigaWnd->RPort,
										hAmigaWnd->BorderLeft, hAmigaWnd->BorderTop /* + start */,
										hAmigaWnd->Width - hAmigaWnd->BorderLeft - hAmigaWnd->BorderLeft,
										hAmigaWnd->Height - hAmigaWnd->BorderTop - hAmigaWnd->BorderBottom /*?*/,
										RECTFMT_RAW);
		                
	}
#endif
}
