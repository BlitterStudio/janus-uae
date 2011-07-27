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
TCHAR start_path_data[MAX_DPATH];
int uaelib_debug;

void target_default_options (struct uae_prefs *p, int type)
{
	if (type == 2 || type == 0) {
		p->win32_middle_mouse = 1;
		p->win32_logfile = 0;
		p->win32_iconified_nosound = 1;
		p->win32_iconified_pause = 1;
		p->win32_inactive_nosound = 0;
		p->win32_inactive_pause = 0;
		p->win32_ctrl_F11_is_quit = 0;
		p->win32_soundcard = 0;
		p->win32_samplersoundcard = -1;
		p->win32_soundexclusive = 0;
		p->win32_minimize_inactive = 0;
		p->win32_active_priority = 1;
		p->win32_inactive_priority = 2;
		p->win32_iconified_priority = 3;
		p->win32_notaskbarbutton = 0;
		p->win32_alwaysontop = 0;
		p->win32_specialkey = 0xcf; // DIK_END
		p->win32_guikey = -1;
		p->win32_automount_removable = 0;
		p->win32_automount_drives = 0;
		p->win32_automount_removabledrives = 0;
		p->win32_automount_cddrives = 0;
		p->win32_automount_netdrives = 0;
		p->win32_kbledmode = 1;
		p->win32_uaescsimode = UAESCSI_CDEMU;
		p->win32_borderless = 0;
		p->win32_powersavedisabled = 1;
		p->sana2 = 0;
		p->win32_rtgmatchdepth = 1;
		p->win32_rtgscaleifsmall = 1;
		p->win32_rtgallowscaling = 0;
		p->win32_rtgscaleaspectratio = -1;
		p->win32_rtgvblankrate = 0;
		p->win32_commandpathstart[0] = 0;
		p->win32_commandpathend[0] = 0;
		p->win32_statusbar = 1;
		p->gfx_api = 1;
	}
	if (type == 1 || type == 0) {
		p->win32_uaescsimode = UAESCSI_CDEMU;
		p->win32_midioutdev = -2;
		p->win32_midiindev = 0;
		p->win32_automount_removable = 0;
		p->win32_automount_drives = 0;
		p->win32_automount_removabledrives = 0;
		p->win32_automount_cddrives = 0;
		p->win32_automount_netdrives = 0;
		p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5PC | RGBFF_B8G8R8A8;
	}
}

void fetch_configurationpath (TCHAR *out, int size)
{
	strncpy(out, "configurations", size);
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


