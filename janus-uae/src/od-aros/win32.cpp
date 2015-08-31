/*
* UAE - The Un*x Amiga Emulator
*
* Win32 interface parts needed by AROS port
*
* Copyright 1997-1998 Mathias Ortmann
* Copyright 1997-1999 Brian King
* Copyright 2015 Oliver Brunner
*/

//#define MEMDEBUG

#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#include "sysconfig.h"

#define USETHREADCHARACTERICS 0
#define _WIN32_WINNT 0x700 /* XButtons + MOUSEHWHEEL=XP, Jump List=Win7 */



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
#include "inputrecord.h"
#include "gfxboard.h"
#ifdef RETROPLATFORM
#include "rp.h"
#include "cloanto/RetroPlatformIPC.h"
#endif

#ifdef __AROS__
#include "aros.h"
#endif

static struct  contextcommand cc_disk[] = {
	{ _T("A500"), _T("-0 \"%1\" -s use_gui=no -cfgparam=quickstart=A500,0"), IDI_DISKIMAGE },
	{ _T("A1200"), _T("-0 \"%1\" -s use_gui=no -cfgparam=quickstart=A1200,0"), IDI_DISKIMAGE },
	{ NULL }
};
struct assext exts[] = {
//	{ _T(".cue"), _T("-cdimage=\"%1\" -s use_gui=no"), _T("WinUAE CD image"), IDI_DISKIMAGE, cc_cd },
//	{ _T(".iso"), _T("-cdimage=\"%1\" -s use_gui=no"), _T("WinUAE CD image"), IDI_DISKIMAGE, cc_cd },
//	{ _T(".ccd"), _T("-cdimage=\"%1\" -s use_gui=no"), _T("WinUAE CD image"), IDI_DISKIMAGE, cc_cd },
	{ _T(".uae"), _T("-f \"%1\""), _T("WinUAE configuration file"), IDI_CONFIGFILE, NULL },
	{ _T(".adf"), _T("-0 \"%1\" -s use_gui=no"), _T("WinUAE floppy disk image"), IDI_DISKIMAGE, cc_disk },
	{ _T(".adz"), _T("-0 \"%1\" -s use_gui=no"), _T("WinUAE floppy disk image"), IDI_DISKIMAGE, cc_disk },
	{ _T(".dms"), _T("-0 \"%1\" -s use_gui=no"), _T("WinUAE floppy disk image"), IDI_DISKIMAGE, cc_disk },
	{ _T(".fdi"), _T("-0 \"%1\" -s use_gui=no"), _T("WinUAE floppy disk image"), IDI_DISKIMAGE, cc_disk },
	{ _T(".ipf"), _T("-0 \"%1\" -s use_gui=no"), _T("WinUAE floppy disk image"), IDI_DISKIMAGE, cc_disk },
	{ _T(".uss"), _T("-s statefile=\"%1\" -s use_gui=no"), _T("WinUAE statefile"), IDI_APPICON, NULL },
	{ NULL }
};

