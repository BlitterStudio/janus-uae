/*
* UAE - The Un*x Amiga Emulator
*
* Win32 interface
*
* Copyright 1997-1998 Mathias Ortmann
* Copyright 1997-1999 Brian King
*/

//#define MEMDEBUG

#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#include "sysconfig.h"

#define USETHREADCHARACTERICS 0
#define _WIN32_WINNT 0x700 /* XButtons + MOUSEHWHEEL=XP, Jump List=Win7 */

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <zmouse.h>
#include <ddraw.h>
#include <dbt.h>
#include <math.h>
#include <mmsystem.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <dbghelp.h>
#include <float.h>
#include <WtsApi32.h>
#include <Avrt.h>

#include "resource"

#include <wintab.h>
#include "wintablet.h"
#include <pktdef.h>

#include "sysdeps.h"
#include "options.h"
#include "audio.h"
#include "sound.h"
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
#include "dxwrap.h"
#include "picasso96_win.h"
#include "bsdsocket.h"
#include "win32.h"
#include "win32gfx.h"
#include "registry.h"
#include "win32gui.h"
#include "autoconf.h"
#include "gui.h"
#include "sys/mman.h"
#include "avioutput.h"
#include "ahidsound.h"
#include "ahidsound_new.h"
#include "zfile.h"
#include "savestate.h"
#include "ioport.h"
#include "parser.h"
#include "scsidev.h"
#include "disk.h"
#include "catweasel.h"
#include "lcd.h"
#include "uaeipc.h"
#include "ar.h"
#include "akiko.h"
#include "cdtv.h"
#include "direct3d.h"
#include "clipboard_win32.h"
#include "blkdev.h"
#include "inputrecord.h"
#ifdef RETROPLATFORM
#include "rp.h"
#include "cloanto/RetroPlatformIPC.h"
#endif

extern int harddrive_dangerous, do_rdbdump, aspi_allow_all, no_rawinput;
extern int force_directsound;
extern int log_a2065, a2065_promiscuous;
int log_scsi;
int log_net;
int uaelib_debug;
int pissoff_value = 25000;
unsigned int fpucontrol;

extern FILE *debugfile;
extern int console_logging;
OSVERSIONINFO osVersion;
static SYSTEM_INFO SystemInfo;
static int logging_started;
static MINIDUMP_TYPE minidumpmode = MiniDumpNormal;
static int doquit;
static int console_started;
void *globalipc, *serialipc;

int qpcdivisor = 0;
int cpu_mmx = 1;
static int userdtsc = 0;
int D3DEX = 1, d3ddebug = 0;

HINSTANCE hInst = NULL;
HMODULE hUIDLL = NULL;
HWND (WINAPI *pHtmlHelp)(HWND, LPCWSTR, UINT, LPDWORD) = NULL;
HWND hAmigaWnd, hMainWnd, hHiddenWnd, hGUIWnd;
RECT amigawin_rect, mainwin_rect;
static int mouseposx, mouseposy;
static UINT TaskbarRestart;
static HWND TaskbarRestartHWND;
static int forceroms;
static int start_data = 0;
static void *tablet;
HCURSOR normalcursor;
static HWND hwndNextViewer;
HANDLE AVTask;

TCHAR VersionStr[256];
TCHAR BetaStr[64];
extern pathtype path_type;

int in_sizemove;
int manual_painting_needed;

int win_x_diff, win_y_diff;

int toggle_sound;
int paraport_mask;

HKEY hWinUAEKey = NULL;
COLORREF g_dwBackgroundColor;

static int activatemouse = 1;
int ignore_messages_all;
int pause_emulation;

static int didmousepos;
static int sound_closed;
static int recapture;
static int focus;
int mouseactive;

static int mm_timerres;
static int timermode, timeon;
#define MAX_TIMEHANDLES 8
static int timehandlecounter;
static HANDLE timehandle[MAX_TIMEHANDLES];
int sleep_resolution;
static CRITICAL_SECTION cs_time;

TCHAR start_path_data[MAX_DPATH];
TCHAR start_path_exe[MAX_DPATH];
TCHAR start_path_plugins[MAX_DPATH];
TCHAR start_path_new1[MAX_DPATH]; /* AF2005 */
TCHAR start_path_new2[MAX_DPATH]; /* AMIGAFOREVERDATA */
TCHAR help_file[MAX_DPATH];
int af_path_2005;
int quickstart = 1, configurationcache = 1, relativepaths = 0;

static int multi_display = 1;
static TCHAR *inipath = NULL;

static int guijoybutton[MAX_JPORTS];
static int guijoyaxis[MAX_JPORTS][4];
static bool guijoychange;

static int timeend (void)
{
	if (!timeon)
		return 1;
	timeon = 0;
	if (timeEndPeriod (mm_timerres) == TIMERR_NOERROR)
		return 1;
	write_log (L"TimeEndPeriod() failed\n");
	return 0;
}

static int timebegin (void)
{
	if (timeon) {
		timeend ();
		return timebegin ();
	}
	timeon = 0;
	if (timeBeginPeriod (mm_timerres) == TIMERR_NOERROR) {
		timeon = 1;
		return 1;
	}
	write_log (L"TimeBeginPeriod() failed\n");
	return 0;
}

static int init_mmtimer (void)
{
	TIMECAPS tc;
	int i;

	mm_timerres = 0;
	if (timeGetDevCaps (&tc, sizeof (TIMECAPS)) != TIMERR_NOERROR)
		return 0;
	mm_timerres = min (max (tc.wPeriodMin, 1), tc.wPeriodMax);
	sleep_resolution = 1000 / mm_timerres;
	for (i = 0; i < MAX_TIMEHANDLES; i++)
		timehandle[i] = CreateEvent (NULL, TRUE, FALSE, NULL);
	InitializeCriticalSection (&cs_time);
	timehandlecounter = 0;
	return 1;
}

void sleep_millis (int ms)
{
	UINT TimerEvent;
	int start;
	int cnt;

	start = read_processor_time ();
	EnterCriticalSection (&cs_time);
	cnt = timehandlecounter++;
	if (timehandlecounter >= MAX_TIMEHANDLES)
		timehandlecounter = 0;
	LeaveCriticalSection (&cs_time);
	TimerEvent = timeSetEvent (ms, 0, (LPTIMECALLBACK)timehandle[cnt], 0, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
	WaitForSingleObject (timehandle[cnt], ms);
	ResetEvent (timehandle[cnt]);
	timeKillEvent (TimerEvent);
	idletime += read_processor_time () - start;
}

frame_time_t read_processor_time_qpf (void)
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter (&counter);
	if (qpcdivisor == 0)
		return (frame_time_t)(counter.LowPart);
	return (frame_time_t)(counter.QuadPart >> qpcdivisor);
}
frame_time_t read_processor_time_rdtsc (void)
{
	frame_time_t foo = 0;
#if defined(X86_MSVC_ASSEMBLY)
	frame_time_t bar;
	__asm
	{
		rdtsc
			mov foo, eax
			mov bar, edx
	}
	/* very high speed CPU's RDTSC might overflow without this.. */
	foo >>= 6;
	foo |= bar << 26;
#endif
	return foo;
}
frame_time_t read_processor_time (void)
{
#if 0
	static int cnt;

	cnt++;
	if (cnt > 1000000) {
		write_log (L"**************\n");
		cnt = 0;
	}
#endif
	if (userdtsc)
		return read_processor_time_rdtsc ();
	else
		return read_processor_time_qpf ();
}

#include <process.h>
static volatile int dummythread_die;
static void _cdecl dummythread (void *dummy)
{
	SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_LOWEST);
	while (!dummythread_die);
}
static uae_u64 win32_read_processor_time (void)
{
#if defined(X86_MSVC_ASSEMBLY)
	uae_u32 foo, bar;
	__asm
	{
		cpuid
			rdtsc
			mov foo, eax
			mov bar, edx
	}
	return (((uae_u64)bar) << 32) | foo;
#else
	return 0;
#endif
}
static void figure_processor_speed_rdtsc (void)
{
	static int freqset;
	uae_u64 clockrate;
	int oldpri;
	HANDLE th;

	if (freqset)
		return;
	th = GetCurrentThread ();
	freqset = 1;
	oldpri = GetThreadPriority (th);
	SetThreadPriority (th, THREAD_PRIORITY_HIGHEST);
	dummythread_die = -1;
	CloseHandle((HANDLE)_beginthread (&dummythread, 0, 0));
	sleep_millis (500);
	clockrate = win32_read_processor_time ();
	sleep_millis (500);
	clockrate = (win32_read_processor_time () - clockrate) * 2;
	dummythread_die = 0;
	SetThreadPriority (th, oldpri);
	write_log (L"CLOCKFREQ: RDTSC %.2fMHz\n", clockrate / 1000000.0);
	syncbase = clockrate >> 6;
}

static void figure_processor_speed_qpf (void)
{
	LARGE_INTEGER freq;
	static LARGE_INTEGER freq2;
	uae_u64 qpfrate;

	if (!QueryPerformanceFrequency (&freq))
		return;
	if (freq.QuadPart == freq2.QuadPart)
		return;
	freq2.QuadPart = freq.QuadPart;
	qpfrate = freq.QuadPart;
	/* limit to 10MHz */
	qpcdivisor = 0;
	while (qpfrate > 10000000) {
		qpfrate >>= 1;
		qpcdivisor++;
	}
	write_log (L"CLOCKFREQ: QPF %.2fMHz (%.2fMHz, DIV=%d)\n", freq.QuadPart / 1000000.0,
		qpfrate / 1000000.0, 1 << qpcdivisor);
	syncbase = (unsigned long)qpfrate;
}

static void figure_processor_speed (void)
{
	if (SystemInfo.dwNumberOfProcessors > 1)
		userdtsc = 0;
	if (userdtsc)
		figure_processor_speed_rdtsc ();
	if (!userdtsc)
		figure_processor_speed_qpf ();
}

static void setcursor (int oldx, int oldy)
{
	int x = (amigawin_rect.right - amigawin_rect.left) / 2;
	int y = (amigawin_rect.bottom - amigawin_rect.top) / 2;
	mouseposx = oldx - x;
	mouseposy = oldy - y;

	if (currprefs.input_magic_mouse && currprefs.input_tablet > 0 && mousehack_alive () && isfullscreen () <= 0) {
		mouseposx = mouseposy = 0;
		return;
	}
#if 0
	write_log (L"%d %d %d %d %d - %d %d %d %d %d\n",
		x, amigawin_rect.left, amigawin_rect.right, mouseposx, oldx,
		y, amigawin_rect.top, amigawin_rect.bottom, mouseposy, oldy);
#endif
	if (oldx >= 30000 || oldy >= 30000 || oldx <= -30000 || oldy <= -30000) {
		mouseposx = mouseposy = 0;
		oldx = oldy = 0;
	} else {
		if (abs (mouseposx) < 50 && abs (mouseposy) < 50)
			return;
	}
	mouseposx = mouseposy = 0;
	if (oldx < 0 || oldy < 0 || oldx > amigawin_rect.right - amigawin_rect.left || oldy > amigawin_rect.bottom - amigawin_rect.top) {
		write_log (L"Mouse out of range: %dx%d (%dx%d %dx%d)\n", oldx, oldy,
			amigawin_rect.left, amigawin_rect.top, amigawin_rect.right, amigawin_rect.bottom);
		return;
	}
	SetCursorPos (amigawin_rect.left + x, amigawin_rect.top + y);
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
void resumepaused (int priority)
{
	//write_log (L"resume %d (%d)\n", priority, pause_emulation);
	if (pause_emulation > priority)
		return;
	resumesoundpaused ();
	blkdev_exitgui ();
	if (pausemouseactive)
		setmouseactive (-1);
	pausemouseactive = 0;
	pause_emulation = 0;
#ifdef RETROPLATFORM
	rp_pause (pause_emulation);
#endif
}
void setpaused (int priority)
{
	//write_log (L"pause %d (%d)\n", priority, pause_emulation);
	if (pause_emulation > priority)
		return;
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
}

static void checkpause (void)
{
	if (currprefs.win32_inactive_pause) {
		setpaused (1);
		setpriority (&priorities[currprefs.win32_inactive_priority]);
	}
}

static int showcursor;

extern TCHAR config_filename[MAX_DPATH];

static void setmaintitle (HWND hwnd)
{
	TCHAR txt[1000], txt2[500];

#ifdef RETROPLATFORM
	if (rp_isactive ())
		return;
#endif
	txt[0] = 0;
	inprec_getstatus (txt);
	if (config_filename[0]) {
		_tcscat (txt, L"[");
		_tcscat (txt, config_filename);
		_tcscat (txt, L"] - ");
	}
	_tcscat (txt, L"WinUAE");
	txt2[0] = 0;
	if (mouseactive > 0) {
		WIN32GUI_LoadUIString (currprefs.win32_middle_mouse ? IDS_WINUAETITLE_MMB : IDS_WINUAETITLE_NORMAL,
			txt2, sizeof (txt2) / sizeof (TCHAR));
	}
	if (_tcslen (WINUAEBETA) > 0) {
		_tcscat (txt, BetaStr);
		if (_tcslen (WINUAEEXTRA) > 0) {
			_tcscat (txt, L" ");
			_tcscat (txt, WINUAEEXTRA);
		}
	}
	if (txt2[0]) {
		_tcscat (txt, L" - ");
		_tcscat (txt, txt2);
	}
	SetWindowText (hwnd, txt);
}

void refreshtitle (void)
{
	if (isfullscreen () == 0)
		setmaintitle (hMainWnd);
}

#ifndef AVIOUTPUT
static int avioutput_video = 0;
#endif

void setpriority (struct threadpriorities *pri)
{
	int err;
	if (!AVTask) {
		DWORD opri = GetPriorityClass (GetCurrentProcess ());

		if (opri != IDLE_PRIORITY_CLASS && opri != NORMAL_PRIORITY_CLASS && opri != BELOW_NORMAL_PRIORITY_CLASS && opri != ABOVE_NORMAL_PRIORITY_CLASS)
			return;
		err = SetPriorityClass (GetCurrentProcess (), pri->classvalue);
		if (!err)
			write_log (L"priority set failed, %08X\n", GetLastError ());
	}
}

static void setcursorshape (void)
{
	if (currprefs.input_tablet && currprefs.input_magic_mouse && currprefs.input_magic_mouse_cursor == MAGICMOUSE_NATIVE_ONLY) {
		if (GetCursor () != NULL)
			SetCursor (NULL);
	}  else if (!picasso_setwincursor ()) {
		if (GetCursor () != normalcursor)
			SetCursor (normalcursor);
	}
}

static void releasecapture (void)
{
	if (!showcursor)
		return;
	ClipCursor (NULL);
	ReleaseCapture ();
	ShowCursor (TRUE);
	showcursor = 0;
}

void setmouseactive (int active)
{
	//write_log (L"setmouseactive %d->%d\n", mouseactive, active);
	if (active == 0)
		releasecapture ();
	if (mouseactive == active && active >= 0)
		return;

	if (active == 1 && !currprefs.input_magic_mouse) {
		HANDLE c = GetCursor ();
		if (c != normalcursor)
			return;
	}
	if (active) {
		if (IsWindowVisible (hAmigaWnd) == FALSE)
			return;
	}

	if (active < 0)
		active = 1;

	mouseactive = active;

	mouseposx = mouseposy = 0;
	//write_log (L"setmouseactive(%d)\n", active);
	releasecapture ();
	recapture = 0;

	if (isfullscreen () <= 0 && currprefs.input_magic_mouse && currprefs.input_tablet > 0) {
		if (mousehack_alive ())
			return;
		SetCursor (normalcursor);
	}

	if (mouseactive > 0)
		focus = 1;

	if (focus) {
		int donotfocus = 0;
		HWND f = GetFocus ();
		HWND fw = GetForegroundWindow ();
		HWND w1 = hAmigaWnd;
		HWND w2 = hMainWnd;
		HWND w3 = NULL;
#ifdef RETROPLATFORM
		if (rp_isactive ())
			w3 = rp_getparent ();
#endif
		if (f != w1 && f != w2)
			donotfocus = 1;
		if (w3 != NULL && f == w3)
			donotfocus = 0;

#ifdef RETROPLATFORM
		if (rp_isactive () && isfullscreen () == 0)
			donotfocus = 0;
#endif
		if (isfullscreen () > 0)
			donotfocus = 0;
		if (donotfocus) {
			//focus = 0;
			mouseactive = 0;
		}
	}

	if (mouseactive) {
		if (focus) {
			if (!showcursor) {
				ShowCursor (FALSE);
				SetCapture (hAmigaWnd);
				ClipCursor (&amigawin_rect);
			}
			showcursor = 1;
			setcursor (-30000, -30000);
		}
		inputdevice_acquire (TRUE);
		setpriority (&priorities[currprefs.win32_active_priority]);
	} else {
		inputdevice_acquire (FALSE);
	}
	if (!active)
		checkpause ();
	setmaintitle (hMainWnd);
#ifdef RETROPLATFORM
	rp_mouse_capture (active);
	rp_mouse_magic (magicmouse_alive ());
#endif
}

static int hotkeys[] = { VK_VOLUME_UP, VK_VOLUME_DOWN, VK_VOLUME_MUTE, -1 };

static void winuae_active (HWND hWnd, int minimized)
{
	struct threadpriorities *pri;

	write_log (L"winuae_active(%d)\n", minimized);
	/* without this returning from hibernate-mode causes wrong timing
	*/
	timeend ();
	sleep_millis (2);
	timebegin ();

	focus = 1;
	pri = &priorities[currprefs.win32_inactive_priority];
	if (!minimized)
		pri = &priorities[currprefs.win32_active_priority];
	setpriority (pri);

	if (!avioutput_video) {
		clear_inhibit_frame (IHF_WINDOWHIDDEN);
	}
	if (sound_closed) {
		if (sound_closed < 0) {
			resumesoundpaused ();
		} else {
			if (currprefs.win32_iconified_pause)
				resumepaused (1);
			if (currprefs.win32_inactive_pause)
				resumepaused (2);
		}
		sound_closed = 0;
	}
#if 0
	RegisterHotKey (hAmigaWnd, IDHOT_SNAPDESKTOP, 0, VK_SNAPSHOT);
	for (int i = 0; hotkeys[i] >= 0; i++)
		RegisterHotKey (hAmigaWnd, hotkeys[i], 0, hotkeys[i]);
#endif
	if (WIN32GFX_IsPicassoScreen ())
		WIN32GFX_EnablePicasso ();
	getcapslock ();
	inputdevice_acquire (FALSE);
	wait_keyrelease ();
	inputdevice_acquire (TRUE);
	if (isfullscreen() != 0 && !gui_active)
		setmouseactive (1);
#ifdef LOGITECHLCD
	if (!minimized)
		lcd_priority (1);
#endif
	clipboard_active (hAmigaWnd, 1);
	SetThreadExecutionState (ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
#if USETHREADCHARACTERICS
	if (os_vista && AVTask == NULL) {
		DWORD taskIndex = 0;
		if (!(AVTask = AvSetMmThreadCharacteristics (TEXT("Pro Audio"), &taskIndex)))
			write_log (L"AvSetMmThreadCharacteristics failed: %d\n", GetLastError ());
	}
#endif
}

static void winuae_inactive (HWND hWnd, int minimized)
{
	struct threadpriorities *pri;
	int wasfocus = focus;

	write_log (L"winuae_inactive(%d)\n", minimized);
#if USETHREADCHARACTERICS
	if (AVTask)
		AvRevertMmThreadCharacteristics (AVTask);
	AVTask = NULL;
#endif
	if (!currprefs.win32_powersavedisabled)
		SetThreadExecutionState (ES_CONTINUOUS);
	if (minimized)
		exit_gui (0);
#if 0
	for (int i = 0; hotkeys[i] >= 0; i++)
		UnregisterHotKey (hAmigaWnd, hotkeys[i]);
	UnregisterHotKey (hAmigaWnd, IDHOT_SNAPDESKTOP);
#endif
	focus = 0;
	wait_keyrelease ();
	setmouseactive (0);
	clipboard_active (hAmigaWnd, 0);
	inputdevice_unacquire ();
	pri = &priorities[currprefs.win32_inactive_priority];
	if (!quit_program) {
		if (minimized) {
			pri = &priorities[currprefs.win32_iconified_priority];
			if (currprefs.win32_iconified_pause) {
				setpaused (1);
				sound_closed = 1;
			} else if (currprefs.win32_iconified_nosound) {
				setsoundpaused ();
				sound_closed = -1;
			}
			if (!avioutput_video) {
				set_inhibit_frame (IHF_WINDOWHIDDEN);
			}
		} else {
			if (currprefs.win32_inactive_pause) {
				setpaused (2);
				sound_closed = 1;
			} else if (currprefs.win32_inactive_nosound) {
				setsoundpaused ();
				sound_closed = -1;
			}
		}
	}
	setpriority (pri);
#ifdef FILESYS
	filesys_flush_cache ();
#endif
#ifdef LOGITECHLCD
	lcd_priority (0);
#endif
}

void minimizewindow (void)
{
	ShowWindow (hMainWnd, SW_MINIMIZE);
}

void disablecapture (void)
{
	setmouseactive (0);
	focus = 0;
}

void gui_gameport_button_change (int port, int button, int onoff)
{
	//write_log (L"%d %d %d\n", port, button, onoff);
#ifdef RETROPLATFORM
	int mask = 0;
	if (button == JOYBUTTON_CD32_PLAY)
		mask = RP_JOYSTICK_BUTTON5;
	if (button == JOYBUTTON_CD32_RWD)
		mask = RP_JOYSTICK_BUTTON6;
	if (button == JOYBUTTON_CD32_FFW)
		mask = RP_JOYSTICK_BUTTON7;
	if (button == JOYBUTTON_CD32_GREEN)
		mask = RP_JOYSTICK_BUTTON4;
	if (button == JOYBUTTON_3 || button == JOYBUTTON_CD32_YELLOW)
		mask = RP_JOYSTICK_BUTTON3;
	if (button == JOYBUTTON_1 || button == JOYBUTTON_CD32_RED)
		mask = RP_JOYSTICK_BUTTON1;
	if (button == JOYBUTTON_2 || button == JOYBUTTON_CD32_BLUE)
		mask = RP_JOYSTICK_BUTTON2;
	rp_update_gameport (port, mask, onoff);
#endif
	if (onoff)
		guijoybutton[port] |= 1 << button;
	else
		guijoybutton[port] &= ~(1 << button);
	guijoychange = true;
}
void gui_gameport_axis_change (int port, int axis, int state, int max)
{
	int onoff = state ? 100 : 0;
	if (axis < 0 || axis > 3)
		return;
	if (max < 0) {
		if (guijoyaxis[port][axis] == 0)
			return;
		if (guijoyaxis[port][axis] > 0)
			guijoyaxis[port][axis]--;
	} else {
		if (state > max)
			state = max;
		if (state < 0)
			state = 0;
		guijoyaxis[port][axis] = max ? state * 127 / max : onoff;
#ifdef RETROPLATFORM
		if (axis == DIR_LEFT_BIT)
			rp_update_gameport (port, RP_JOYSTICK_LEFT, onoff);
		if (axis == DIR_RIGHT_BIT)
			rp_update_gameport (port, RP_JOYSTICK_RIGHT, onoff);
		if (axis == DIR_UP_BIT)
			rp_update_gameport (port, RP_JOYSTICK_UP, onoff);
		if (axis == DIR_DOWN_BIT)
			rp_update_gameport (port, RP_JOYSTICK_DOWN, onoff);
#endif
	}
	guijoychange = true;
}


void setmouseactivexy (int x, int y, int dir)
{
	int diff = 8;

	if (isfullscreen () > 0)
		return;
	x += amigawin_rect.left;
	y += amigawin_rect.top;
	if (dir & 1)
		x = amigawin_rect.left - diff;
	if (dir & 2)
		x = amigawin_rect.right + diff;
	if (dir & 4)
		y = amigawin_rect.top - diff;
	if (dir & 8)
		y = amigawin_rect.bottom + diff;
	if (!dir) {
		x += (amigawin_rect.right - amigawin_rect.left) / 2;
		y += (amigawin_rect.bottom - amigawin_rect.top) / 2;
	}
	if (mouseactive) {
		disablecapture ();
		SetCursorPos (x, y);
		if (dir)
			recapture = 1;
	}
}

int isfocus (void)
{
	if (isfullscreen () > 0)
		return 1;
	if (focus && mouseactive)
		return 1;
	if (focus)
		return -1;
	return 0;
}

static void handleXbutton (WPARAM wParam, int updown)
{
	int b = GET_XBUTTON_WPARAM (wParam);
	int num = (b & XBUTTON1) ? 3 : (b & XBUTTON2) ? 4 : -1;
	if (num >= 0)
		setmousebuttonstate (dinput_winmouse (), num, updown);
}

#define MSGDEBUG 1

static LRESULT CALLBACK AmigaWindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;
	int mx, my;
	int istablet = (GetMessageExtraInfo () & 0xFFFFFF00) == 0xFF515700;
	static int mm, minimized, recursive, ignoremousemove;
	static bool ignorelbutton;

#if MSGDEBUG > 1
	write_log (L"AWP: %x %x\n", hWnd, message);
#endif
	if (ignore_messages_all)
		return DefWindowProc (hWnd, message, wParam, lParam);

	switch (message)
	{

	case WM_SETFOCUS:
		winuae_active (hWnd, minimized);
		minimized = 0;
		dx_check ();
		break;
	case WM_ACTIVATE:
		if (LOWORD (wParam) == WA_INACTIVE) {
			minimized = HIWORD (wParam) ? 1 : 0;
			winuae_inactive (hWnd, minimized);
		}
		dx_check ();
		break;
	case WM_MOUSEACTIVATE:
		if (isfocus () == 0)
			ignorelbutton = true;
		break;
	case WM_ACTIVATEAPP:
		if (!wParam && isfullscreen () <= 0 && currprefs.win32_minimize_inactive)
			minimizewindow ();

#ifdef RETROPLATFORM
		rp_activate (wParam, lParam);
#endif
		dx_check ();
		break;

	case WM_KEYDOWN:
		if (dinput_wmkey ((uae_u32)lParam))
			inputdevice_add_inputcode (AKS_ENTERGUI, 1);
		return 0;

	case WM_LBUTTONUP:
		if (dinput_winmouse () >= 0 && isfocus ())
			setmousebuttonstate (dinput_winmouse (), 0, 0);
		return 0;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		if (!mouseactive && !gui_active && (!mousehack_alive () || currprefs.input_tablet != TABLET_MOUSEHACK || (currprefs.input_tablet == TABLET_MOUSEHACK && !currprefs.input_magic_mouse) || isfullscreen () > 0)) {
			// borderless = do not capture with single-click
			if (ignorelbutton) {
				ignorelbutton = 0;
				return 0;
			}
			if (message == WM_LBUTTONDOWN && isfullscreen () == 0 && currprefs.win32_borderless && !rp_isactive ()) {
				// full-window drag
				SendMessage (hAmigaWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
				return 0;
			}
			setmouseactive ((message == WM_LBUTTONDBLCLK || isfullscreen() > 0) ? 2 : 1);
		} else if (dinput_winmouse () >= 0 && isfocus ()) {
			setmousebuttonstate (dinput_winmouse (), 0, 1);
		}
		return 0;
	case WM_RBUTTONUP:
		if (dinput_winmouse () >= 0 && isfocus ())
			setmousebuttonstate (dinput_winmouse (), 1, 0);
		return 0;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		if (dinput_winmouse () >= 0 && isfocus ())
			setmousebuttonstate (dinput_winmouse (), 1, 1);
		return 0;
	case WM_MBUTTONUP:
		if (!currprefs.win32_middle_mouse) {
			if (dinput_winmouse () >= 0 && isfocus ())
				setmousebuttonstate (dinput_winmouse (), 2, 0);
		}
		return 0;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		if (currprefs.win32_middle_mouse) {
#ifndef _DEBUG
			if (isfullscreen () > 0)
				minimizewindow ();
#endif
			if (isfullscreen () < 0 && currprefs.win32_minimize_inactive)
				minimizewindow ();
			if (mouseactive)
				setmouseactive (0);
		} else {
			if (dinput_winmouse () >= 0 && isfocus ())
				setmousebuttonstate (dinput_winmouse (), 2, 1);
		}
		return 0;
	case WM_XBUTTONUP:
		if (dinput_winmouse () >= 0 && isfocus ()) {
			handleXbutton (wParam, 0);
			return TRUE;
		}
		return 0;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONDBLCLK:
		if (dinput_winmouse () >= 0 && isfocus ()) {
			handleXbutton (wParam, 1);
			return TRUE;
		}
		return 0;
	case WM_MOUSEWHEEL:
		if (dinput_winmouse () >= 0 && isfocus ()) {
			int val = ((short)HIWORD (wParam));
			setmousestate (dinput_winmouse (), 2, val, 0);
			if (val < 0)
				setmousebuttonstate (dinput_winmouse (), dinput_wheelbuttonstart () + 0, -1);
			else if (val > 0)
				setmousebuttonstate (dinput_winmouse (), dinput_wheelbuttonstart () + 1, -1);
			return TRUE;
		}
		return 0;
	case WM_MOUSEHWHEEL:
		if (dinput_winmouse () >= 0 && isfocus ()) {
			int val = ((short)HIWORD (wParam));
			setmousestate (dinput_winmouse (), 3, val, 0);
			if (val < 0)
				setmousebuttonstate (dinput_winmouse (), dinput_wheelbuttonstart () + 2, -1);
			else if (val > 0)
				setmousebuttonstate (dinput_winmouse (), dinput_wheelbuttonstart () + 3, -1);
			return TRUE;
		}
		return 0;

	case WM_PAINT:
		{
			static int recursive = 0;
			if (recursive == 0) {
				PAINTSTRUCT ps;
				recursive++;
				notice_screen_contents_lost ();
				hDC = BeginPaint (hWnd, &ps);
				/* Check to see if this WM_PAINT is coming while we've got the GUI visible */
				if (manual_painting_needed)
					updatedisplayarea ();
				EndPaint (hWnd, &ps);
				recursive--;
			}
		}
		return 0;

	case WM_DROPFILES:
		dragdrop (hWnd, (HDROP)wParam, &changed_prefs, -1);
		return 0;

	case WM_TIMER:
#ifdef PARALLEL_PORT
		finishjob ();
#endif
		return 0;

	case WM_CREATE:
#ifdef RETROPLATFORM
		rp_set_hwnd (hWnd);
#endif
		DragAcceptFiles (hWnd, TRUE);
		tablet = open_tablet (hWnd);
		normalcursor = LoadCursor (NULL, IDC_ARROW);
		hwndNextViewer = SetClipboardViewer (hWnd); 
		clipboard_init (hWnd);
		return 0;

	case WM_DESTROY:
		ChangeClipboardChain (hWnd, hwndNextViewer); 
		close_tablet (tablet);
		inputdevice_unacquire ();
		dinput_window ();
		return 0;

	case WM_CLOSE:
		uae_quit ();
		return 0;

	case WM_SIZE:
		if (hStatusWnd)
			SendMessage (hStatusWnd, WM_SIZE, wParam, lParam);
		break;

	case WM_WINDOWPOSCHANGED:
		{
			WINDOWPOS *wp = (WINDOWPOS*)lParam;
			if (isfullscreen () <= 0) {
				if (!IsIconic (hWnd) && hWnd == hAmigaWnd) {
					GetWindowRect (hWnd, &amigawin_rect);
					if (isfullscreen () == 0) {
						changed_prefs.gfx_size_win.x = amigawin_rect.left;
						changed_prefs.gfx_size_win.y = amigawin_rect.top;
						config_changed = 1;
					}
				}
				notice_screen_contents_lost ();
			}
		}
		break;

	case WM_SETCURSOR:
		{
			if ((HWND)wParam == hAmigaWnd && currprefs.input_tablet > 0 && currprefs.input_magic_mouse && isfullscreen () <= 0) {
				if (mousehack_alive ()) {
					setcursorshape ();
					return 1;
				}
			}
			break;
		}

	case WM_MOUSEMOVE:
		{
			int wm = dinput_winmouse ();

			mx = (signed short) LOWORD (lParam);
			my = (signed short) HIWORD (lParam);
			mx -= mouseposx;
			my -= mouseposy;

			//write_log (L"%d %d %d %d %d %d %d\n", wm, mouseactive, focus, mx, my, mouseposx, mouseposy);
			if (recapture && isfullscreen () <= 0) {
				setmouseactive (1);
				return 0;
			}
			if (wm < 0 && (istablet || currprefs.input_tablet >= TABLET_MOUSEHACK)) {
				/* absolute */
				setmousestate (0, 0, mx, 1);
				setmousestate (0, 1, my, 1);
				return 0;
			}
			if (wm >= 0) {
				if (istablet || currprefs.input_tablet >= TABLET_MOUSEHACK) {
					/* absolute */
					setmousestate (dinput_winmouse (), 0, mx, 1);
					setmousestate (dinput_winmouse (), 1, my, 1);
					return 0;
				}
				if (!focus || !mouseactive)
					return DefWindowProc (hWnd, message, wParam, lParam);
				if (dinput_winmousemode () == 0) {
					/* relative */
					int mxx = (amigawin_rect.right - amigawin_rect.left) / 2;
					int myy = (amigawin_rect.bottom - amigawin_rect.top) / 2;
					mx = mx - mxx;
					my = my - myy;
					//write_log (L"%d:%dx%d\n", dinput_winmouse(), mx, my);
					setmousestate (dinput_winmouse (), 0, mx, 0);
					setmousestate (dinput_winmouse (), 1, my, 0);
				}
			} else if (isfocus () < 0 && (istablet || currprefs.input_tablet >= TABLET_MOUSEHACK)) {
				setmousestate (0, 0, mx, 1);
				setmousestate (0, 1, my, 1);
			}
			if (showcursor || mouseactive)
				setcursor (LOWORD (lParam), HIWORD (lParam));
			return 0;
		}
		break;

	case WM_MOVING:
		{
			LRESULT lr = DefWindowProc (hWnd, message, wParam, lParam);
			WIN32GFX_WindowMove ();
			return lr;
		}
	case WM_MOVE:
		WIN32GFX_WindowMove ();
		return FALSE;

	case WM_ENABLE:
		rp_set_enabledisable (wParam ? 1 : 0);
		return FALSE;

#ifdef FILESYS
	case WM_USER + 2:
		{
			typedef struct {
				DWORD dwItem1;    // dwItem1 contains the previous PIDL or name of the folder. 
				DWORD dwItem2;    // dwItem2 contains the new PIDL or name of the folder. 
			} SHNOTIFYSTRUCT;
			TCHAR path[MAX_PATH];

			if (lParam == SHCNE_MEDIAINSERTED || lParam == SHCNE_MEDIAREMOVED) {
				SHNOTIFYSTRUCT *shns = (SHNOTIFYSTRUCT*)wParam;
				if (SHGetPathFromIDList ((struct _ITEMIDLIST *)(shns->dwItem1), path)) {
					int inserted = lParam == SHCNE_MEDIAINSERTED ? 1 : 0;
					UINT errormode = SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
					write_log (L"Shell Notification %d '%s'\n", inserted, path);
					if (!win32_hardfile_media_change (path, inserted)) {	
						if ((inserted && CheckRM (path)) || !inserted) {
							if (inserted) {
								DWORD type = GetDriveType(path);
								if (type == DRIVE_CDROM)
									inserted = -1;
							}
							filesys_media_change (path, inserted, NULL);
						}
					}
					SetErrorMode (errormode);
				}
			}
		}
		return TRUE;
	case WM_DEVICECHANGE:
		{
			extern void win32_spti_media_change (TCHAR driveletter, int insert);
			extern void win32_ioctl_media_change (TCHAR driveletter, int insert);
			extern void win32_aspi_media_change (TCHAR driveletter, int insert);
			DEV_BROADCAST_HDR *pBHdr = (DEV_BROADCAST_HDR *)lParam;
			static int waitfornext;

			if (wParam == DBT_DEVNODES_CHANGED && lParam == 0) {
				if (waitfornext)
					inputdevice_devicechange (&changed_prefs);
				waitfornext = 0;
			} else if (pBHdr && pBHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
				DEV_BROADCAST_DEVICEINTERFACE *dbd = (DEV_BROADCAST_DEVICEINTERFACE*)lParam;
				if (wParam == DBT_DEVICEREMOVECOMPLETE)
					inputdevice_devicechange (&changed_prefs);
				else if (wParam == DBT_DEVICEARRIVAL)
					waitfornext = 1;
			} else if (pBHdr && pBHdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
				DEV_BROADCAST_VOLUME *pBVol = (DEV_BROADCAST_VOLUME *)lParam;
				if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
					if (pBVol->dbcv_unitmask) {
						int inserted, i;
						TCHAR drive;
						UINT errormode = SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
						for (i = 0; i <= 'Z'-'A'; i++) {
							if (pBVol->dbcv_unitmask & (1 << i)) {
								TCHAR drvname[10];
								int type;

								drive = 'A' + i;
								_stprintf (drvname, L"%c:\\", drive);
								type = GetDriveType (drvname);
								if (wParam == DBT_DEVICEARRIVAL)
									inserted = 1;
								else
									inserted = 0;
								if (pBVol->dbcv_flags & DBTF_MEDIA) {
#ifdef WINDDK
									win32_spti_media_change (drive, inserted);
									win32_ioctl_media_change (drive, inserted);
#endif
									win32_aspi_media_change (drive, inserted);
								}
								if (type == DRIVE_REMOVABLE || type == DRIVE_CDROM || !inserted) {
									write_log (L"WM_DEVICECHANGE '%s' type=%d inserted=%d\n", drvname, type, inserted);
									if (!win32_hardfile_media_change (drvname, inserted)) {
										if ((inserted && CheckRM (drvname)) || !inserted) {
											if (type == DRIVE_CDROM && inserted)
												inserted = -1;
											filesys_media_change (drvname, inserted, NULL);
										}
									}
								}
							}
						}
						SetErrorMode (errormode);
					}
				}
			}
		}
#endif
		return TRUE;

	case WM_SYSCOMMAND:
		switch (wParam & 0xfff0) // Check System Calls
		{
		case SC_SCREENSAVE: // Screensaver Trying To Start?
		case SC_MONITORPOWER: // Monitor Trying To Enter Powersave?

			// SetThreadExecutionState handles this now
			break;

		default:
			{
				LRESULT lr;

#ifdef RETROPLATFORM
				if ((wParam & 0xfff0) == SC_CLOSE) {
					if (rp_close ())
						return 0;
				}
#endif
				lr = DefWindowProc (hWnd, message, wParam, lParam);
				switch (wParam & 0xfff0)
				{
				case SC_MINIMIZE:
					break;
				case SC_RESTORE:
					break;
				case SC_CLOSE:
					PostQuitMessage (0);
					break;
				}
				return lr;
			}
		}
		break;

	case WM_SYSKEYDOWN:
		if(currprefs.win32_ctrl_F11_is_quit && wParam == VK_F4)
			return 0;
		break;

	case WM_INPUT:
		handle_rawinput (lParam);
		DefWindowProc (hWnd, message, wParam, lParam);
		return 0;

	case WM_NOTIFY:
		{
			LPNMHDR nm = (LPNMHDR)lParam;
			if (nm->hwndFrom == hStatusWnd) {
				switch (nm->code)
				{
					/* status bar clicks */
				case NM_CLICK:
				case NM_RCLICK:
					{
						LPNMMOUSE lpnm = (LPNMMOUSE) lParam;
						int num = (int)lpnm->dwItemSpec;
						if (num >= 7 && num <= 10) { // DF0-DF3
							num -= 7;
							if (nm->code == NM_RCLICK) {
								disk_eject (num);
							} else if (changed_prefs.floppyslots[num].dfxtype >= 0) {
								DiskSelection (hWnd, IDC_DF0 + num, 0, &changed_prefs, 0);
								disk_insert (num, changed_prefs.floppyslots[num].df);
							}
						} else if (num == 4) {
							if (nm->code == NM_CLICK) // POWER
								inputdevice_add_inputcode (AKS_ENTERGUI, 1);
							else
								uae_reset (0);
						} else if (num == 3) {
							if (pause_emulation) {
								resumepaused (9);
								setmouseactive (1);
							}
						}
						return TRUE;
					}
				}
			}
		}
		break;

	case WM_CHANGECBCHAIN: 
		if ((HWND) wParam == hwndNextViewer) 
			hwndNextViewer = (HWND) lParam; 
		else if (hwndNextViewer != NULL) 
			SendMessage (hwndNextViewer, message, wParam, lParam); 
		return 0;
	case WM_DRAWCLIPBOARD:
		clipboard_changed (hWnd);
		SendMessage (hwndNextViewer, message, wParam, lParam); 
		return 0;

	case WM_WTSSESSION_CHANGE:
		{
			static int wasactive;
			switch (wParam)
			{
			case WTS_CONSOLE_CONNECT:
			case WTS_SESSION_UNLOCK:
				if (wasactive)
					winuae_active (hWnd, 0);
				wasactive = 0;
				break;
			case WTS_CONSOLE_DISCONNECT:
			case WTS_SESSION_LOCK:
				wasactive = mouseactive;
				winuae_inactive (hWnd, 0);
				break;
			}
		}

#ifndef _WIN64
	case WT_PROXIMITY:
		{
			send_tablet_proximity (LOWORD (lParam) ? 1 : 0);
			return 0;
		}
	case WT_PACKET:
		{
			PACKET pkt;
			if (inputdevice_is_tablet () <= 0) {
				close_tablet (tablet);
				tablet = NULL;
				return 0;
			}
			if (WTPacket ((HCTX)lParam, wParam, &pkt)) {
				int x, y, z, pres, proxi;
				DWORD buttons;
				ORIENTATION ori;
				ROTATION rot;

				x = pkt.pkX;
				y = pkt.pkY;
				z = pkt.pkZ;
				pres = pkt.pkNormalPressure;
				ori = pkt.pkOrientation;
				rot = pkt.pkRotation;
				buttons = pkt.pkButtons;
				proxi = pkt.pkStatus;
				send_tablet (x, y, z, pres, buttons, proxi, ori.orAzimuth, ori.orAltitude, ori.orTwist, rot.roPitch, rot.roRoll, rot.roYaw, &amigawin_rect);

			}
			return 0;
		}
#endif
	default:
		break;
	}

	return DefWindowProc (hWnd, message, wParam, lParam);
}

static int canstretch (void)
{
	if (isfullscreen () != 0)
		return 0;
	if (currprefs.gfx_filter_autoscale == AUTOSCALE_RESIZE)
		return 0;
	if (!WIN32GFX_IsPicassoScreen ())
		return 1;
	if (currprefs.win32_rtgallowscaling || currprefs.win32_rtgscaleaspectratio)
		return 1;
	return 0;
}

static void plot (LPDRAWITEMSTRUCT lpDIS, int x, int y, int dx, int dy, int idx)
{
	COLORREF rgb;

	x += dx;
	y += dy;
	if (idx == 0)
		rgb = RGB(0x00,0x00,0xff);
	else if (idx == 1)
		rgb = RGB(0xff,0x00,0x00);
	else if (idx == 2)
		rgb = RGB(0xff,0xff,0x00);
	else if (idx == 3)
		rgb = RGB(0x00,0xff,0x00);
	else
		rgb = RGB(0x00,0x00,0x00);

	SetPixel (lpDIS->hDC, x, y, rgb);

	SetPixel (lpDIS->hDC, x + 1, y, rgb);
	SetPixel (lpDIS->hDC, x - 1, y, rgb);

	SetPixel (lpDIS->hDC, x, y + 1, rgb);
	SetPixel (lpDIS->hDC, x, y - 1, rgb);
}

static LRESULT CALLBACK MainWindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RECT myrect;
	PAINTSTRUCT ps;
	RECT rc;
	HDC hDC;

#if MSGDEBUG > 1
	write_log (L"MWP: %x %d\n", hWnd, message);
#endif

	switch (message)
	{
	case WM_SETCURSOR:
	case WM_KILLFOCUS:
	case WM_SETFOCUS:
	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
	case WM_ACTIVATEAPP:
	case WM_DROPFILES:
	case WM_ACTIVATE:
	case WM_SYSCOMMAND:
	case WM_KEYUP:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MOVING:
	case WM_MOVE:
	case WM_SIZING:
	case WM_SIZE:
	case WM_DESTROY:
	case WM_CLOSE:
	case WM_HELP:
	case WM_DEVICECHANGE:
	case WM_INPUT:
	case WM_USER + 1:
	case WM_USER + 2:
	case WM_COMMAND:
	case WM_NOTIFY:
	case WM_ENABLE:
	case WT_PACKET:
	case WM_WTSSESSION_CHANGE:
		return AmigaWindowProc (hWnd, message, wParam, lParam);
#if 0
	case WM_DISPLAYCHANGE:
		if (isfullscreen() <= 0 && !currprefs.gfx_filter && (wParam + 7) / 8 != DirectDraw_GetBytesPerPixel ())
			WIN32GFX_DisplayChangeRequested ();
		break;
#endif
	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi;
			lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize.x = 160 + window_extra_width;
			lpmmi->ptMinTrackSize.y = 128 + window_extra_height;
			lpmmi->ptMaxTrackSize.x = 3072 + window_extra_width;
			lpmmi->ptMaxTrackSize.y = 2048 + window_extra_height;
		}
		return 0;

	case WM_ENTERSIZEMOVE:
		in_sizemove++;
		break;

	case WM_EXITSIZEMOVE:
		in_sizemove--;
		/* fall through */

	case WM_WINDOWPOSCHANGED:
		WIN32GFX_WindowMove ();
		if (hAmigaWnd && isfullscreen () <= 0) {
			DWORD aw, ah;
			int iconic = IsIconic (hWnd);
			if (!iconic)
				GetWindowRect (hAmigaWnd, &amigawin_rect);
			aw = amigawin_rect.right - amigawin_rect.left;
			ah = amigawin_rect.bottom - amigawin_rect.top;
			if (in_sizemove > 0)
				break;

			if (isfullscreen() == 0 && hAmigaWnd && !iconic) {
				static int store_xy;
				RECT rc2;
				if (GetWindowRect (hMainWnd, &rc2)) {
					DWORD left = rc2.left - win_x_diff;
					DWORD top = rc2.top - win_y_diff;
					DWORD width = rc2.right - rc2.left;
					DWORD height = rc2.bottom - rc2.top;
					if (store_xy++) {
						regsetint (NULL, L"MainPosX", left);
						regsetint (NULL, L"MainPosY", top);
					}
					changed_prefs.gfx_size_win.x = left;
					changed_prefs.gfx_size_win.y = top;
					if (canstretch () && (mainwin_rect.right - mainwin_rect.left != width || mainwin_rect.bottom - mainwin_rect.top != height)) {
						changed_prefs.gfx_size_win.width = width - window_extra_width;
						changed_prefs.gfx_size_win.height = height - window_extra_height;
					}
					config_changed = 1;
				}
				if (hStatusWnd)
					SendMessage (hStatusWnd, WM_SIZE, wParam, lParam);
				GetWindowRect (hMainWnd, &mainwin_rect);
				return 0;
			}
			if (!iconic)
				GetWindowRect (hMainWnd, &mainwin_rect);
		}
		break;

	case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *wp = (WINDOWPOS*)lParam;
			if (!canstretch ())
				wp->flags |= SWP_NOSIZE;
			break;
		}

	case WM_PAINT:
		hDC = BeginPaint (hWnd, &ps);
		GetClientRect (hWnd, &rc);
		DrawEdge (hDC, &rc, EDGE_SUNKEN, BF_RECT);
		EndPaint (hWnd, &ps);
		return 0;

	case WM_NCLBUTTONDBLCLK:
		if (wParam == HTCAPTION) {
			toggle_fullscreen (-1);
			return 0;
		}
		break;

	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)lParam;
		if (lpDIS->hwndItem == hStatusWnd) {
			if (lpDIS->itemID > 0 && lpDIS->itemID <= window_led_joy_start) {
				int port = lpDIS->itemID - 1;
				int x = (lpDIS->rcItem.right - lpDIS->rcItem.left + 1) / 2 + lpDIS->rcItem.left - 1;
				int y = (lpDIS->rcItem.bottom - lpDIS->rcItem.top + 1) / 2 + lpDIS->rcItem.top - 1;
				RECT r = lpDIS->rcItem;
				r.left++;
				r.right--;
				r.top++;
				r.bottom--;
				FillRect (lpDIS->hDC, &r, (HBRUSH)(COLOR_3DFACE + 1));
				for (int i = 0; i < 2; i++) {
					int buttons = guijoybutton[port + i * 2];
					int m = i == 0 ? 1 : 2;
					bool got = false;
					if (buttons & (1 << JOYBUTTON_CD32_BLUE)) {
						plot (lpDIS, x - 1, y,  0,  0, 0);
						got = true;
					}
					if (buttons & (1 << JOYBUTTON_CD32_RED)) {
						plot (lpDIS, x + 1, y,  0,  0, 1);
						got = true;
					}
					if (buttons & (1 << JOYBUTTON_CD32_YELLOW)) {
						plot (lpDIS, x, y - 1,  0,  0, 2);
						got = true;
					}
					if (buttons & (1 << JOYBUTTON_CD32_GREEN)) {
						plot (lpDIS, x, y + 1,  0,  0, 3);
						got = true;
					}
					if (!got) {
						if (buttons & 1)
							plot (lpDIS, x, y,  0,  0, 1);
						if (buttons & 2)
							plot (lpDIS, x, y,  0,  0, 0);
						if (buttons & ~(1 | 2))
							plot (lpDIS, x, y,  0,  0, -1);
					}
					for (int j = 0; j < 4; j++) {
						int dx = 0, dy = 0;
						int axis = guijoyaxis[port + i * 2][j];
						if (j == DIR_LEFT_BIT)
							dx = -1;
						if (j == DIR_RIGHT_BIT)
							dx = +1;
						if (j == DIR_UP_BIT)
							dy = -1;
						if (j == DIR_DOWN_BIT)
							dy = +1;
						if (axis && (dx || dy)) {
							dx *= axis * 8 / 127;
							dy *= axis * 8 / 127;
							plot (lpDIS, x, y,  dx, dy, -1);
						}
					}
				}
			} else {
				DWORD flags, tflags;
				COLORREF oc;
				TCHAR *txt = (TCHAR*)lpDIS->itemData;
				tflags = txt[_tcslen (txt) + 1];
				SetBkMode (lpDIS->hDC, TRANSPARENT);
				if ((tflags & 2) == 0)
					tflags &= ~(4 | 8 | 16);
				if (tflags & 4) {
					oc = SetTextColor (lpDIS->hDC, RGB(0xcc, 0x00, 0x00)); // writing
				} else if (tflags & 8) {
					oc = SetTextColor (lpDIS->hDC, RGB(0x00, 0xcc, 0x00)); // playing
				} else {
					oc = SetTextColor (lpDIS->hDC, GetSysColor ((tflags & 2) ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
				}
				flags = DT_VCENTER | DT_SINGLELINE;
				if (tflags & 1) {
					flags |= DT_CENTER;
					lpDIS->rcItem.left++;
					lpDIS->rcItem.right -= 3;
				} else {
					flags |= DT_LEFT;
					lpDIS->rcItem.right--;
					lpDIS->rcItem.left += 2;
				}
				DrawText (lpDIS->hDC, txt, _tcslen (txt), &lpDIS->rcItem, flags);
				SetTextColor (lpDIS->hDC, oc);
			}
		}
		break;
	}
	
	default:
		break;

	}
	return DefWindowProc (hWnd, message, wParam, lParam);
}

static LRESULT CALLBACK HiddenWindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_USER + 1: /* Systray icon */
		switch (lParam)
		{
		case WM_LBUTTONDOWN:
			SetForegroundWindow (hGUIWnd ? hGUIWnd : hMainWnd);
			break;
		case WM_LBUTTONDBLCLK:
			if (!gui_active)
				inputdevice_add_inputcode (AKS_ENTERGUI, 1);
			break;
		case WM_RBUTTONDOWN:
			if (!gui_active)
				systraymenu (hWnd);
			else
				SetForegroundWindow (hGUIWnd ? hGUIWnd : hMainWnd);
			break;
		}
		break;
	case WM_COMMAND:
		switch (wParam & 0xffff)
		{
		case ID_ST_CONFIGURATION:
			inputdevice_add_inputcode (AKS_ENTERGUI, 1);
			break;
		case ID_ST_HELP:
			if (pHtmlHelp)
				pHtmlHelp (NULL, help_file, 0, NULL);
			break;
		case ID_ST_QUIT:
			uae_quit ();
			break;
		case ID_ST_RESET:
			uae_reset (0);
			break;

		case ID_ST_CDEJECTALL:
			changed_prefs.cdslots[0].name[0] = 0;
			changed_prefs.cdslots[0].inuse = false;
			break;
		case ID_ST_CD0:
			DiskSelection (isfullscreen() > 0 ? NULL : hWnd, IDC_CD_SELECT, 17, &changed_prefs, 0);
			break;

		case ID_ST_EJECTALL:
			disk_eject (0);
			disk_eject (1);
			disk_eject (2);
			disk_eject (3);
			break;
		case ID_ST_DF0:
			DiskSelection (isfullscreen() > 0 ? NULL : hWnd, IDC_DF0, 0, &changed_prefs, 0);
			disk_insert (0, changed_prefs.floppyslots[0].df);
			break;
		case ID_ST_DF1:
			DiskSelection (isfullscreen() > 0 ? NULL : hWnd, IDC_DF1, 0, &changed_prefs, 0);
			disk_insert (1, changed_prefs.floppyslots[0].df);
			break;
		case ID_ST_DF2:
			DiskSelection (isfullscreen() > 0 ? NULL : hWnd, IDC_DF2, 0, &changed_prefs, 0);
			disk_insert (2, changed_prefs.floppyslots[0].df);
			break;
		case ID_ST_DF3:
			DiskSelection (isfullscreen() > 0 ? NULL : hWnd, IDC_DF3, 0, &changed_prefs, 0);
			disk_insert (3, changed_prefs.floppyslots[0].df);
			break;

		}
		break;
	}
	if (TaskbarRestart != 0 && TaskbarRestartHWND == hWnd && message == TaskbarRestart) {
		//write_log (L"notif: taskbarrestart\n");
		systray (TaskbarRestartHWND, FALSE);
	}
	return DefWindowProc (hWnd, message, wParam, lParam);
}

int handle_msgpump (void)
{
	int got = 0;
	MSG msg;

	while (PeekMessage (&msg, 0, 0, 0, PM_REMOVE)) {
		got = 1;
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	while (checkIPC (globalipc, &currprefs));
	return got;
}

void handle_events (void)
{
	MSG msg;
	int was_paused = 0;
	static int cnt;

	if (hStatusWnd && guijoychange && window_led_joy_start > 0) {
		guijoychange = false;
		for (int i = 0; i < window_led_joy_start; i++)
			PostMessage (hStatusWnd, SB_SETTEXT, (WPARAM)((i + 1) | SBT_OWNERDRAW), (LPARAM)L"");
	}

	while (pause_emulation) {
		if (pause_emulation && was_paused == 0) {
			setpaused (pause_emulation);
			was_paused = pause_emulation;
			manual_painting_needed++;
			gui_fps (0, 0);
		}
		while (PeekMessage (&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
		sleep_millis (20);
		inputdevicefunc_keyboard.read ();
		inputdevicefunc_mouse.read ();
		inputdevicefunc_joystick.read ();
		inputdevice_handle_inputcode ();
		check_prefs_changed_gfx ();
#ifdef RETROPLATFORM
		rp_vsync ();
#endif
		cnt = 0;
		while (checkIPC (globalipc, &currprefs));
		if (quit_program)
			break;
	}
#if 0
	while (PeekMessage (&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	while (checkIPC (globalipc, &currprefs));
#endif
	if (was_paused) {
		resumepaused (was_paused);
		sound_closed = 0;
		manual_painting_needed--;
	}
	cnt--;
	if (cnt <= 0) {
		figure_processor_speed ();
		flush_log ();
		cnt = 50 * 5;
	}
}

/* We're not a console-app anymore! */
void setup_brkhandler (void)
{
}
void remove_brkhandler (void)
{
}

static void WIN32_UnregisterClasses (void)
{
	systray (hHiddenWnd, TRUE);
	DestroyWindow (hHiddenWnd);
}

static int WIN32_RegisterClasses (void)
{
	WNDCLASS wc;
	HDC hDC;
	COLORREF black = RGB(0, 0, 0);

	g_dwBackgroundColor = RGB(10, 0, 10);
	hDC = GetDC (NULL);
	if (GetDeviceCaps (hDC, NUMCOLORS) != -1)
		g_dwBackgroundColor = RGB (255, 0, 255);
	ReleaseDC (NULL, hDC);

	wc.style = CS_DBLCLKS | CS_OWNDC;
	wc.lpfnWndProc = AmigaWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon (GetModuleHandle (NULL), MAKEINTRESOURCE (IDI_APPICON));
	wc.hCursor = LoadCursor (NULL, IDC_ARROW);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"AmigaPowah";
	wc.hbrBackground = CreateSolidBrush (g_dwBackgroundColor);
	if (!RegisterClass (&wc))
		return 0;

	wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon (GetModuleHandle (NULL), MAKEINTRESOURCE (IDI_APPICON));
	wc.hCursor = LoadCursor (NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush (black);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"PCsuxRox";
	if (!RegisterClass (&wc))
		return 0;

	wc.style = 0;
	wc.lpfnWndProc = HiddenWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon (GetModuleHandle (NULL), MAKEINTRESOURCE (IDI_APPICON));
	wc.hCursor = NULL;
	wc.hbrBackground = CreateSolidBrush (g_dwBackgroundColor);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"Useless";
	if (!RegisterClass (&wc))
		return 0;

	hHiddenWnd = CreateWindowEx (0,
		L"Useless", L"You don't see me",
		WS_POPUP,
		0, 0,
		1, 1,
		NULL, NULL, 0, NULL);
	if (!hHiddenWnd)
		return 0;

	return 1;
}

static HINSTANCE hRichEdit = NULL, hHtmlHelp = NULL;

int WIN32_CleanupLibraries (void)
{
	if (hRichEdit)
		FreeLibrary (hRichEdit);

	if (hHtmlHelp)
		FreeLibrary (hHtmlHelp);

	if (hUIDLL)
		FreeLibrary (hUIDLL);
	CoUninitialize ();
	return 1;
}

/* HtmlHelp Initialization - optional component */
int WIN32_InitHtmlHelp (void)
{
	TCHAR *chm = L"WinUAE.chm";
	int result = 0;
	_stprintf(help_file, L"%s%s", start_path_data, chm);
	if (!zfile_exists (help_file))
		_stprintf(help_file, L"%s%s", start_path_exe, chm);
	if (zfile_exists (help_file)) {
		if (hHtmlHelp = LoadLibrary (L"HHCTRL.OCX")) {
			pHtmlHelp = (HWND(WINAPI *)(HWND, LPCWSTR, UINT, LPDWORD))GetProcAddress (hHtmlHelp, "HtmlHelpW");
			result = 1;
		}
	}
	return result;
}

struct winuae_lang langs[] =
{
	{ LANG_AFRIKAANS, L"Afrikaans" },
	{ LANG_ARABIC, L"Arabic" },
	{ LANG_ARMENIAN, L"Armenian" },
	{ LANG_ASSAMESE, L"Assamese" },
	{ LANG_AZERI, L"Azeri" },
	{ LANG_BASQUE, L"Basque" },
	{ LANG_BELARUSIAN, L"Belarusian" },
	{ LANG_BENGALI, L"Bengali" },
	{ LANG_BULGARIAN, L"Bulgarian" },
	{ LANG_CATALAN, L"Catalan" },
	{ LANG_CHINESE, L"Chinese" },
	{ LANG_CROATIAN, L"Croatian" },
	{ LANG_CZECH, L"Czech" },
	{ LANG_DANISH, L"Danish" },
	{ LANG_DUTCH, L"Dutch" },
	{ LANG_ESTONIAN, L"Estoanian" },
	{ LANG_FAEROESE, L"Faeroese" },
	{ LANG_FARSI, L"Farsi" },
	{ LANG_FINNISH, L"Finnish" },
	{ LANG_FRENCH, L"French" },
	{ LANG_GEORGIAN, L"Georgian" },
	{ LANG_GERMAN, L"German" },
	{ LANG_GREEK, L"Greek" },
	{ LANG_GUJARATI, L"Gujarati" },
	{ LANG_HEBREW, L"Hebrew" },
	{ LANG_HINDI, L"Hindi" },
	{ LANG_HUNGARIAN, L"Hungarian" },
	{ LANG_ICELANDIC, L"Icelandic" },
	{ LANG_INDONESIAN, L"Indonesian" },
	{ LANG_ITALIAN, L"Italian" },
	{ LANG_JAPANESE, L"Japanese" },
	{ LANG_KANNADA, L"Kannada" },
	{ LANG_KASHMIRI, L"Kashmiri" },
	{ LANG_KAZAK, L"Kazak" },
	{ LANG_KONKANI, L"Konkani" },
	{ LANG_KOREAN, L"Korean" },
	{ LANG_LATVIAN, L"Latvian" },
	{ LANG_LITHUANIAN, L"Lithuanian" },
	{ LANG_MACEDONIAN, L"Macedonian" },
	{ LANG_MALAY, L"Malay" },
	{ LANG_MALAYALAM, L"Malayalam" },
	{ LANG_MANIPURI, L"Manipuri" },
	{ LANG_MARATHI, L"Marathi" },
	{ LANG_NEPALI, L"Nepali" },
	{ LANG_NORWEGIAN, L"Norwegian" },
	{ LANG_ORIYA, L"Oriya" },
	{ LANG_POLISH, L"Polish" },
	{ LANG_PORTUGUESE, L"Portuguese" },
	{ LANG_PUNJABI, L"Punjabi" },
	{ LANG_ROMANIAN, L"Romanian" },
	{ LANG_RUSSIAN, L"Russian" },
	{ LANG_SANSKRIT, L"Sanskrit" },
	{ LANG_SINDHI, L"Sindhi" },
	{ LANG_SLOVAK, L"Slovak" },
	{ LANG_SLOVENIAN, L"Slovenian" },
	{ LANG_SPANISH, L"Spanish" },
	{ LANG_SWAHILI, L"Swahili" },
	{ LANG_SWEDISH, L"Swedish" },
	{ LANG_TAMIL, L"Tamil" },
	{ LANG_TATAR, L"Tatar" },
	{ LANG_TELUGU, L"Telugu" },
	{ LANG_THAI, L"Thai" },
	{ LANG_TURKISH, L"Turkish" },
	{ LANG_UKRAINIAN, L"Ukrainian" },
	{ LANG_UZBEK, L"Uzbek" },
	{ LANG_VIETNAMESE, L"Vietnamese" },
	{ LANG_ENGLISH, L"default" },
	{ 0x400, L"guidll.dll"},
	{ 0, NULL }
};
static TCHAR *getlanguagename(DWORD id)
{
	int i;
	for (i = 0; langs[i].name; i++) {
		if (langs[i].id == id)
			return langs[i].name;
	}
	return NULL;
}

HMODULE language_load (WORD language)
{
	HMODULE result = NULL;
#if LANG_DLL > 0
	TCHAR dllbuf[MAX_DPATH];
	TCHAR *dllname;

	if (language <= 0) {
		/* new user-specific Windows ME/2K/XP method to get UI language */
		language = GetUserDefaultUILanguage ();
		language &= 0x3ff; // low 9-bits form the primary-language ID
	}
	if (language == LANG_GERMAN)
		hrtmon_lang = 2;
	if (language == LANG_FRENCH)
		hrtmon_lang = 3;
	dllname = getlanguagename (language);
	if (dllname) {
		DWORD  dwVersionHandle, dwFileVersionInfoSize;
		LPVOID lpFileVersionData = NULL;
		BOOL   success = FALSE;
		int fail = 1;

		if (language == 0x400)
			_tcscpy (dllbuf, L"guidll.dll");
		else
			_stprintf (dllbuf, L"WinUAE_%s.dll", dllname);
		result = WIN32_LoadLibrary (dllbuf);
		if (result)  {
			dwFileVersionInfoSize = GetFileVersionInfoSize (dllbuf, &dwVersionHandle);
			if (dwFileVersionInfoSize) {
				if (lpFileVersionData = xcalloc (uae_u8, dwFileVersionInfoSize)) {
					if (GetFileVersionInfo (dllbuf, dwVersionHandle, dwFileVersionInfoSize, lpFileVersionData)) {
						VS_FIXEDFILEINFO *vsFileInfo = NULL;
						UINT uLen;
						fail = 0;
						if (VerQueryValue (lpFileVersionData, TEXT("\\"), (void **)&vsFileInfo, &uLen)) {
							if (vsFileInfo &&
								HIWORD(vsFileInfo->dwProductVersionMS) == UAEMAJOR
								&& LOWORD(vsFileInfo->dwProductVersionMS) == UAEMINOR
								&& (HIWORD(vsFileInfo->dwProductVersionLS) == UAESUBREV)) {
									success = TRUE;
									write_log (L"Translation DLL '%s' loaded and enabled\n", dllbuf);
							} else {
								write_log (L"Translation DLL '%s' version mismatch (%d.%d.%d)\n", dllbuf,
									HIWORD(vsFileInfo->dwProductVersionMS),
									LOWORD(vsFileInfo->dwProductVersionMS),
									HIWORD(vsFileInfo->dwProductVersionLS));
							}
						}
					}
					xfree (lpFileVersionData);
				}
			}
		}
		if (fail) {
			DWORD err = GetLastError ();
			if (err != ERROR_MOD_NOT_FOUND && err != ERROR_DLL_NOT_FOUND)
				write_log (L"Translation DLL '%s' failed to load, error %d\n", dllbuf, GetLastError ());
		}
		if (result && !success) {
			FreeLibrary (result);
			result = NULL;
		}
	}
#endif
	return result;
}

struct threadpriorities priorities[] = {
	{ NULL, THREAD_PRIORITY_ABOVE_NORMAL, ABOVE_NORMAL_PRIORITY_CLASS, IDS_PRI_ABOVENORMAL },
	{ NULL, THREAD_PRIORITY_NORMAL, NORMAL_PRIORITY_CLASS, IDS_PRI_NORMAL },
	{ NULL, THREAD_PRIORITY_BELOW_NORMAL, BELOW_NORMAL_PRIORITY_CLASS, IDS_PRI_BELOWNORMAL },
	{ NULL, THREAD_PRIORITY_LOWEST, IDLE_PRIORITY_CLASS, IDS_PRI_LOW },
	{ 0, 0, 0, 0 }
};

static void pritransla (void)
{
	int i;

	for (i = 0; priorities[i].id; i++) {
		TCHAR tmp[MAX_DPATH];
		WIN32GUI_LoadUIString (priorities[i].id, tmp, sizeof (tmp) / sizeof (TCHAR));
		priorities[i].name = my_strdup (tmp);
	}
}

static void WIN32_InitLang (void)
{
	int lid;
	WORD langid = -1;

	if (regqueryint (NULL, L"Language", &lid))
		langid = (WORD)lid;
	hUIDLL = language_load (langid);
	pritransla ();
}

typedef HRESULT (CALLBACK* SETCURRENTPROCESSEXPLICITAPPUSERMODEIDD)(PCWSTR);

/* try to load COMDLG32 and DDRAW, initialize csDraw */
static int WIN32_InitLibraries (void)
{
	LARGE_INTEGER freq;
	SETCURRENTPROCESSEXPLICITAPPUSERMODEIDD pSetCurrentProcessExplicitAppUserModelID; 

	CoInitializeEx (NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	/* Determine our processor speed and capabilities */
	if (!init_mmtimer ()) {
		pre_gui_message (L"MMTimer initialization failed, exiting..");
		return 0;
	}
	if (!QueryPerformanceCounter (&freq)) {
		pre_gui_message (L"No QueryPerformanceFrequency() supported, exiting..\n");
		return 0;
	}
	rpt_available = 1;
	figure_processor_speed ();
	if (!timebegin ()) {
		pre_gui_message (L"MMTimer second initialization failed, exiting..");
		return 0;
	}
	pSetCurrentProcessExplicitAppUserModelID = (SETCURRENTPROCESSEXPLICITAPPUSERMODEIDD)GetProcAddress (
		GetModuleHandle (L"shell32.dll"), "SetCurrentProcessExplicitAppUserModelID");
	if (pSetCurrentProcessExplicitAppUserModelID)
		pSetCurrentProcessExplicitAppUserModelID (WINUAEAPPNAME);

	hRichEdit = LoadLibrary (L"RICHED32.DLL");
	return 1;
}

int debuggable (void)
{
	return 0;
}

void toggle_mousegrab (void)
{
}
#define LOG_BOOT L"winuaebootlog.txt"
#define LOG_NORMAL L"winuaelog.txt"

void logging_open (int bootlog, int append)
{
	TCHAR debugfilename[MAX_DPATH];

	debugfilename[0] = 0;
#ifndef	SINGLEFILE
	if (currprefs.win32_logfile)
		_stprintf (debugfilename, L"%s%s", start_path_data, LOG_NORMAL);
	if (bootlog)
		_stprintf (debugfilename, L"%s%s", start_path_data, LOG_BOOT);
	if (debugfilename[0]) {
		if (!debugfile)
			debugfile = log_open (debugfilename, append, bootlog);
	}
#endif
}

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

void logging_init (void)
{
#ifndef _WIN64
	LPFN_ISWOW64PROCESS fnIsWow64Process;
#endif
	int wow64 = 0;
	static int started;
	static int first;
	TCHAR tmp[MAX_DPATH];

	if (first > 1) {
		write_log (L"** RESTART **\n");
		return;
	}
	if (first == 1) {
		write_log (L"Log (%s): '%s%s'\n", currprefs.win32_logfile ? L"enabled" : L"disabled",
			start_path_data, LOG_NORMAL);
		if (debugfile)
			log_close (debugfile);
		debugfile = 0;
	}
	logging_open (first ? 0 : 1, 0);
	logging_started = 1;
	first++;
#ifdef _WIN64
	wow64 = 1;
#else
	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress (GetModuleHandle (L"kernel32"), "IsWow64Process");
	if (fnIsWow64Process)
		fnIsWow64Process (GetCurrentProcess (), &wow64);
#endif
	write_log (L"%s (%d.%d %s%s[%d])", VersionStr,
		osVersion.dwMajorVersion, osVersion.dwMinorVersion, osVersion.szCSDVersion,
		_tcslen (osVersion.szCSDVersion) > 0 ? L" " : L"", os_winnt_admin);
	write_log (L" %d-bit %X.%X.%X %d",
		wow64 ? 64 : 32,
		SystemInfo.wProcessorArchitecture, SystemInfo.wProcessorLevel, SystemInfo.wProcessorRevision,
		SystemInfo.dwNumberOfProcessors);
	write_log (L"\n(c) 1995-2001 Bernd Schmidt   - Core UAE concept and implementation."
		L"\n(c) 1998-2011 Toni Wilen      - Win32 port, core code updates."
		L"\n(c) 1996-2001 Brian King      - Win32 port, Picasso96 RTG, and GUI."
		L"\n(c) 1996-1999 Mathias Ortmann - Win32 port and bsdsocket support."
		L"\n(c) 2000-2001 Bernd Meyer     - JIT engine."
		L"\n(c) 2000-2005 Bernd Roesch    - MIDI input, many fixes."
		L"\nPress F12 to show the Settings Dialog (GUI), Alt-F4 to quit."
		L"\nEnd+F1 changes floppy 0, End+F2 changes floppy 1, etc."
		L"\n");
	tmp[0] = 0;
	GetModuleFileName (NULL, tmp, sizeof tmp / sizeof (TCHAR));
	write_log (L"'%s'\n", tmp);
	write_log (L"EXE: '%s', DATA: '%s', PLUGIN: '%s'\n", start_path_exe, start_path_data, start_path_plugins);
	regstatus ();
}

void logging_cleanup (void)
{
	if (debugfile)
		fclose (debugfile);
	debugfile = 0;
}

uae_u8 *save_log (int bootlog, int *len)
{
	FILE *f;
	uae_u8 *dst = NULL;
	int size;

	if (!logging_started)
		return NULL;
	f = _tfopen (bootlog ? LOG_BOOT : LOG_NORMAL, L"rb");
	if (!f)
		return NULL;
	fseek (f, 0, SEEK_END);
	size = ftell (f);
	fseek (f, 0, SEEK_SET);
	if (*len > 0 && size > *len)
		size = *len;
	if (size > 0) {
		dst = xcalloc (uae_u8, size + 1);
		if (dst)
			fread (dst, 1, size, f);
		fclose (f);
		*len = size + 1;
	}
	return dst;
}

void stripslashes (TCHAR *p)
{
	while (_tcslen (p) > 0 && (p[_tcslen (p) - 1] == '\\' || p[_tcslen (p) - 1] == '/'))
		p[_tcslen (p) - 1] = 0;
}
void fixtrailing (TCHAR *p)
{
	if (_tcslen(p) == 0)
		return;
	if (p[_tcslen(p) - 1] == '/' || p[_tcslen(p) - 1] == '\\')
		return;
	_tcscat(p, L"\\");
}
// convert path to absolute or relative
void fullpath (TCHAR *path, int size)
{
	if (path[0] == 0 || (path[0] == '\\' && path[1] == '\\') || path[0] == ':')
		return;
	/* <drive letter>: is supposed to mean same as <drive letter>:\ */
	if (_istalpha (path[0]) && path[1] == ':' && path[2] == 0)
		_tcscat (path, L"\\");
	if (relativepaths) {
		TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH];
		tmp1[0] = 0;
		GetCurrentDirectory (sizeof tmp1 / sizeof (TCHAR), tmp1);
		fixtrailing (tmp1);
		tmp2[0] = 0;
		int ret = GetFullPathName (path, sizeof tmp2 / sizeof (TCHAR), tmp2, NULL);
		if (ret == 0 || ret >= sizeof tmp2 / sizeof (TCHAR))
			return;
		if (_tcsnicmp (tmp1, tmp2, _tcslen (tmp1)) == 0) { // tmp2 is inside tmp1
			_tcscpy (path, L".\\");
			_tcscat (path, tmp2 + _tcslen (tmp1));
		} else {
			_tcscpy (path, tmp2);
		}
	} else {
		TCHAR tmp[MAX_DPATH];
		_tcscpy (tmp, path);
		GetFullPathName (tmp, size, path, NULL);
	}
}
void getpathpart (TCHAR *outpath, int size, const TCHAR *inpath)
{
	_tcscpy (outpath, inpath);
	TCHAR *p = _tcsrchr (outpath, '\\');
	if (p)
		p[0] = 0;
	fixtrailing (outpath);
}
void getfilepart (TCHAR *out, int size, const TCHAR *path)
{
	out[0] = 0;
	const TCHAR *p = _tcsrchr (path, '\\');
	if (p)
		_tcscpy (out, p + 1);
	else
		_tcscpy (out, path);
}

typedef DWORD (STDAPICALLTYPE *PFN_GetKey)(LPVOID lpvBuffer, DWORD dwSize);
uae_u8 *target_load_keyfile (struct uae_prefs *p, const TCHAR *path, int *sizep, TCHAR *name)
{
	uae_u8 *keybuf = NULL;
	HMODULE h;
	PFN_GetKey pfnGetKey;
	int size;
	TCHAR *libname = L"amigaforever.dll";

	h = WIN32_LoadLibrary (libname);
	if (!h) {
		TCHAR path[MAX_DPATH];
		_stprintf (path, L"%s..\\Player\\%s", start_path_exe, libname);
		h = WIN32_LoadLibrary2 (path);
		if (!h) {
			TCHAR *afr = _wgetenv (L"AMIGAFOREVERROOT");
			if (afr) {
				TCHAR tmp[MAX_DPATH];
				_tcscpy (tmp, afr);
				fixtrailing (tmp);
				_stprintf (path, L"%sPlayer\\%s", tmp, libname);
				h = WIN32_LoadLibrary2 (path);
			}
		}
	}
	if (!h)
		return NULL;
	GetModuleFileName (h, name, MAX_DPATH);
	//write_log (L"keydll: %s'\n", name);
	pfnGetKey = (PFN_GetKey)GetProcAddress (h, "GetKey");
	//write_log (L"addr: %08x\n", pfnGetKey);
	if (pfnGetKey) {
		size = pfnGetKey (NULL, 0);
		*sizep = size;
		//write_log (L"size: %d\n", size);
		if (size > 0) {
			int gotsize;
			keybuf = xmalloc (uae_u8, size);
			gotsize = pfnGetKey (keybuf, size);
			//write_log (L"gotsize: %d\n", gotsize);
			if (gotsize != size) {
				xfree (keybuf);
				keybuf = NULL;
			}
		}
	}
	FreeLibrary (h);
	//write_log (L"keybuf=%08x\n", keybuf);
	return keybuf;
}


extern const TCHAR *get_aspi_path (int);

static int get_aspi (int old)
{
	if (old == UAESCSI_NEROASPI && get_aspi_path (1))
		return old;
	if (old == UAESCSI_FROGASPI && get_aspi_path (2))
		return old;
	if (old == UAESCSI_ADAPTECASPI && get_aspi_path (0))
		return old;
	if (get_aspi_path (1))
		return UAESCSI_NEROASPI;
	else if (get_aspi_path (2))
		return UAESCSI_FROGASPI;
	else if (get_aspi_path (0))
		return UAESCSI_ADAPTECASPI;
	else
		return UAESCSI_SPTI;
}


/***
*static void parse_cmdline(cmdstart, argv, args, numargs, numchars)
*
*Purpose:
*       Parses the command line and sets up the argv[] array.
*       On entry, cmdstart should point to the command line,
*       argv should point to memory for the argv array, args
*       points to memory to place the text of the arguments.
*       If these are NULL, then no storing (only counting)
*       is done.  On exit, *numargs has the number of
*       arguments (plus one for a final NULL argument),
*       and *numchars has the number of bytes used in the buffer
*       pointed to by args.
*
*Entry:
*       _TSCHAR *cmdstart - pointer to command line of the form
*           <progname><nul><args><nul>
*       _TSCHAR **argv - where to build argv array; NULL means don't
*                       build array
*       _TSCHAR *args - where to place argument text; NULL means don't
*                       store text
*
*Exit:
*       no return value
*       int *numargs - returns number of argv entries created
*       int *numchars - number of characters used in args buffer
*
*Exceptions:
*
*******************************************************************************/

#define NULCHAR    _T('\0')
#define SPACECHAR  _T(' ')
#define TABCHAR    _T('\t')
#define DQUOTECHAR _T('\"')
#define SLASHCHAR  _T('\\')


static void __cdecl wparse_cmdline (
	const _TSCHAR *cmdstart,
	_TSCHAR **argv,
	_TSCHAR *args,
	int *numargs,
	int *numchars
	)
{
	const _TSCHAR *p;
	_TUCHAR c;
	int inquote;                    /* 1 = inside quotes */
	int copychar;                   /* 1 = copy char to *args */
	unsigned numslash;              /* num of backslashes seen */

	*numchars = 0;
	*numargs = 1;                   /* the program name at least */

	/* first scan the program name, copy it, and count the bytes */
	p = cmdstart;
	if (argv)
		*argv++ = args;

#ifdef WILDCARD
	/* To handle later wild card expansion, we prefix each entry by
	it's first character before quote handling.  This is done
	so _[w]cwild() knows whether to expand an entry or not. */
	if (args)
		*args++ = *p;
	++*numchars;

#endif  /* WILDCARD */

	/* A quoted program name is handled here. The handling is much
	simpler than for other arguments. Basically, whatever lies
	between the leading double-quote and next one, or a terminal null
	character is simply accepted. Fancier handling is not required
	because the program name must be a legal NTFS/HPFS file name.
	Note that the double-quote characters are not copied, nor do they
	contribute to numchars. */
	inquote = FALSE;
	do {
		if (*p == DQUOTECHAR )
		{
			inquote = !inquote;
			c = (_TUCHAR) *p++;
			continue;
		}
		++*numchars;
		if (args)
			*args++ = *p;

		c = (_TUCHAR) *p++;
#ifdef _MBCS
		if (_ismbblead(c)) {
			++*numchars;
			if (args)
				*args++ = *p;   /* copy 2nd byte too */
			p++;  /* skip over trail byte */
		}
#endif  /* _MBCS */

	} while ( (c != NULCHAR && (inquote || (c != SPACECHAR && c != TABCHAR))) );

	if ( c == NULCHAR ) {
		p--;
	} else {
		if (args)
			*(args-1) = NULCHAR;
	}

	inquote = 0;

	/* loop on each argument */
	for(;;) {

		if ( *p ) {
			while (*p == SPACECHAR || *p == TABCHAR)
				++p;
		}

		if (*p == NULCHAR)
			break;              /* end of args */

		/* scan an argument */
		if (argv)
			*argv++ = args;     /* store ptr to arg */
		++*numargs;

#ifdef WILDCARD
		/* To handle later wild card expansion, we prefix each entry by
		it's first character before quote handling.  This is done
		so _[w]cwild() knows whether to expand an entry or not. */
		if (args)
			*args++ = *p;
		++*numchars;

#endif  /* WILDCARD */

		/* loop through scanning one argument */
		for (;;) {
			copychar = 1;
			/* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
			2N+1 backslashes + " ==> N backslashes + literal "
			N backslashes ==> N backslashes */
			numslash = 0;
			while (*p == SLASHCHAR) {
				/* count number of backslashes for use below */
				++p;
				++numslash;
			}
			if (*p == DQUOTECHAR) {
				/* if 2N backslashes before, start/end quote, otherwise
				copy literally */
				if (numslash % 2 == 0) {
					if (inquote && p[1] == DQUOTECHAR) {
						p++;    /* Double quote inside quoted string */
					} else {    /* skip first quote char and copy second */
						copychar = 0;       /* don't copy quote */
						inquote = !inquote;
					}
				}
				numslash /= 2;          /* divide numslash by two */
			}

			/* copy slashes */
			while (numslash--) {
				if (args)
					*args++ = SLASHCHAR;
				++*numchars;
			}

			/* if at end of arg, break loop */
			if (*p == NULCHAR || (!inquote && (*p == SPACECHAR || *p == TABCHAR)))
				break;

			/* copy character into argument */
#ifdef _MBCS
			if (copychar) {
				if (args) {
					if (_ismbblead(*p)) {
						*args++ = *p++;
						++*numchars;
					}
					*args++ = *p;
				} else {
					if (_ismbblead(*p)) {
						++p;
						++*numchars;
					}
				}
				++*numchars;
			}
			++p;
#else  /* _MBCS */
			if (copychar) {
				if (args)
					*args++ = *p;
				++*numchars;
			}
			++p;
#endif  /* _MBCS */
		}

		/* null-terminate the argument */

		if (args)
			*args++ = NULCHAR;          /* terminate string */
		++*numchars;
	}

	/* We put one last argument in -- a null ptr */
	if (argv)
		*argv++ = NULL;
	++*numargs;
}

#define MAX_ARGUMENTS 128

static TCHAR **parseargstring (const TCHAR *s)
{
	TCHAR **p;
	int numa, numc;

	if (_tcslen (s) == 0)
		return NULL;
	wparse_cmdline (s, NULL, NULL, &numa, &numc);
	numa++;
	p = (TCHAR**)xcalloc (uae_u8, numa * sizeof (TCHAR*) + numc * sizeof (TCHAR));
	wparse_cmdline (s, (wchar_t **)p, (wchar_t *)(((char *)p) + numa * sizeof(wchar_t *)), &numa, &numc);
	if (numa > MAX_ARGUMENTS) {
		p[MAX_ARGUMENTS] = NULL;
		numa = MAX_ARGUMENTS;
	}
	TCHAR **dstp = xcalloc (TCHAR*, MAX_ARGUMENTS + 1);
	for (int i = 0; p[i]; i++)
		dstp[i] = my_strdup (p[i]);
	xfree (p);
	return dstp;
}


static void shellexecute (const TCHAR *command)
{
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	TCHAR **arg;
	int i, j, k, stop;

	if (_tcslen (command) == 0)
		return;
	i = j = 0;
	stop = 0;
	arg = parseargstring (command);
	while (!stop) {
		TCHAR *cmd, *exec;
		int len = 1;
		j = i;
		while (arg[i] && _tcscmp (arg[i], L";")) {
			len += _tcslen (arg[i]) + 3;
			i++;
		}
		exec = NULL;
		cmd = xcalloc (TCHAR, len);
		for (k = j; k < i; k++) {
			int quote = 0;
			if (_tcslen (cmd) > 0)
				_tcscat (cmd, L" ");
			if (_tcschr (arg[k], ' '))
				quote = 1;
			if (quote)
				_tcscat (cmd, L"\"");
			_tcscat (cmd, arg[k]);
			if (quote)
				_tcscat (cmd, L"\"");
			if (!exec && !_tcsicmp (cmd, L"cmd.exe")) {
				int size;
				size = GetEnvironmentVariable (L"ComSpec", NULL, 0);
				if (size > 0) {
					exec = xcalloc (TCHAR, size + 1);
					GetEnvironmentVariable (L"ComSpec", exec, size);
				}
				cmd[0] = 0;
			}
		}
		if (arg[i++] == 0)
			stop = 1;
		si.cb = sizeof si;
		//si.wShowWindow = SW_HIDE;
		//si.dwFlags = STARTF_USESHOWWINDOW;
		if (CreateProcess (exec,
			cmd,
			NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
				WaitForSingleObject (pi.hProcess, INFINITE);
				CloseHandle (pi.hProcess);
				CloseHandle (pi.hThread);
		} else {
			write_log (L"CreateProcess('%s' '%s') failed, %d\n",
				exec, cmd, GetLastError ());
		}
		xfree (exec);
		xfree (cmd);
	}
	for (i = 0; arg && arg[i]; i++)
		xfree (arg[i]);
	xfree (arg);
}

void target_run (void)
{
	shellexecute (currprefs.win32_commandpathstart);
}
void target_quit (void)
{
	shellexecute (currprefs.win32_commandpathend);
}

void target_fixup_options (struct uae_prefs *p)
{
#ifdef RETROPLATFORM
	rp_fixup_options (p);
#endif
}

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

static const TCHAR *scsimode[] = { L"SCSIEMU", L"SPTI", L"SPTI+SCSISCAN", L"AdaptecASPI", L"NeroASPI", L"FrogASPI", NULL };
static const TCHAR *statusbarmode[] = { L"none", L"normal", L"extended", NULL };

void target_save_options (struct zfile *f, struct uae_prefs *p)
{
	cfgfile_target_dwrite_bool (f, L"middle_mouse", p->win32_middle_mouse);
	cfgfile_target_dwrite_bool (f, L"logfile", p->win32_logfile);
	cfgfile_target_dwrite_bool (f, L"map_drives", p->win32_automount_drives);
	cfgfile_target_dwrite_bool (f, L"map_drives_auto", p->win32_automount_removable);
	cfgfile_target_dwrite_bool (f, L"map_cd_drives", p->win32_automount_cddrives);
	cfgfile_target_dwrite_bool (f, L"map_net_drives", p->win32_automount_netdrives);
	cfgfile_target_dwrite_bool (f, L"map_removable_drives", p->win32_automount_removabledrives);
	serdevtoname (p->sername);
	cfgfile_target_dwrite_str (f, L"serial_port", p->sername[0] ? p->sername : L"none");
	sernametodev (p->sername);
	cfgfile_target_dwrite_str (f, L"parallel_port", p->prtname[0] ? p->prtname : L"none");

	cfgfile_target_dwrite (f, L"active_priority", L"%d", priorities[p->win32_active_priority].value);
	cfgfile_target_dwrite (f, L"inactive_priority", L"%d", priorities[p->win32_inactive_priority].value);
	cfgfile_target_dwrite_bool (f, L"inactive_nosound", p->win32_inactive_nosound);
	cfgfile_target_dwrite_bool (f, L"inactive_pause", p->win32_inactive_pause);
	cfgfile_target_dwrite (f, L"iconified_priority", L"%d", priorities[p->win32_iconified_priority].value);
	cfgfile_target_dwrite_bool (f, L"iconified_nosound", p->win32_iconified_nosound);
	cfgfile_target_dwrite_bool (f, L"iconified_pause", p->win32_iconified_pause);
	cfgfile_target_dwrite_bool (f, L"inactive_iconify", p->win32_minimize_inactive);

	cfgfile_target_dwrite_bool (f, L"ctrl_f11_is_quit", p->win32_ctrl_F11_is_quit);

	cfgfile_target_dwrite (f, L"midiout_device", L"%d", p->win32_midioutdev);
	cfgfile_target_dwrite (f, L"midiin_device", L"%d", p->win32_midiindev);
	if (p->win32_midioutdev < -1)
		cfgfile_target_dwrite_str (f, L"midiout_device_name", L"none");
	else if (p->win32_midioutdev == -1)
		cfgfile_target_dwrite_str (f, L"midiout_device_name", L"default");
	else if (p->win32_midioutdev >= 0 && p->win32_midioutdev < MAX_MIDI_PORTS) {
		if (midioutportinfo[p->win32_midioutdev + 1].name == NULL)
			p->win32_midioutdev = -1;
		else
			cfgfile_target_dwrite_str (f, L"midiout_device_name", midioutportinfo[p->win32_midioutdev + 1].name);
	}
	if (p->win32_midiindev < 0)
		cfgfile_target_dwrite_str (f, L"midiin_device_name", L"none");
	else if (p->win32_midiindev >= 0 && p->win32_midiindev < MAX_MIDI_PORTS) {
		if (midiinportinfo[p->win32_midiindev].name == NULL)
			p->win32_midiindev = -1;
		else
			cfgfile_target_dwrite_str (f, L"midiin_device_name", midiinportinfo[p->win32_midiindev].name);
	}
			
	cfgfile_target_dwrite_bool (f, L"rtg_match_depth", p->win32_rtgmatchdepth);
	cfgfile_target_dwrite_bool (f, L"rtg_scale_small", p->win32_rtgscaleifsmall);
	cfgfile_target_dwrite_bool (f, L"rtg_scale_allow", p->win32_rtgallowscaling);
	cfgfile_target_dwrite (f, L"rtg_scale_aspect_ratio", L"%d:%d",
		p->win32_rtgscaleaspectratio >= 0 ? (p->win32_rtgscaleaspectratio >> 8) : -1,
		p->win32_rtgscaleaspectratio >= 0 ? (p->win32_rtgscaleaspectratio & 0xff) : -1);
	if (p->win32_rtgvblankrate <= 0)
		cfgfile_target_dwrite_str (f, L"rtg_vblank", p->win32_rtgvblankrate == -1 ? L"real" : (p->win32_rtgvblankrate == -2 ? L"disabled" : L"chipset"));
	else
		cfgfile_target_dwrite (f, L"rtg_vblank", L"%d", p->win32_rtgvblankrate);
	cfgfile_target_dwrite_bool (f, L"borderless", p->win32_borderless);
	cfgfile_target_dwrite_str (f, L"uaescsimode", scsimode[p->win32_uaescsimode]);
	cfgfile_target_dwrite_str (f, L"statusbar", statusbarmode[p->win32_statusbar]);
	cfgfile_target_dwrite (f, L"soundcard", L"%d", p->win32_soundcard);
	if (sound_devices[p->win32_soundcard].cfgname)
		cfgfile_target_dwrite_str (f, L"soundcardname", sound_devices[p->win32_soundcard].cfgname);
	cfgfile_target_dwrite_bool (f, L"soundcard_exclusive", p->win32_soundexclusive);
	if (p->win32_samplersoundcard >= 0) {
		cfgfile_target_dwrite (f, L"samplersoundcard", L"%d", p->win32_samplersoundcard);
		if (record_devices[p->win32_samplersoundcard].cfgname)
			cfgfile_target_dwrite_str (f, L"samplersoundcardname", record_devices[p->win32_samplersoundcard].cfgname);
	}

	cfgfile_target_dwrite (f, L"cpu_idle", L"%d", p->cpu_idle);
	cfgfile_target_dwrite_bool (f, L"notaskbarbutton", p->win32_notaskbarbutton);
	cfgfile_target_dwrite_bool (f, L"always_on_top", p->win32_alwaysontop);
	cfgfile_target_dwrite_bool (f, L"no_recyclebin", p->win32_norecyclebin);
	cfgfile_target_dwrite (f, L"specialkey", L"0x%x", p->win32_specialkey);
	if (p->win32_guikey >= 0)
		cfgfile_target_dwrite (f, L"guikey", L"0x%x", p->win32_guikey);
	cfgfile_target_dwrite (f, L"kbledmode", L"%d", p->win32_kbledmode);
	cfgfile_target_dwrite_bool (f, L"powersavedisabled", p->win32_powersavedisabled);
	cfgfile_target_dwrite_str (f, L"exec_before", p->win32_commandpathstart);
	cfgfile_target_dwrite_str (f, L"exec_after", p->win32_commandpathend);
	cfgfile_target_dwrite_str (f, L"parjoyport0", p->win32_parjoyport0);
	cfgfile_target_dwrite_str (f, L"parjoyport1", p->win32_parjoyport1);

}

static int fetchpri (int pri, int defpri)
{
	int i = 0;
	while (priorities[i].name) {
		if (priorities[i].value == pri)
			return i;
		i++;
	}
	return defpri;
}

TCHAR *target_expand_environment (const TCHAR *path)
{
	if (!path)
		return NULL;
	int len = ExpandEnvironmentStrings (path, NULL, 0);
	if (len <= 0)
		return my_strdup (path);
	TCHAR *s = xmalloc (TCHAR, len + 1);
	ExpandEnvironmentStrings (path, s, len);
	return s;
}

static const TCHAR *obsolete[] = {
	L"killwinkeys", L"sound_force_primary", L"iconified_highpriority",
	L"sound_sync", L"sound_tweak", L"directx6", L"sound_style",
	L"file_path", L"iconified_nospeed", L"activepriority", L"magic_mouse",
	L"filesystem_codepage", L"aspi", L"no_overlay",
	0
};

int target_parse_option (struct uae_prefs *p, const TCHAR *option, const TCHAR *value)
{
	TCHAR tmpbuf[CONFIG_BLEN];
	int i, v;

	int result = (cfgfile_yesno (option, value, L"middle_mouse", &p->win32_middle_mouse)
		|| cfgfile_yesno (option, value, L"map_drives", &p->win32_automount_drives)
		|| cfgfile_yesno (option, value, L"map_drives_auto", &p->win32_automount_removable)
		|| cfgfile_yesno (option, value, L"map_cd_drives", &p->win32_automount_cddrives)
		|| cfgfile_yesno (option, value, L"map_net_drives", &p->win32_automount_netdrives)
		|| cfgfile_yesno (option, value, L"map_removable_drives", &p->win32_automount_removabledrives)
		|| cfgfile_yesno (option, value, L"logfile", &p->win32_logfile)
		|| cfgfile_yesno (option, value, L"networking", &p->socket_emu)
		|| cfgfile_yesno (option, value, L"borderless", &p->win32_borderless)
		|| cfgfile_yesno (option, value, L"inactive_pause", &p->win32_inactive_pause)
		|| cfgfile_yesno (option, value, L"inactive_nosound", &p->win32_inactive_nosound)
		|| cfgfile_yesno (option, value, L"iconified_pause", &p->win32_iconified_pause)
		|| cfgfile_yesno (option, value, L"iconified_nosound", &p->win32_iconified_nosound)
		|| cfgfile_yesno (option, value, L"ctrl_f11_is_quit", &p->win32_ctrl_F11_is_quit)
		|| cfgfile_yesno (option, value, L"no_recyclebin", &p->win32_norecyclebin)
		|| cfgfile_intval (option, value, L"midi_device", &p->win32_midioutdev, 1)
		|| cfgfile_intval (option, value, L"midiout_device", &p->win32_midioutdev, 1)
		|| cfgfile_intval (option, value, L"midiin_device", &p->win32_midiindev, 1)
		|| cfgfile_intval (option, value, L"soundcard", &p->win32_soundcard, 1)
		|| cfgfile_intval (option, value, L"samplersoundcard", &p->win32_samplersoundcard, 1)
		|| cfgfile_yesno (option, value, L"soundcard_exclusive", &p->win32_soundexclusive)
		|| cfgfile_yesno (option, value, L"notaskbarbutton", &p->win32_notaskbarbutton)
		|| cfgfile_yesno (option, value, L"always_on_top", &p->win32_alwaysontop)
		|| cfgfile_yesno (option, value, L"powersavedisabled", &p->win32_powersavedisabled)
		|| cfgfile_string (option, value, L"exec_before", p->win32_commandpathstart, sizeof p->win32_commandpathstart / sizeof (TCHAR))
		|| cfgfile_string (option, value, L"exec_after", p->win32_commandpathend, sizeof p->win32_commandpathend / sizeof (TCHAR))
		|| cfgfile_string (option, value, L"parjoyport0", p->win32_parjoyport0, sizeof p->win32_parjoyport0 / sizeof (TCHAR))
		|| cfgfile_string (option, value, L"parjoyport1", p->win32_parjoyport1, sizeof p->win32_parjoyport1 / sizeof (TCHAR))
		|| cfgfile_intval (option, value, L"specialkey", &p->win32_specialkey, 1)
		|| cfgfile_intval (option, value, L"guikey", &p->win32_guikey, 1)
		|| cfgfile_intval (option, value, L"kbledmode", &p->win32_kbledmode, 1)
		|| cfgfile_intval (option, value, L"cpu_idle", &p->cpu_idle, 1));

	if (cfgfile_yesno (option, value, L"rtg_match_depth", &p->win32_rtgmatchdepth))
		return 1;
	if (cfgfile_yesno (option, value, L"rtg_scale_small", &p->win32_rtgscaleifsmall))
		return 1;
	if (cfgfile_yesno (option, value, L"rtg_scale_allow", &p->win32_rtgallowscaling))
		return 1;

	if (cfgfile_string (option, value, L"soundcardname", tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		int i, num;

		num = p->win32_soundcard;
		p->win32_soundcard = -1;
		for (i = 0; sound_devices[i].cfgname; i++) {
			if (i < num)
				continue;
			if (!_tcscmp (sound_devices[i].cfgname, tmpbuf)) {
				p->win32_soundcard = i;
				break;
			}
		}
		if (p->win32_soundcard < 0) {
			for (i = 0; sound_devices[i].cfgname; i++) {
				if (!_tcscmp (sound_devices[i].cfgname, tmpbuf)) {
					p->win32_soundcard = i;
					break;
				}
			}
		}
		if (p->win32_soundcard < 0)
			p->win32_soundcard = num;
		return 1;
	}
	if (cfgfile_string (option, value, L"samplersoundcardname", tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		int i, num;

		num = p->win32_samplersoundcard;
		p->win32_samplersoundcard = -1;
		for (i = 0; record_devices[i].cfgname; i++) {
			if (i < num)
				continue;
			if (!_tcscmp (record_devices[i].cfgname, tmpbuf)) {
				p->win32_samplersoundcard = i;
				break;
			}
		}
		if (p->win32_samplersoundcard < 0) {
			for (i = 0; record_devices[i].cfgname; i++) {
				if (!_tcscmp (record_devices[i].cfgname, tmpbuf)) {
					p->win32_samplersoundcard = i;
					break;
				}
			}
		}
		return 1;
	}

	if (cfgfile_string (option, value, L"rtg_vblank", tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		if (!_tcscmp (tmpbuf, L"real")) {
			p->win32_rtgvblankrate = -1;
			return 1;
		}
		if (!_tcscmp (tmpbuf, L"disabled")) {
			p->win32_rtgvblankrate = -2;
			return 1;
		}
		if (!_tcscmp (tmpbuf, L"chipset")) {
			p->win32_rtgvblankrate = 0;
			return 1;
		}
		p->win32_rtgvblankrate = _tstol (tmpbuf);
		return 1;
	}

	if (cfgfile_string (option, value, L"rtg_scale_aspect_ratio", tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		int v1, v2;
		TCHAR *s;

		p->gfx_filter_aspect = -1;
		v1 = _tstol (tmpbuf);
		s = _tcschr (tmpbuf, ':');
		if (s) {
			v2 = _tstol (s + 1);
			if (v1 < 0 || v2 < 0)
				p->gfx_filter_aspect = -1;
			else if (v1 == 0 || v2 == 0)
				p->gfx_filter_aspect = 0;
			else
				p->gfx_filter_aspect = (v1 << 8) | v2;
		}
		return 1;
	}

	if (cfgfile_strval (option, value, L"uaescsimode", &p->win32_uaescsimode, scsimode, 0)) {
		// force SCSIEMU if pre 2.3 configuration
		if (p->config_version < ((2 << 16) | (3 << 8)))
			p->win32_uaescsimode = UAESCSI_CDEMU;
		return 1;
	}

	if (cfgfile_strval (option, value, L"statusbar", &p->win32_statusbar, statusbarmode, 0))
		return 1;

	if (cfgfile_intval (option, value, L"active_priority", &v, 1)) {
		p->win32_active_priority = fetchpri (v, 1);
		return 1;
	}
	if (cfgfile_intval (option, value, L"activepriority", &v, 1)) {
		p->win32_active_priority = fetchpri (v, 1);
		return 1;
	}
	if (cfgfile_intval (option, value, L"inactive_priority", &v, 1)) {
		p->win32_inactive_priority = fetchpri (v, 1);
		return 1;
	}
	if (cfgfile_intval (option, value, L"iconified_priority", &v, 1)) {
		p->win32_iconified_priority = fetchpri (v, 2);
		return 1;
	}
	
	if (cfgfile_yesno (option, value, L"inactive_iconify", &p->win32_minimize_inactive))
		return 1;

	if (cfgfile_string (option, value, L"serial_port", &p->sername[0], 256)) {
		sernametodev (p->sername);
		if (p->sername[0])
			p->use_serial = 1;
		else
			p->use_serial = 0;
		return 1;
	}

	if (cfgfile_string (option, value, L"parallel_port", &p->prtname[0], 256)) {
		if (!_tcscmp (p->prtname, L"none"))
			p->prtname[0] = 0;
		if (!_tcscmp (p->prtname, L"default")) {
			p->prtname[0] = 0;
			DWORD size = 256;
			GetDefaultPrinter (p->prtname, &size);
		}
		return 1;
	}

	if (cfgfile_string (option, value, L"midiout_device_name", tmpbuf, 256)) {
		p->win32_midioutdev = -2;
		if (!_tcsicmp (tmpbuf, L"default") || (midioutportinfo[0].name && !_tcsicmp (tmpbuf, midioutportinfo[0].name)))
			p->win32_midioutdev = -1;
		for (int i = 0; midioutportinfo[i].name; i++) {
			if (!_tcsicmp (midioutportinfo[i].name, tmpbuf)) {
				p->win32_midioutdev = i - 1;
			}
		}
		return 1;
	}
	if (cfgfile_string (option, value, L"midiin_device_name", tmpbuf, 256)) {
		p->win32_midiindev = -1;
		for (int i = 0; midiinportinfo[i].name; i++) {
			if (!_tcsicmp (midiinportinfo[i].name, tmpbuf)) {
				p->win32_midiindev = i;
			}
		}
		return 1;
	}


	i = 0;
	while (obsolete[i]) {
		if (!strcasecmp (obsolete[i], option)) {
			write_log (L"obsolete config entry '%s'\n", option);
			return 1;
		}
		i++;
	}

	return result;
}

static void createdir (const TCHAR *path)
{
	CreateDirectory (path, NULL);
}

void fetch_saveimagepath (TCHAR *out, int size, int dir)
{
	fetch_path (L"SaveimagePath", out, size);
	if (dir) {
		out[_tcslen (out) - 1] = 0;
		createdir (out);
		fetch_path (L"SaveimagePath", out, size);
	}
}
void fetch_configurationpath (TCHAR *out, int size)
{
	fetch_path (L"ConfigurationPath", out, size);
}
void fetch_screenshotpath (TCHAR *out, int size)
{
	fetch_path (L"ScreenshotPath", out, size);
}
void fetch_ripperpath (TCHAR *out, int size)
{
	fetch_path (L"RipperPath", out, size);
}
void fetch_statefilepath (TCHAR *out, int size)
{
	fetch_path (L"StatefilePath", out, size);
}
void fetch_inputfilepath (TCHAR *out, int size)
{
	fetch_path (L"InputPath", out, size);
}
void fetch_datapath (TCHAR *out, int size)
{
	fetch_path (NULL, out, size);
}

static int isfilesindir (const TCHAR *p)
{
	WIN32_FIND_DATA fd;
	HANDLE h;
	TCHAR path[MAX_DPATH];
	int i = 0;
	DWORD v;

	v = GetFileAttributes (p);
	if (v == INVALID_FILE_ATTRIBUTES || !(v & FILE_ATTRIBUTE_DIRECTORY))
		return 0;
	_tcscpy (path, p);
	_tcscat (path, L"\\*.*");
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
	return 0;
}

void fetch_path (const TCHAR *name, TCHAR *out, int size)
{
	int size2 = size;

	_tcscpy (out, start_path_data);
	if (!name) {
		fullpath (out, size);
		return;
	}
	if (!_tcscmp (name, L"FloppyPath"))
		_tcscat (out, L"..\\shared\\adf\\");
	if (!_tcscmp (name, L"CDPath"))
		_tcscat (out, L"..\\shared\\cd\\");
	if (!_tcscmp (name, L"hdfPath"))
		_tcscat (out, L"..\\shared\\hdf\\");
	if (!_tcscmp (name, L"KickstartPath"))
		_tcscat (out, L"..\\shared\\rom\\");
	if (!_tcscmp (name, L"ConfigurationPath"))
		_tcscat (out, L"Configurations\\");
	if (!_tcscmp (name, L"StatefilePath"))
		_tcscat (out, L"Savestates\\");
	if (!_tcscmp (name, L"InputPath"))
		_tcscat (out, L"Inputrecordings\\");
	if (start_data >= 0)
		regquerystr (NULL, name, out, &size); 
	if (GetFileAttributes (out) == INVALID_FILE_ATTRIBUTES)
		_tcscpy (out, start_path_data);
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
	if (!_tcscmp (name, L"KickstartPath")) {
		DWORD v = GetFileAttributes (out);
		if (v == INVALID_FILE_ATTRIBUTES || !(v & FILE_ATTRIBUTE_DIRECTORY))
			_tcscpy (out, start_path_data);
	}
	fixtrailing (out);
	fullpath (out, size);
}

int get_rom_path (TCHAR *out, pathtype mode)
{
	TCHAR tmp[MAX_DPATH];

	tmp[0] = 0;
	switch (mode)
	{
	case PATH_TYPE_DEFAULT:
		{
			if (!_tcscmp (start_path_data, start_path_exe))
				_tcscpy (tmp, L".\\");
			else
				_tcscpy (tmp, start_path_data);
			if (GetFileAttributes (tmp) != INVALID_FILE_ATTRIBUTES) {
				TCHAR tmp2[MAX_DPATH];
				_tcscpy (tmp2, tmp);
				_tcscat (tmp2, L"rom");
				if (GetFileAttributes (tmp2) != INVALID_FILE_ATTRIBUTES) {
					_tcscpy (tmp, tmp2);
				} else {
					_tcscpy (tmp2, tmp);
					_tcscpy (tmp2, L"roms");
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
			TCHAR tmp2[MAX_DPATH];
			_tcscpy (tmp2, start_path_new1);
			_tcscat (tmp2, L"..\\system\\rom");
			if (isfilesindir (tmp2)) {
				_tcscpy (tmp, tmp2);
				break;
			}
			_tcscpy (tmp2, start_path_new1);
			_tcscat (tmp2, L"..\\shared\\rom");
			if (isfilesindir (tmp2)) {
				_tcscpy (tmp, tmp2);
				break;
			}
		}
		break;
	case PATH_TYPE_AMIGAFOREVERDATA:
		{
			TCHAR tmp2[MAX_DPATH];
			_tcscpy (tmp2, start_path_new2);
			_tcscat (tmp2, L"system\\rom");
			if (isfilesindir (tmp2)) {
				_tcscpy (tmp, tmp2);
				break;
			}
			_tcscpy (tmp2, start_path_new2);
			_tcscat (tmp2, L"shared\\rom");
			if (isfilesindir (tmp2)) {
				_tcscpy (tmp, tmp2);
				break;
			}
		}
		break;
	default:
		return -1;
	}
	if (isfilesindir (tmp)) {
		_tcscpy (out, tmp);
		fixtrailing (out);
	}
	if (out[0]) {
		fullpath (out, MAX_DPATH);
	}
	return out[0] ? 1 : 0;
}

void set_path (const TCHAR *name, TCHAR *path, pathtype mode)
{
	TCHAR tmp[MAX_DPATH];

	if (!path) {
		if (!_tcscmp (start_path_data, start_path_exe))
			_tcscpy (tmp, L".\\");
		else
			_tcscpy (tmp, start_path_data);
		if (!_tcscmp (name, L"KickstartPath"))
			_tcscat (tmp, L"Roms");
		if (!_tcscmp (name, L"ConfigurationPath"))
			_tcscat (tmp, L"Configurations");
		if (!_tcscmp (name, L"ScreenshotPath"))
			_tcscat (tmp, L"Screenshots");
		if (!_tcscmp (name, L"StatefilePath"))
			_tcscat (tmp, L"Savestates");
		if (!_tcscmp (name, L"SaveimagePath"))
			_tcscat (tmp, L"SaveImages");
		if (!_tcscmp (name, L"InputPath"))
			_tcscat (tmp, L"Inputrecordings");
	} else {
		_tcscpy (tmp, path);
	}
	stripslashes (tmp);
	if (!_tcscmp (name, L"KickstartPath")) {
		DWORD v = GetFileAttributes (tmp);
		if (v == INVALID_FILE_ATTRIBUTES || !(v & FILE_ATTRIBUTE_DIRECTORY))
			get_rom_path (tmp, PATH_TYPE_DEFAULT);
		if (mode == PATH_TYPE_NEWAF) {
			get_rom_path (tmp, PATH_TYPE_NEWAF);
		} else if (mode == PATH_TYPE_AMIGAFOREVERDATA) {
			get_rom_path (tmp, PATH_TYPE_AMIGAFOREVERDATA);
		}
	}
	fixtrailing (tmp);
	fullpath (tmp, sizeof tmp / sizeof (TCHAR));
	regsetstr (NULL, name, tmp);
}
void set_path (const TCHAR *name, TCHAR *path)
{
	set_path (name, path, PATH_TYPE_DEFAULT);
}

static void initpath (const TCHAR *name, TCHAR *path)
{
	if (regexists (NULL, name))
		return;
	set_path (name, NULL);
}

static void romlist_add2 (const TCHAR *path, struct romdata *rd)
{
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

	romlist_clear ();
	exists = regexiststree (NULL, L"DetectedROMs");
	fkey = regcreatetree (NULL, L"DetectedROMs");
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
	if (_tcsncmp (vs, L"WinUAE ", 7))
		return 0;
	vs += 7;
	ver = parseversion (&vs) << 16;
	ver |= parseversion (&vs) << 8;
	ver |= parseversion (&vs);
	if (ver >= ((UAEMAJOR << 16) | (UAEMINOR << 8) | UAESUBREV))
		return 0;
	return 1;
}

static int shell_deassociate (const TCHAR *extension)
{
	HKEY rkey;
	const TCHAR *progid = L"WinUAE";
	int def = !_tcscmp (extension, L".uae");
	TCHAR rpath1[MAX_DPATH], rpath2[MAX_DPATH], progid2[MAX_DPATH];
	UAEREG *fkey;

	if (extension == NULL || _tcslen (extension) < 1 || extension[0] != '.')
		return 0;
	_tcscpy (progid2, progid);
	_tcscat (progid2, extension);
	if (os_winnt_admin > 1)
		rkey = HKEY_LOCAL_MACHINE;
	else
		rkey = HKEY_CURRENT_USER;

	_tcscpy (rpath1, L"Software\\Classes\\");
	_tcscpy (rpath2, rpath1);
	_tcscat (rpath2, extension);
	RegDeleteKey (rkey, rpath2);
	_tcscpy (rpath2, rpath1);
	_tcscat (rpath2, progid);
	if (!def)
		_tcscat (rpath2, extension);
	SHDeleteKey (rkey, rpath2);
	fkey = regcreatetree (NULL, L"FileAssociations");
	regdelete (fkey, extension);
	regclosetree (fkey);
	return 1;
}

static int shell_associate_2 (const TCHAR *extension, TCHAR *shellcommand, TCHAR *command, struct contextcommand *cc, const TCHAR *perceivedtype,
	const TCHAR *description, const TCHAR *ext2, int icon)
{
	TCHAR rpath1[MAX_DPATH], rpath2[MAX_DPATH], progid2[MAX_DPATH];
	HKEY rkey, key1, key2;
	DWORD disposition;
	const TCHAR *progid = L"WinUAE";
	int def = !_tcscmp (extension, L".uae");
	const TCHAR *defprogid;
	UAEREG *fkey;

	_tcscpy (progid2, progid);
	_tcscat (progid2, ext2 ? ext2 : extension);
	if (os_winnt_admin > 1)
		rkey = HKEY_LOCAL_MACHINE;
	else
		rkey = HKEY_CURRENT_USER;
	defprogid = def ? progid : progid2;

	_tcscpy (rpath1, L"Software\\Classes\\");
	_tcscpy (rpath2, rpath1);
	_tcscat (rpath2, extension);
	if (RegCreateKeyEx (rkey, rpath2, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_WRITE | KEY_READ, NULL, &key1, &disposition) == ERROR_SUCCESS) {
			RegSetValueEx (key1, L"", 0, REG_SZ, (CONST BYTE *)defprogid, (_tcslen (defprogid) + 1) * sizeof (TCHAR));
			if (perceivedtype)
				RegSetValueEx (key1, L"PerceivedType", 0, REG_SZ, (CONST BYTE *)perceivedtype, (_tcslen (perceivedtype) + 1) * sizeof (TCHAR));
			RegCloseKey (key1);
	}
	_tcscpy (rpath2, rpath1);
	_tcscat (rpath2, progid);
	if (!def)
		_tcscat (rpath2, ext2 ? ext2 : extension);
	if (description) {
		if (RegCreateKeyEx (rkey, rpath2, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &key1, &disposition) == ERROR_SUCCESS) {
			TCHAR tmp[MAX_DPATH];
			RegSetValueEx (key1, L"", 0, REG_SZ, (CONST BYTE *)description, (_tcslen (description) + 1) * sizeof (TCHAR));
			RegSetValueEx (key1, L"AppUserModelID", 0, REG_SZ, (CONST BYTE *)WINUAEAPPNAME, (_tcslen (WINUAEAPPNAME) + 1) * sizeof (TCHAR));
			_tcscpy (tmp, rpath2);
			_tcscat (tmp, L"\\CurVer");
			if (RegCreateKeyEx (rkey, tmp, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &key2, &disposition) == ERROR_SUCCESS) {
				RegSetValueEx (key2, L"", 0, REG_SZ, (CONST BYTE *)defprogid, (_tcslen (defprogid) + 1) * sizeof (TCHAR));
				RegCloseKey (key2);
			}
			if (icon) {
				_tcscpy (tmp, rpath2);
				_tcscat (tmp, L"\\DefaultIcon");
				if (RegCreateKeyEx (rkey, tmp, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &key2, &disposition) == ERROR_SUCCESS) {
					_stprintf (tmp, L"%s,%d", _wpgmptr, -icon);
					RegSetValueEx (key2, L"", 0, REG_SZ, (CONST BYTE *)tmp, (_tcslen (tmp) + 1) * sizeof (TCHAR));
					RegCloseKey (key2);
				}
			}
			RegCloseKey (key1);
		}
	}
	cc = NULL;
	struct contextcommand ccs[2];
	if ((command || shellcommand)) {
		ccs[0].command = command;
		ccs[0].shellcommand = shellcommand;
		ccs[1].command = NULL;
		cc = &ccs[0];
	}
	if (cc) {
		TCHAR path2[MAX_DPATH];
		for (int i = 0; cc[i].command; i++) {
			_tcscpy (path2, rpath2);
			_tcscat (path2, L"\\shell\\");
			if (cc[i].shellcommand)
				_tcscat (path2, cc[i].shellcommand);
			else
				_tcscat (path2, L"open");
			if (cc[i].icon) {
				if (RegCreateKeyEx (rkey, path2, 0, NULL, REG_OPTION_NON_VOLATILE,
					KEY_WRITE | KEY_READ, NULL, &key1, &disposition) == ERROR_SUCCESS) {
						TCHAR tmp[MAX_DPATH];
						_stprintf (tmp, L"%s,%d", _wpgmptr, -cc[i].icon);
						RegSetValueEx (key1, L"Icon", 0, REG_SZ, (CONST BYTE *)tmp, (_tcslen (tmp) + 1) * sizeof (TCHAR));
						RegCloseKey (key1);
				}
			}
			_tcscat (path2, L"\\command");
			if (RegCreateKeyEx (rkey, path2, 0, NULL, REG_OPTION_NON_VOLATILE,
				KEY_WRITE | KEY_READ, NULL, &key1, &disposition) == ERROR_SUCCESS) {
					TCHAR path[MAX_DPATH];
					_stprintf (path, L"\"%sWinUAE.exe\" %s", start_path_exe, cc[i].command);
					RegSetValueEx (key1, L"", 0, REG_SZ, (CONST BYTE *)path, (_tcslen (path) + 1) * sizeof (TCHAR));
					RegCloseKey (key1);
			}
		}
	}
	fkey = regcreatetree (NULL, L"FileAssociations");
	regsetstr (fkey, extension, L"");
	regclosetree (fkey);
	return 1;
}
static int shell_associate (const TCHAR *extension, TCHAR *command, struct contextcommand *cc, const TCHAR *perceivedtype, const TCHAR *description, const TCHAR *ext2, int icon)
{
	int v = shell_associate_2 (extension, NULL, command, cc, perceivedtype, description, ext2, icon);
	if (!_tcscmp (extension, L".uae"))
		shell_associate_2 (extension, L"edit", L"-f \"%1\" -s use_gui=yes", NULL, L"text", description, NULL, 0);
	return v;
}

static int shell_associate_is (const TCHAR *extension)
{
	TCHAR rpath1[MAX_DPATH], rpath2[MAX_DPATH];
	TCHAR progid2[MAX_DPATH], tmp[MAX_DPATH];
	DWORD size;
	HKEY rkey, key1;
	const TCHAR *progid = L"WinUAE";
	int def = !_tcscmp (extension, L".uae");

	_tcscpy (progid2, progid);
	_tcscat (progid2, extension);
	if (os_winnt_admin > 1)
		rkey = HKEY_LOCAL_MACHINE;
	else
		rkey = HKEY_CURRENT_USER;

	_tcscpy (rpath1, L"Software\\Classes\\");
	_tcscpy (rpath2, rpath1);
	_tcscat (rpath2, extension);
	size = sizeof tmp / sizeof (TCHAR);
	if (RegOpenKeyEx (rkey, rpath2, 0, KEY_READ, &key1) == ERROR_SUCCESS) {
		if (RegQueryValueEx (key1, NULL, NULL, NULL, (LPBYTE)tmp, &size) == ERROR_SUCCESS) {
			if (_tcscmp (tmp, def ? progid : progid2)) {
				RegCloseKey (key1);
				return 0;
			}
		}
		RegCloseKey (key1);
	}
	_tcscpy (rpath2, rpath1);
	_tcscat (rpath2, progid);
	if (!def)
		_tcscat (rpath2, extension);
	if (RegOpenKeyEx (rkey, rpath2, 0, KEY_READ, &key1) == ERROR_SUCCESS) {
		RegCloseKey (key1);
		return 1;
	}
	return 0;
}
static struct contextcommand cc_cd[] = {
	{ L"CDTV", L"-cdimage=\"%1\" -s use_gui=no -cfgparam=quickstart=CDTV,0", IDI_APPICON },
	{ L"CD32", L"-cdimage=\"%1\" -s use_gui=no -cfgparam=quickstart=CD32,0", IDI_APPICON },
	{ NULL }
};
static struct  contextcommand cc_disk[] = {
	{ L"A500", L"-0 \"%1\" -s use_gui=no -cfgparam=quickstart=A500,0", IDI_DISKIMAGE },
	{ L"A1200", L"-0 \"%1\" -s use_gui=no -cfgparam=quickstart=A1200,0", IDI_DISKIMAGE },
	{ NULL }
};
struct assext exts[] = {
//	{ L".cue", L"-cdimage=\"%1\" -s use_gui=no", L"WinUAE CD image", IDI_DISKIMAGE, cc_cd },
//	{ L".iso", L"-cdimage=\"%1\" -s use_gui=no", L"WinUAE CD image", IDI_DISKIMAGE, cc_cd },
//	{ L".ccd", L"-cdimage=\"%1\" -s use_gui=no", L"WinUAE CD image", IDI_DISKIMAGE, cc_cd },
	{ L".uae", L"-f \"%1\"", L"WinUAE configuration file", IDI_CONFIGFILE, NULL },
	{ L".adf", L"-0 \"%1\" -s use_gui=no", L"WinUAE floppy disk image", IDI_DISKIMAGE, cc_disk },
	{ L".adz", L"-0 \"%1\" -s use_gui=no", L"WinUAE floppy disk image", IDI_DISKIMAGE, cc_disk },
	{ L".dms", L"-0 \"%1\" -s use_gui=no", L"WinUAE floppy disk image", IDI_DISKIMAGE, cc_disk },
	{ L".fdi", L"-0 \"%1\" -s use_gui=no", L"WinUAE floppy disk image", IDI_DISKIMAGE, cc_disk },
	{ L".ipf", L"-0 \"%1\" -s use_gui=no", L"WinUAE floppy disk image", IDI_DISKIMAGE, cc_disk },
	{ L".uss", L"-s statefile=\"%1\" -s use_gui=no", L"WinUAE statefile", IDI_APPICON, NULL },
	{ NULL }
};

static void associate_init_extensions (void)
{
	int i;

	for (i = 0; exts[i].ext; i++) {
		exts[i].enabled = 0;
		if (shell_associate_is (exts[i].ext))
			exts[i].enabled = 1;
	}
	if (rp_param || inipath)
		return;
	// associate .uae by default when running for the first time
	if (!regexiststree (NULL, L"FileAssociations")) {
		UAEREG *fkey;
		if (exts[0].enabled == 0) {
			shell_associate (exts[0].ext, exts[0].cmd, exts[0].cc, NULL, exts[0].desc, NULL, exts[0].icon);
			exts[0].enabled = shell_associate_is (exts[0].ext);
		}
		fkey = regcreatetree (NULL, L"FileAssociations");
		regsetstr (fkey, exts[0].ext, L"");
		regclosetree (fkey);
	}
	if (os_winnt_admin > 1) {
		DWORD disposition;
		TCHAR rpath[MAX_DPATH];
		HKEY rkey = HKEY_LOCAL_MACHINE;
		HKEY key1;
		int setit = 1;

		_tcscpy (rpath, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\winuae.exe");
		if (RegOpenKeyEx (rkey, rpath, 0, KEY_READ, &key1) == ERROR_SUCCESS) {
			TCHAR tmp[MAX_DPATH];
			DWORD size = sizeof tmp / sizeof (TCHAR);
			if (RegQueryValueEx (key1, NULL, NULL, NULL, (LPBYTE)tmp, &size) == ERROR_SUCCESS) {
				if (!_tcscmp (tmp, _wpgmptr))
					setit = 0;
			}
			RegCloseKey (key1);
		}
		if (setit) {
			if (RegCreateKeyEx (rkey, rpath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &key1, &disposition) == ERROR_SUCCESS) {
				RegSetValueEx (key1, L"", 0, REG_SZ, (CONST BYTE *)_wpgmptr, (_tcslen (_wpgmptr) + 1) * sizeof (TCHAR));
				RegCloseKey (key1);
				SHChangeNotify (SHCNE_ASSOCCHANGED, 0, 0, 0); 
			}
		}
	}

#if 0
	UAEREG *fkey;
	fkey = regcreatetree (NULL, L"FileAssociations");
	if (fkey) {
		int ok = 1;
		TCHAR tmp[MAX_DPATH];
		_tcscpy (tmp, L"Following file associations:\n");
		for (i = 0; exts[i].ext; i++) {
			TCHAR tmp2[10];
			int size = sizeof tmp;
			int is1 = exts[i].enabled;
			int is2 = regquerystr (fkey, exts[i].ext, tmp2, &size);
			if (is1 == 0 && is2 != 0) {
				_tcscat (tmp, exts[i].ext);
				_tcscat (tmp, L"\n");
				ok = 0;
			}
		}
		if (!ok) {
			TCHAR szTitle[MAX_DPATH];
			WIN32GUI_LoadUIString (IDS_ERRORTITLE, szTitle, MAX_DPATH);
			_tcscat (szTitle, BetaStr);
			if (MessageBox (NULL, tmp, szTitle, MB_YESNO | MB_TASKMODAL) == IDOK) {
				for (i = 0; exts[i].ext; i++) {
					TCHAR tmp2[10];
					int size = sizeof tmp;
					int is1 = exts[i].enabled;
					int is2 = regquerystr (fkey, exts[i].ext, tmp2, &size);
					if (is1 == 0 && is2 != 0) {
						regdelete (fkey, exts[i].ext);
						shell_associate (exts[i].ext, exts[i].cmd, NULL, exts[i].desc, NULL);
						exts[i].enabled = shell_associate_is (exts[i].ext);
					}
				}
			} else {
				for (i = 0; exts[i].ext; i++) {
					if (!exts[i].enabled)
						regdelete (fkey, exts[i].ext);
				}
			}
		}
	}
#endif
}

void associate_file_extensions (void)
{
	int i;
	int modified = 0;

	if (rp_param)
		return;
	for (i = 0; exts[i].ext; i++) {
		int already = shell_associate_is (exts[i].ext);
		if (exts[i].enabled == 0 && already) {
			shell_deassociate (exts[i].ext);
			exts[i].enabled = shell_associate_is (exts[i].ext);
			if (exts[i].enabled) {
				modified = 1;
				shell_associate (exts[i].ext, exts[i].cmd, exts[i].cc, NULL, exts[i].desc, NULL, exts[i].icon);
			}
		} else if (exts[i].enabled) {
			shell_associate (exts[i].ext, exts[i].cmd, exts[i].cc, NULL, exts[i].desc, NULL, exts[i].icon);
			exts[i].enabled = shell_associate_is (exts[i].ext);
			if (exts[i].enabled != already)
				modified = 1;
		}
	}
	if (modified)
		SHChangeNotify (SHCNE_ASSOCCHANGED, 0, 0, 0); 
}

static void WIN32_HandleRegistryStuff (void)
{
	RGBFTYPE colortype = RGBFB_NONE;
	DWORD dwType = REG_DWORD;
	DWORD dwDisplayInfoSize = sizeof (colortype);
	int size;
	TCHAR path[MAX_DPATH] = L"";
	TCHAR version[100];

	initpath (L"FloppyPath", start_path_data);
	initpath (L"KickstartPath", start_path_data);
	initpath (L"hdfPath", start_path_data);
	initpath (L"ConfigurationPath", start_path_data);
	initpath (L"ScreenshotPath", start_path_data);
	initpath (L"StatefilePath", start_path_data);
	initpath (L"SaveimagePath", start_path_data);
	initpath (L"VideoPath", start_path_data);
	initpath (L"InputPath", start_path_data);
	if (!regexists (NULL, L"MainPosX") || !regexists (NULL, L"GUIPosX")) {
		int x = GetSystemMetrics (SM_CXSCREEN);
		int y = GetSystemMetrics (SM_CYSCREEN);
		x = (x - 800) / 2;
		y = (y - 600) / 2;
		if (x < 10)
			x = 10;
		if (y < 10)
			y = 10;
		/* Create and initialize all our sub-keys to the default values */
		regsetint (NULL, L"MainPosX", x);
		regsetint (NULL, L"MainPosY", y);
		regsetint (NULL, L"GUIPosX", x);
		regsetint (NULL, L"GUIPosY", y);
	}
	size = sizeof (version) / sizeof (TCHAR);
	if (regquerystr (NULL, L"Version", version, &size)) {
		if (checkversion (version))
			regsetstr (NULL, L"Version", VersionStr);
	} else {
		regsetstr (NULL, L"Version", VersionStr);
	}
	size = sizeof (version) / sizeof (TCHAR);
	if (regquerystr (NULL, L"ROMCheckVersion", version, &size)) {
		if (checkversion (version)) {
			if (regsetstr (NULL, L"ROMCheckVersion", VersionStr))
				forceroms = 1;
		}
	} else {
		if (regsetstr (NULL, L"ROMCheckVersion", VersionStr))
			forceroms = 1;
	}

	regqueryint (NULL, L"DirectDraw_Secondary", &ddforceram);
	if (regexists (NULL, L"SoundDriverMask")) {
		regqueryint (NULL, L"SoundDriverMask", &sounddrivermask);
	} else {
		sounddrivermask = 3;
		regsetint (NULL, L"SoundDriverMask", sounddrivermask);
	}
	if (regexists (NULL, L"ConfigurationCache"))
		regqueryint (NULL, L"ConfigurationCache", &configurationcache);
	else
		regsetint (NULL, L"ConfigurationCache", configurationcache);
	if (regexists (NULL, L"RelativePaths"))
		regqueryint (NULL, L"RelativePaths", &relativepaths);
	else
		regsetint (NULL, L"RelativePaths", relativepaths);
	regqueryint (NULL, L"QuickStartMode", &quickstart);
	reopen_console ();
	fetch_path (L"ConfigurationPath", path, sizeof (path) / sizeof (TCHAR));
	path[_tcslen (path) - 1] = 0;
	if (GetFileAttributes (path) == 0xffffffff) {
		createdir (path);
		_tcscat (path, L"\\Host");
		createdir (path);
		fetch_path (L"ConfigurationPath", path, sizeof (path) / sizeof (TCHAR));
		_tcscat (path, L"Hardware");
		createdir (path);
	}
	fetch_path (L"StatefilePath", path, sizeof (path) / sizeof (TCHAR));
	createdir (path);
	_tcscat (path, L"default.uss");
	_tcscpy (savestate_fname, path);
	fetch_path (L"InputPath", path, sizeof (path) / sizeof (TCHAR));
	createdir (path);
	regclosetree (read_disk_history (HISTORY_FLOPPY));
	regclosetree (read_disk_history (HISTORY_CD));
	associate_init_extensions ();
	read_rom_list ();
	load_keyring (NULL, NULL);
}

#if WINUAEPUBLICBETA > 0
static TCHAR *BETAMESSAGE = {
	L"This is unstable beta software. Click cancel if you are not comfortable using software that is incomplete and can have serious programming errors."
};
#endif

static int betamessage (void)
{
#if WINUAEPUBLICBETA > 0
	int showmsg = TRUE;
	HANDLE h = INVALID_HANDLE_VALUE;
	ULONGLONG regft64;
	ULARGE_INTEGER ft64;
	ULARGE_INTEGER sft64;
	struct tm *t;
	__int64 ltime;
	DWORD dwType, size, data;

	ft64.QuadPart = 0;
	for (;;) {
		FILETIME ft, sft;
		SYSTEMTIME st;
		TCHAR tmp1[MAX_DPATH];

		if (!hWinUAEKey)
			break;
		if (GetModuleFileName (NULL, tmp1, sizeof tmp1 / sizeof (TCHAR)) == 0)
			break;
		h = CreateFile (tmp1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (h == INVALID_HANDLE_VALUE)
			break;
		if (GetFileTime (h, &ft, NULL, NULL) == 0)
			break;
		ft64.LowPart = ft.dwLowDateTime;
		ft64.HighPart = ft.dwHighDateTime;
		dwType = REG_QWORD;
		size = sizeof regft64;
		if (RegQueryValueEx (hWinUAEKey, L"BetaToken", 0, &dwType, (LPBYTE)&regft64, &size) != ERROR_SUCCESS)
			break;
		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &sft);
		sft64.LowPart = sft.dwLowDateTime;
		sft64.HighPart = sft.dwHighDateTime;
		if (ft64.QuadPart == regft64)
			showmsg = FALSE;
		/* complain again in 7 days */
		if (sft64.QuadPart > regft64 + (ULONGLONG)1000000000 * 60 * 60 * 24 * 7)
			showmsg = TRUE;
		break;
	}
	if (h != INVALID_HANDLE_VALUE)
		CloseHandle (h);
	if (showmsg) {
		int r;
		TCHAR title[MAX_DPATH];

		dwType = REG_DWORD;
		size = sizeof data;
		if (hWinUAEKey && RegQueryValueEx (hWinUAEKey, L"Beta_Just_Shut_Up", 0, &dwType, (LPBYTE)&data, &size) == ERROR_SUCCESS) {
			if (data == 68000 + 10) {
				write_log (L"I was told to shut up :(\n");
				return 1;
			}
		}

		_time64 (&ltime);
		t = _gmtime64 (&ltime);
		/* "expire" in 1 month */
		if (MAKEBD(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday) > WINUAEDATE + 100)
			pre_gui_message (L"This beta build of WinUAE is obsolete.\nPlease download newer version.");

		_tcscpy (title, L"WinUAE Public Beta Disclaimer");
		_tcscat (title, BetaStr);
		r = MessageBox (NULL, BETAMESSAGE, title, MB_OKCANCEL | MB_TASKMODAL | MB_SETFOREGROUND | MB_ICONWARNING | MB_DEFBUTTON2);
		if (r == IDABORT || r == IDCANCEL)
			return 0;
		if (ft64.QuadPart > 0) {
			regft64 = ft64.QuadPart;
			RegSetValueEx (hWinUAEKey, L"BetaToken", 0, REG_QWORD, (LPBYTE)&regft64, sizeof regft64);
		}
	}
#endif
	return 1;
}

static int dxdetect (void)
{
#if !defined(WIN64)
	/* believe or not but this is MS supported way of detecting DX8+ */
	HMODULE h = LoadLibrary (L"D3D8.DLL");
	TCHAR szWrongDXVersion[MAX_DPATH];
	if (h) {
		FreeLibrary (h);
		return 1;
	}
	WIN32GUI_LoadUIString (IDS_WRONGDXVERSION, szWrongDXVersion, MAX_DPATH);
	pre_gui_message (szWrongDXVersion);
	return 0;
#else
	return 1;
#endif
}

int os_winnt, os_winnt_admin, os_64bit, os_win7, os_vista, os_winxp;

static int isadminpriv (void)
{
	DWORD i, dwSize = 0, dwResult = 0;
	HANDLE hToken;
	PTOKEN_GROUPS pGroupInfo;
	BYTE sidBuffer[100];
	PSID pSID = (PSID)&sidBuffer;
	SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
	int isadmin = 0;

	// Open a handle to the access token for the calling process.
	if (!OpenProcessToken (GetCurrentProcess (), TOKEN_QUERY, &hToken)) {
		write_log (L"OpenProcessToken Error %u\n", GetLastError ());
		return FALSE;
	}

	// Call GetTokenInformation to get the buffer size.
	if(!GetTokenInformation (hToken, TokenGroups, NULL, dwSize, &dwSize)) {
		dwResult = GetLastError ();
		if(dwResult != ERROR_INSUFFICIENT_BUFFER) {
			write_log (L"GetTokenInformation Error %u\n", dwResult);
			return FALSE;
		}
	}

	// Allocate the buffer.
	pGroupInfo = (PTOKEN_GROUPS)GlobalAlloc (GPTR, dwSize);

	// Call GetTokenInformation again to get the group information.
	if (!GetTokenInformation (hToken, TokenGroups, pGroupInfo, dwSize, &dwSize)) {
		write_log (L"GetTokenInformation Error %u\n", GetLastError ());
		return FALSE;
	}

	// Create a SID for the BUILTIN\Administrators group.
	if (!AllocateAndInitializeSid (&SIDAuth, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pSID)) {
			write_log (L"AllocateAndInitializeSid Error %u\n", GetLastError ());
			return FALSE;
	}

	// Loop through the group SIDs looking for the administrator SID.
	for(i = 0; i < pGroupInfo->GroupCount; i++) {
		if (EqualSid (pSID, pGroupInfo->Groups[i].Sid))
			isadmin = 1;
	}

	if (pSID)
		FreeSid (pSID);
	if (pGroupInfo)
		GlobalFree (pGroupInfo);
	return isadmin;
}

typedef void (CALLBACK* PGETNATIVESYSTEMINFO)(LPSYSTEM_INFO);
typedef BOOL (CALLBACK* PISUSERANADMIN)(VOID);

static int osdetect (void)
{
	PGETNATIVESYSTEMINFO pGetNativeSystemInfo;
	PISUSERANADMIN pIsUserAnAdmin;

	pGetNativeSystemInfo = (PGETNATIVESYSTEMINFO)GetProcAddress (
		GetModuleHandle (L"kernel32.dll"), "GetNativeSystemInfo");
	pIsUserAnAdmin = (PISUSERANADMIN)GetProcAddress (
		GetModuleHandle (L"shell32.dll"), "IsUserAnAdmin");

	GetSystemInfo (&SystemInfo);
	if (pGetNativeSystemInfo)
		pGetNativeSystemInfo (&SystemInfo);
	osVersion.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	if (GetVersionEx (&osVersion)) {
		if ((osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
			(osVersion.dwMajorVersion <= 4))
		{
			/* WinUAE not supported on this version of Windows... */
			TCHAR szWrongOSVersion[MAX_DPATH];
			WIN32GUI_LoadUIString (IDS_WRONGOSVERSION, szWrongOSVersion, MAX_DPATH);
			pre_gui_message (szWrongOSVersion);
			return FALSE;
		}
		if (osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT)
			os_winnt = 1;
		if (osVersion.dwMajorVersion > 5 || (osVersion.dwMajorVersion == 5 && osVersion.dwMinorVersion >= 1))
			os_winxp = 1;
		if (osVersion.dwMajorVersion >= 6)
			os_vista = 1;
		if (osVersion.dwMajorVersion >= 7 || (osVersion.dwMajorVersion == 6 && osVersion.dwMinorVersion >= 1))
			os_win7 = 1;
		if (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
			os_64bit = 1;
	}
	if (!os_winnt)
		return 0;
	os_winnt_admin = isadminpriv ();
	if (os_winnt_admin) {
		if (pIsUserAnAdmin) {
			if (pIsUserAnAdmin ())
				os_winnt_admin++;
		} else {
			os_winnt_admin++;
		}
	}

	return 1;
}

void create_afnewdir (int remove)
{
	TCHAR tmp[MAX_DPATH], tmp2[MAX_DPATH];

	if (SUCCEEDED (SHGetFolderPath (NULL, CSIDL_COMMON_DOCUMENTS, NULL, 0, tmp))) {
		fixtrailing (tmp);
		_tcscpy (tmp2, tmp);
		_tcscat (tmp2, L"Amiga Files");
		_tcscpy (tmp, tmp2);
		_tcscat (tmp, L"\\WinUAE");
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
}

static bool isdir (const TCHAR *path)
{
	DWORD a = GetFileAttributes (path);
	if (a == INVALID_FILE_ATTRIBUTES)
		return false;
	if (a & FILE_ATTRIBUTE_DIRECTORY)
		return true;
	return false;
}

bool get_plugin_path (TCHAR *out, int len, const TCHAR *path)
{
	TCHAR tmp[MAX_DPATH];
	_tcscpy (tmp, start_path_plugins);
	if (path != NULL)
		_tcscat (tmp, path);
	if (isdir (tmp)) {
		_tcscpy (out, tmp);
		fixtrailing (out);
		return true;
	}
	if (!_tcsicmp (path, L"floppysounds")) {
		_tcscpy (tmp, start_path_data);
		_tcscpy (tmp, L"uae_data");
		if (isdir (tmp)) {
			_tcscpy (out, tmp);
			fixtrailing (out);
			return true;
		}
		_tcscpy (tmp, start_path_exe);
		_tcscpy (tmp, L"uae_data");
		if (isdir (tmp)) {
			_tcscpy (out, tmp);
			fixtrailing (out);
			return true;
		}
	}
	_tcscpy (tmp, start_path_data);
	_tcscpy (tmp, WIN32_PLUGINDIR);
	if (path != NULL)
		_tcscat (tmp, path);
	if (isdir (tmp)) {
		_tcscpy (out, tmp);
		fixtrailing (out);
		return true;
	}
	_tcscpy (tmp, start_path_exe);
	_tcscpy (tmp, WIN32_PLUGINDIR);
	if (path != NULL)
		_tcscat (tmp, path);
	if (isdir (tmp)) {
		_tcscpy (out, tmp);
		fixtrailing (out);
		return true;
	}
	_tcscpy (out, start_path_plugins);
	if (path != NULL)
		_tcscat (tmp, path);
	fixtrailing (out);
	return false;
}

void setpathmode (pathtype pt)
{
	TCHAR pathmode[32] = { 0 };
	if (pt == PATH_TYPE_WINUAE)
		_tcscpy (pathmode, L"WinUAE");
	if (pt == PATH_TYPE_NEWWINUAE)
		_tcscpy (pathmode, L"WinUAE_2");
	if (pt == PATH_TYPE_NEWAF)
		_tcscpy (pathmode, L"AmigaForever");
	if (pt == PATH_TYPE_AMIGAFOREVERDATA)
		_tcscpy (pathmode, L"AMIGAFOREVERDATA");
	regsetstr (NULL, L"PathMode", pathmode);
}

static void getstartpaths (void)
{
	TCHAR *posn, *p;
	TCHAR tmp[MAX_DPATH], tmp2[MAX_DPATH], prevpath[MAX_DPATH];
	DWORD v;
	UAEREG *key;
	TCHAR xstart_path_uae[MAX_DPATH], xstart_path_old[MAX_DPATH];
	TCHAR xstart_path_new1[MAX_DPATH], xstart_path_new2[MAX_DPATH];

	path_type = PATH_TYPE_DEFAULT;
	prevpath[0] = 0;
	xstart_path_uae[0] = xstart_path_old[0] = xstart_path_new1[0] = xstart_path_new2[0] = 0;
	key = regcreatetree (NULL, NULL);
	if (key)  {
		int size = sizeof (prevpath) / sizeof (TCHAR);
		if (!regquerystr (key, L"PathMode", prevpath, &size))
			prevpath[0] = 0;
		regclosetree (key);
	}
	if (!_tcscmp (prevpath, L"WinUAE"))
		path_type = PATH_TYPE_WINUAE;
	if (!_tcscmp (prevpath, L"WinUAE_2"))
		path_type = PATH_TYPE_NEWWINUAE;
	if (!_tcscmp (prevpath, L"AF2005") || !_tcscmp (prevpath, L"AmigaForever"))
		path_type = PATH_TYPE_NEWAF;
	if (!_tcscmp (prevpath, L"AMIGAFOREVERDATA"))
		path_type = PATH_TYPE_AMIGAFOREVERDATA;

	GetFullPathName (_wpgmptr, sizeof start_path_exe / sizeof (TCHAR), start_path_exe, NULL);
	if((posn = _tcsrchr (start_path_exe, '\\')))
		posn[1] = 0;

	if (path_type == PATH_TYPE_DEFAULT && inipath) {
		path_type = PATH_TYPE_WINUAE;
		_tcscpy (xstart_path_uae, start_path_exe);
		relativepaths = 1;
	} else if (path_type == PATH_TYPE_DEFAULT && start_data == 0 && key) {
		bool ispath = false;
		_tcscpy (tmp2, start_path_exe);
		_tcscat (tmp2, L"configurations\\configuration.cache");
		v = GetFileAttributes (tmp2);
		if (v != INVALID_FILE_ATTRIBUTES && !(v & FILE_ATTRIBUTE_DIRECTORY))
			ispath = true;
		_tcscpy (tmp2, start_path_exe);
		_tcscat (tmp2, L"roms");
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
						_tcscat (tmp2, L"Amiga Files");
						CreateDirectory (tmp2, NULL);
						_tcscat (tmp2, L"\\WinUAE");
						CreateDirectory (tmp2, NULL);
						v = GetFileAttributes (tmp2);
						if (v != INVALID_FILE_ATTRIBUTES && (v & FILE_ATTRIBUTE_DIRECTORY)) {
							_tcscat (tmp2, L"\\");
							path_type = PATH_TYPE_NEWWINUAE;
							_tcscpy (tmp, tmp2);
							_tcscat (tmp, L"Configurations");
							CreateDirectory (tmp, NULL);
							_tcscpy (tmp, tmp2);
							_tcscat (tmp, L"Screenshots");
							CreateDirectory (tmp, NULL);
							_tcscpy (tmp, tmp2);
							_tcscat (tmp, L"Savestates");
							CreateDirectory (tmp, NULL);
							_tcscpy (tmp, tmp2);
							_tcscat (tmp, L"Screenshots");
							CreateDirectory (tmp, NULL);
						}
					}
				}
			}
		}
	}

	_tcscpy (tmp, start_path_exe);
	_tcscat (tmp, L"roms");
	if (isfilesindir (tmp)) {
		_tcscpy (xstart_path_uae, start_path_exe);
	}
	_tcscpy (tmp, start_path_exe);
	_tcscat (tmp, L"configurations");
	if (isfilesindir (tmp)) {
		_tcscpy (xstart_path_uae, start_path_exe);
	}

	p = _wgetenv (L"AMIGAFOREVERDATA");
	if (p) {
		_tcscpy (tmp, p);
		fixtrailing (tmp);
		_tcscpy (start_path_new2, p);
		v = GetFileAttributes (tmp);
		if (v != INVALID_FILE_ATTRIBUTES && (v & FILE_ATTRIBUTE_DIRECTORY)) {
			_tcscpy (xstart_path_new2, start_path_new2);
			_tcscpy (xstart_path_new2, L"WinUAE\\");
			af_path_2005 |= 2;
		}
	}

	if (SUCCEEDED (SHGetFolderPath (NULL, CSIDL_COMMON_DOCUMENTS, NULL, SHGFP_TYPE_CURRENT, tmp))) {
		fixtrailing (tmp);
		_tcscpy (tmp2, tmp);
		_tcscat (tmp2, L"Amiga Files\\");
		_tcscpy (tmp, tmp2);
		_tcscat (tmp, L"WinUAE");
		v = GetFileAttributes (tmp);
		if (v != INVALID_FILE_ATTRIBUTES && (v & FILE_ATTRIBUTE_DIRECTORY)) {
			TCHAR *p;
			_tcscpy (xstart_path_new1, tmp2);
			_tcscat (xstart_path_new1, L"WinUAE\\");
			_tcscpy (xstart_path_uae, start_path_exe);
			_tcscpy (start_path_new1, xstart_path_new1);
			p = tmp2 + _tcslen (tmp2);
			_tcscpy (p, L"System");
			if (isfilesindir (tmp2)) {
				af_path_2005 |= 1;
			} else {
				_tcscpy (p, L"Shared");
				if (isfilesindir (tmp2)) {
					af_path_2005 |= 1;
				}
			}
		}
	}

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
				_tcscat (tmp, L"system\\rom");
				if (isfilesindir (tmp)) {
					path_type = PATH_TYPE_AMIGAFOREVERDATA;
				} else {
					_tcscpy (tmp, xstart_path_new2);
					_tcscat (tmp, L"shared\\rom");
					if (isfilesindir (tmp)) {
						path_type = PATH_TYPE_AMIGAFOREVERDATA;
					} else {
						path_type = PATH_TYPE_NEWWINUAE;
					}
				}
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
}

extern void test (void);
extern int screenshotmode, postscript_print_debugging, sound_debug, log_uaeserial, clipboard_debug;
extern int force_direct_catweasel, sound_mode_skip, maxmem;
extern int pngprint, log_sercon;

extern DWORD_PTR cpu_affinity, cpu_paffinity;
static DWORD_PTR original_affinity = -1;

static int getval (const TCHAR *s)
{
	int base = 10;
	int v;
	TCHAR *endptr;

	if (s[0] == '0' && _totupper(s[1]) == 'X')
		s += 2, base = 16;
	v = _tcstol (s, &endptr, base);
	if (*endptr != '\0' || *s == '\0')
		return 0;
	return v;
}

static void makeverstr (TCHAR *s)
{
	if (_tcslen (WINUAEBETA) > 0) {
		_stprintf (BetaStr, L" (%sBeta %s, %d.%02d.%02d)", WINUAEPUBLICBETA > 0 ? L"Public " : L"", WINUAEBETA,
			GETBDY(WINUAEDATE), GETBDM(WINUAEDATE), GETBDD(WINUAEDATE));
#ifdef _WIN64
		_tcscat (BetaStr, L" 64-bit");
#endif
		_stprintf (s, L"WinUAE %d.%d.%d%s%s",
			UAEMAJOR, UAEMINOR, UAESUBREV, WINUAEREV, BetaStr);
	} else {
		_stprintf (s, L"WinUAE %d.%d.%d%s (%d.%02d.%02d)",
			UAEMAJOR, UAEMINOR, UAESUBREV, WINUAEREV, GETBDY(WINUAEDATE), GETBDM(WINUAEDATE), GETBDD(WINUAEDATE));
#ifdef _WIN64
		_tcscat (s, L" 64-bit");
#endif
	}
	if (_tcslen (WINUAEEXTRA) > 0) {
		_tcscat (s, L" ");
		_tcscat (s, WINUAEEXTRA);
	}
}

static TCHAR *getdefaultini (void)
{
	FILE *f;
	TCHAR path[MAX_DPATH], orgpath[MAX_DPATH];
	path[0] = 0;
	if (!GetFullPathName (_wpgmptr, sizeof path / sizeof (TCHAR), path, NULL))
		_tcscpy (path, _wpgmptr);
	TCHAR *posn;
	if((posn = _tcsrchr (path, '\\')))
		posn[1] = 0;
	_tcscat (path, L"winuae.ini");
	_tcscpy (orgpath, path);
#if 1
	f = _tfopen (path, L"r+");
	if (f) {
		fclose (f);
		return my_strdup (path);
	}
	f = _tfopen (path, L"w");
	if (f) {
		fclose (f);
		return my_strdup (path);
	}
#endif
	int v = GetTempPath (sizeof path / sizeof (TCHAR), path);
	if (v == 0 || v > sizeof path / sizeof (TCHAR))
		return my_strdup (orgpath);
	_tcsncat (path, L"winuae.ini", sizeof path / sizeof (TCHAR));
	f = _tfopen (path, L"w");
	if (f) {
		fclose (f);
		return my_strdup (path);
	}
	return my_strdup (orgpath);
}

static int parseargs (const TCHAR *argx, const TCHAR *np, const TCHAR *np2)
{
	const TCHAR *arg = argx + 1;

	if (argx[0] != '-' && argx[0] != '/')
		return 0;

	if (!_tcscmp (arg, L"convert") && np && np2) {
		zfile_convertimage (np, np2);
		return -1;
	}
	if (!_tcscmp (arg, L"console")) {
		console_started = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"cli")) {
		console_emulation = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"log")) {
		console_logging = 1;
		return 1;
	}
#ifdef FILESYS
	if (!_tcscmp (arg, L"rdbdump")) {
		do_rdbdump = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"hddump")) {
		do_rdbdump = 2;
		return 1;
	}
	if (!_tcscmp (arg, L"disableharddrivesafetycheck")) {
		//harddrive_dangerous = 0x1234dead;
		return 1;
	}
	if (!_tcscmp (arg, L"noaspifiltering")) {
		aspi_allow_all = 1;
		return 1;
	}
#endif
	if (!_tcscmp (arg, L"pngprint")) {
		pngprint = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"norawinput")) {
		no_rawinput = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"rawkeyboard")) {
		// obsolete
		return 1;
	}
	if (!_tcscmp (arg, L"directsound")) {
		force_directsound = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"scsilog")) {
		log_scsi = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"scsiemulog")) {
		extern int log_scsiemu;
		log_scsiemu = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"netlog")) {
		log_net = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"serlog")) {
		log_sercon = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"a2065log")) {
		log_a2065 = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"a2065log2")) {
		log_a2065 = 2;
		return 1;
	}
	if (!_tcscmp (arg, L"a2065_promiscuous")) {
		a2065_promiscuous = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"seriallog")) {
		log_uaeserial = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"clipboarddebug")) {
		clipboard_debug = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"rplog")) {
		log_rp = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"nomultidisplay")) {
		multi_display = 0;
		return 1;
	}
	if (!_tcscmp (arg, L"legacypaths")) {
		start_data = -2;
		return 1;
	}
	if (!_tcscmp (arg, L"screenshotbmp")) {
		screenshotmode = 0;
		return 1;
	}
	if (!_tcscmp (arg, L"psprintdebug")) {
		postscript_print_debugging = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"sounddebug")) {
		sound_debug = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"directcatweasel")) {
		force_direct_catweasel = 1;
		if (np) {
			force_direct_catweasel = getval (np);
			return 2;
		}
		return 1;
	}
	if (!_tcscmp (arg, L"forcerdtsc")) {
		userdtsc = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"ddsoftwarecolorkey")) {
		// obsolete
		return 1;
	}
	if (!_tcscmp (arg, L"nod3d9ex")) {
		D3DEX = 0;
		return 1;
	}
	if (!_tcscmp (arg, L"d3ddebug")) {
		d3ddebug = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"logflush")) {
		extern int always_flush_log;
		always_flush_log = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"ahidebug")) {
		extern int ahi_debug;
		ahi_debug = 2;
		return 1;
	}
	if (!_tcscmp (arg, L"ahidebug2")) {
		extern int ahi_debug;
		ahi_debug = 3;
		return 1;
	}
	if (!_tcscmp (arg, L"quittogui")) {
		quit_to_gui = 1;
		return 1;
	}
	if (!_tcscmp (arg, L"ini") && np) {
		inipath = my_strdup (np);
		return 2;
	}
	if (!_tcscmp (arg, L"portable")) {
		inipath = getdefaultini ();
		return 2;
	}

	if (!np)
		return 0;

	if (!_tcscmp (arg, L"ddforcemode")) {
		extern int ddforceram;
		ddforceram = getval (np);
		if (ddforceram < 0 || ddforceram > 3)
			ddforceram = 0;
		return 2;
	}
	if (!_tcscmp (arg, L"affinity")) {
		cpu_affinity = getval (np);
		if (cpu_affinity == 0)
			cpu_affinity = original_affinity;
		SetThreadAffinityMask (GetCurrentThread (), cpu_affinity);
		return 2;
	}
	if (!_tcscmp (arg, L"paffinity")) {
		cpu_paffinity = getval (np);
		if (cpu_paffinity == 0)
			cpu_paffinity = original_affinity;
		SetProcessAffinityMask (GetCurrentProcess (), cpu_paffinity);
		return 2;
	}
	if (!_tcscmp (arg, L"datapath")) {
		ExpandEnvironmentStrings (np, start_path_data, sizeof start_path_data / sizeof (TCHAR));
		start_data = -1;
		return 2;
	}
	if (!_tcscmp (arg, L"pluginpath")) {
		ExpandEnvironmentStrings (np, start_path_plugins, sizeof start_path_plugins / sizeof (TCHAR));
		return 2;
	}
	if (!_tcscmp (arg, L"maxmem")) {
		maxmem = getval (np);
		return 2;
	}
	if (!_tcscmp (arg, L"soundmodeskip")) {
		sound_mode_skip = getval (np);
		return 2;
	}
	if (!_tcscmp (arg, L"p96skipmode")) {
		extern int p96skipmode;
		p96skipmode = getval (np);
		return 2;
	}
	if (!_tcscmp (arg, L"minidumpmode")) {
		minidumpmode = (MINIDUMP_TYPE)getval (np);
		return 2;
	}
	if (!_tcscmp (arg, L"jitevent")) {
		pissoff_value = getval (np);
		return 2;
	}
	if (!_tcscmp (arg, L"inputrecorddebug")) {
		inputrecord_debug = getval (np);
		return 2;
	}
#ifdef RETROPLATFORM
	if (!_tcscmp (arg, L"rphost")) {
		rp_param = my_strdup (np);
		return 2;
	}
	if (!_tcscmp (arg, L"rpescapekey")) {
		rp_rpescapekey = getval (np);
		return 2;
	}
	if (!_tcscmp (arg, L"rpescapeholdtime")) {
		rp_rpescapeholdtime = getval (np);
		return 2;
	}
	if (!_tcscmp (arg, L"rpscreenmode")) {
		rp_screenmode = getval (np);
		return 2;
	}
	if (!_tcscmp (arg, L"rpinputmode")) {
		rp_inputmode = getval (np);
		return 2;
	}
#endif
	return 0;
}

static TCHAR **parseargstrings (TCHAR *s, TCHAR **xargv)
{
	int cnt, i, xargc;
	TCHAR **args;

	args = parseargstring (s);
	for (cnt = 0; args[cnt]; cnt++);
	for (xargc = 0; xargv[xargc]; xargc++);
	for (i = 0; i < cnt; i++) {
		TCHAR *arg = args[i];
		TCHAR *next = i + 1 < cnt ? args[i + 1] : NULL;
		TCHAR *next2 = i + 2 < cnt ? args[i + 2] : NULL;
		int v = parseargs (arg, next, next2);
		if (!v) {
			xargv[xargc++] = my_strdup (arg);
		} else if (v == 2) {
			i++;
		} else if (v < 0) {
			doquit = 1;
			return NULL;
		}
	}
	return args;
}


static int process_arg (TCHAR *cmdline, TCHAR **xargv, TCHAR ***xargv3)
{
	int i, xargc;
	TCHAR **argv;
	TCHAR tmp[MAX_DPATH];
	int fd, ok, added;

	*xargv3 = NULL;
	argv = parseargstring (cmdline);
	if (argv == NULL)
		return 0;
	added = 0;
	xargc = 0;
	xargv[xargc++] = my_strdup (_wpgmptr);
	fd = 0;
	for (i = 0; argv[i]; i++) {
		TCHAR *f = argv[i];
		ok = 0;
		if (f[0] != '-' && f[0] != '/') {
			int type = -1;
			struct zfile *z = zfile_fopen (f, L"rb", ZFD_NORMAL);
			if (z) {
				type = zfile_gettype (z);
				zfile_fclose (z);
			}
			tmp[0] = 0;
			switch (type)
			{
			case ZFILE_CONFIGURATION:
				_stprintf (tmp, L"-config=%s", f);
				break;
			case ZFILE_STATEFILE:
				_stprintf (tmp, L"-statefile=%s", f);
				break;
			case ZFILE_CDIMAGE:
				_stprintf (tmp, L"-cdimage=%s", f);
				break;
			case ZFILE_DISKIMAGE:
				if (fd < 4)
					_stprintf (tmp, L"-cfgparam=floppy%d=%s", fd++, f);
				break;
			}
			if (tmp[0]) {
				xfree (argv[i]);
				argv[i] = my_strdup (tmp);
				ok = 1;
				added = 1;
			}
		}
		if (!ok)
			break;
	}
	if (added) {
		for (i = 0; argv[i]; i++);
		argv[i++] = my_strdup (L"-s");
		argv[i++] = my_strdup (L"use_gui=no");
		argv[i] = NULL;
	}
	for (i = 0; argv[i]; i++) {
		TCHAR *arg = argv[i];
		TCHAR *next = argv[i + 1];
		TCHAR *next2 = next != NULL ? argv[i + 2] : NULL;
		int v = parseargs (arg, next, next2);
		if (!v) {
			xargv[xargc++] = my_strdup (arg);
		} else if (v == 2) {
			i++;
		} else if (v < 0) {
			doquit = 1;
			return 0;
		}
	}
#if 0
	argv = 0;
	argv[0] = 0;
#endif
	*xargv3 = argv;
	return xargc;
}

static TCHAR **WIN32_InitRegistry (TCHAR **argv)
{
	DWORD disposition;
	TCHAR tmp[MAX_DPATH];
	int size = sizeof tmp / sizeof (TCHAR);

	reginitializeinit (inipath);
	hWinUAEKey = NULL;
	if (getregmode () == 0 || WINUAEPUBLICBETA > 0) {
		/* Create/Open the hWinUAEKey which points our config-info */
		RegCreateKeyEx (HKEY_CURRENT_USER, L"Software\\Arabuusimiehet\\WinUAE", 0, L"", REG_OPTION_NON_VOLATILE,
			KEY_WRITE | KEY_READ, NULL, &hWinUAEKey, &disposition);
		if (hWinUAEKey == NULL) {
			FILE *f;
			TCHAR *path;

			path = getdefaultini ();
			f = _tfopen (path, L"r");
			if (!f)
				f = _tfopen (path, L"w");
			if (f) {
				fclose (f);
				reginitializeinit (path);
			}
			xfree (path);
		}
	}
	if (regquerystr (NULL, L"Commandline", tmp, &size))
		return parseargstrings (tmp, argv);
	return NULL;
}


static int PASCAL WinMain2 (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	HANDLE hMutex;
	TCHAR **argv = NULL, **argv2 = NULL, **argv3;
	int argc, i;

	if (!osdetect ())
		return 0;
	if (!dxdetect ())
		return 0;

	hInst = hInstance;
	hMutex = CreateMutex (NULL, FALSE, L"WinUAE Instantiated"); // To tell the installer we're running


	argv = xcalloc (TCHAR*, MAX_ARGUMENTS);
	argv3 = NULL;
	argc = process_arg (lpCmdLine, argv, &argv3);
	if (doquit)
		return 0;

	argv2 = WIN32_InitRegistry (argv);
	getstartpaths ();
	makeverstr (VersionStr);

	logging_init ();
	if (_tcslen (lpCmdLine) > 0)
		write_log (L"'%s'\n", lpCmdLine);
	if (argv3 && argv3[0]) {
		write_log (L"params:\n");
		for (i = 0; argv3[i]; i++)
			write_log (L"%d: '%s'\n", i + 1, argv3[i]);
	}
	if (argv2) {
		write_log (L"extra params:\n");
		for (i = 0; argv2[i]; i++)
			write_log (L"%d: '%s'\n", i + 1, argv2[i]);
	}
	if (WIN32_RegisterClasses () && WIN32_InitLibraries ()) {
		DEVMODE devmode;
		DWORD i;

		WIN32_HandleRegistryStuff ();
		write_log (L"Enumerating display devices.. \n");
		enumeratedisplays (multi_display);
		write_log (L"Sorting devices and modes..\n");
		sortdisplays ();
		write_log (L"Display buffer mode = %d\n", ddforceram);
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
		memset (&devmode, 0, sizeof (devmode));
		devmode.dmSize = sizeof (DEVMODE);
		if (EnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &devmode)) {
			default_freq = devmode.dmDisplayFrequency;
			if (default_freq >= 70)
				default_freq = 70;
			else
				default_freq = 60;
		}
		WIN32_InitLang ();
		WIN32_InitHtmlHelp ();
		DirectDraw_Release ();
		unicode_init ();
		if (betamessage ()) {
			keyboard_settrans ();
#ifdef CATWEASEL
			catweasel_init ();
#endif
#ifdef PARALLEL_PORT
			paraport_mask = paraport_init ();
#endif
			globalipc = createIPC (L"WinUAE", 0);
			serialipc = createIPC (COMPIPENAME, 1);
			enumserialports ();
			enummidiports ();
			real_main (argc, argv);
		}
	}

	closeIPC (globalipc);
	closeIPC (serialipc);
	write_disk_history ();
	timeend ();
#ifdef AVIOUTPUT
	AVIOutput_Release ();
#endif
#ifdef AHI
	ahi_close_sound ();
#endif
#ifdef PARALLEL_PORT
	paraport_free ();
	closeprinter ();
#endif
	create_afnewdir (1);
	close_console ();
	_fcloseall ();
#ifdef RETROPLATFORM
	rp_free ();
#endif
	if (hWinUAEKey)
		RegCloseKey (hWinUAEKey);
	CloseHandle (hMutex);
	WIN32_CleanupLibraries ();
	WIN32_UnregisterClasses ();
#ifdef _DEBUG
	// show memory leaks
	//_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
	for (i = 0; i < argc; i++)
		xfree (argv[i]);
	xfree (argv);
	if (argv2) {
		for (i = 0; argv2[i]; i++)
			xfree (argv2[i]);
		xfree (argv2);
	}
	for (i = 0; argv3 && argv3[i]; i++)
		xfree (argv3[i]);
	xfree (argv3);
	return FALSE;
}

#if 0
int execute_command (TCHAR *cmd)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset (&si, 0, sizeof (si));
	si.cb = sizeof (si);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	if(CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))  {
		WaitForSingleObject(pi.hProcess, INFINITE);
		return 1;
	}
	return 0;
}
#endif

#include "driveclick.h"
static int drvsampleres[] = {
	IDR_DRIVE_CLICK_A500_1, DS_CLICK,
	IDR_DRIVE_SPIN_A500_1, DS_SPIN,
	IDR_DRIVE_SPINND_A500_1, DS_SPINND,
	IDR_DRIVE_STARTUP_A500_1, DS_START,
	IDR_DRIVE_SNATCH_A500_1, DS_SNATCH,
	-1
};
int driveclick_loadresource (struct drvsample *sp, int drivetype)
{
	int i, ok;

	ok = 1;
	for (i = 0; drvsampleres[i] >= 0; i += 2) {
		struct drvsample *s = sp + drvsampleres[i + 1];
		HRSRC res = FindResource (NULL, MAKEINTRESOURCE (drvsampleres[i + 0]), L"WAVE");
		if (res != 0) {
			HANDLE h = LoadResource (NULL, res);
			int len = SizeofResource (NULL, res);
			uae_u8 *p = (uae_u8*)LockResource (h);
			s->p = decodewav (p, &len);
			s->len = len;
		} else {
			ok = 0;
		}
	}
	return ok;
}

#if defined(_WIN64)

LONG WINAPI WIN32_ExceptionFilter (struct _EXCEPTION_POINTERS * pExceptionPointers, DWORD ec)
{
	write_log (L"EVALEXCEPTION!\n");
	return EXCEPTION_EXECUTE_HANDLER;
}
#else

#if 0
#include <errorrep.h>
#endif
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
	CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

/* Gah, don't look at this crap, please.. */
static void efix (DWORD *regp, void *p, void *ps, int *got)
{
	DWORD reg = *regp;
	if (p >= (void*)reg && p < (void*)(reg + 32)) {
		*regp = (DWORD)ps;
		*got = 1;
	}
}

static void savedump (MINIDUMPWRITEDUMP dump, HANDLE f, struct _EXCEPTION_POINTERS *pExceptionPointers)
{
	MINIDUMP_EXCEPTION_INFORMATION exinfo;
	MINIDUMP_USER_STREAM_INFORMATION musi, *musip;
	MINIDUMP_USER_STREAM mus[2], *musp;
	uae_u8 *log;
	int loglen;

	musip = NULL;
	loglen = 30000;
	log = save_log (TRUE, &loglen);
	if (log) {
		musi.UserStreamArray = mus;
		musi.UserStreamCount = 1;
		musp = &mus[0];
		musp->Type = LastReservedStream + 1;
		musp->Buffer = log;
		musp->BufferSize = loglen;
		musip = &musi;
		loglen = 30000;
		log = save_log (FALSE, &loglen);
		if (log) {
			musi.UserStreamCount++;
			musp = &mus[1];
			musp->Type = LastReservedStream + 2;
			musp->Buffer = log;
			musp->BufferSize = loglen;
		}
	}
	exinfo.ThreadId = GetCurrentThreadId ();
	exinfo.ExceptionPointers = pExceptionPointers;
	exinfo.ClientPointers = 0;
	dump (GetCurrentProcess (), GetCurrentProcessId (), f, minidumpmode, &exinfo, musip, NULL);
}

LONG WINAPI WIN32_ExceptionFilter (struct _EXCEPTION_POINTERS *pExceptionPointers, DWORD ec)
{
	static uae_u8 *prevpc;
	LONG lRet = EXCEPTION_CONTINUE_SEARCH;
	PEXCEPTION_RECORD er = pExceptionPointers->ExceptionRecord;
	PCONTEXT ctx = pExceptionPointers->ContextRecord;

	/* Check possible access violation in 68010+/compatible mode disabled if PC points to non-existing memory */
#if 1
	if (ec == EXCEPTION_ACCESS_VIOLATION && !er->ExceptionFlags &&
		er->NumberParameters >= 2 && !er->ExceptionInformation[0] && regs.pc_p) {
			void *p = (void*)er->ExceptionInformation[1];
			write_log (L"ExceptionFilter Trap: %p %p %p\n", p, regs.pc_p, prevpc);
			if ((p >= (void*)regs.pc_p && p < (void*)(regs.pc_p + 32))
				|| (p >= (void*)prevpc && p < (void*)(prevpc + 32))) {
					int got = 0;
					uaecptr opc = m68k_getpc ();
					void *ps = get_real_address (0);
					m68k_dumpstate (0, 0);
					efix (&ctx->Eax, p, ps, &got);
					efix (&ctx->Ebx, p, ps, &got);
					efix (&ctx->Ecx, p, ps, &got);
					efix (&ctx->Edx, p, ps, &got);
					efix (&ctx->Esi, p, ps, &got);
					efix (&ctx->Edi, p, ps, &got);
					write_log (L"Access violation! (68KPC=%08X HOSTADDR=%p)\n", M68K_GETPC, p);
					if (got == 0) {
						write_log (L"failed to find and fix the problem (%p). crashing..\n", p);
					} else {
						void *ppc = regs.pc_p;
						m68k_setpc (0);
						if (ppc != regs.pc_p) {
							prevpc = (uae_u8*)ppc;
						}
						m68k_setpc ((uaecptr)p);
						exception2 (opc);
						lRet = EXCEPTION_CONTINUE_EXECUTION;
					}
			}
	}
#endif
#ifndef	_DEBUG
	if (lRet == EXCEPTION_CONTINUE_SEARCH) {
		TCHAR path[MAX_DPATH];
		TCHAR path2[MAX_DPATH];
		TCHAR msg[1024];
		TCHAR *p;
		HMODULE dll = NULL;
		struct tm when;
		__time64_t now;

		if (os_winnt && GetModuleFileName (NULL, path, MAX_DPATH)) {
			TCHAR beta[100];
			TCHAR *slash = _tcsrchr (path, '\\');
			_time64 (&now);
			when = *_localtime64 (&now);
			_tcscpy (path2, path);
			if (slash) {
				_tcscpy (slash + 1, L"DBGHELP.DLL");
				dll = WIN32_LoadLibrary (path);
			}
			slash = _tcsrchr (path2, '\\');
			if (slash)
				p = slash + 1;
			else
				p = path2;
			beta[0] = 0;
			if (WINUAEPUBLICBETA > 0)
				_stprintf (beta, L"b%s", WINUAEBETA);
			_stprintf (p, L"winuae_%d%d%d%s_%d%02d%02d_%02d%02d%02d.dmp",
				UAEMAJOR, UAEMINOR, UAESUBREV, beta,
				when.tm_year + 1900, when.tm_mon + 1, when.tm_mday, when.tm_hour, when.tm_min, when.tm_sec);
			if (dll == NULL)
				dll = WIN32_LoadLibrary (L"DBGHELP.DLL");
			if (dll) {
				MINIDUMPWRITEDUMP dump = (MINIDUMPWRITEDUMP)GetProcAddress (dll, "MiniDumpWriteDump");
				if (dump) {
					HANDLE f = CreateFile (path2, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if (f != INVALID_HANDLE_VALUE) {
						flush_log ();
						savedump (dump, f, pExceptionPointers);
						CloseHandle (f);
						if (isfullscreen () <= 0) {
							_stprintf (msg, L"Crash detected. MiniDump saved as:\n%s\n", path2);
							MessageBox (NULL, msg, L"Crash", MB_OK | MB_ICONWARNING | MB_TASKMODAL | MB_SETFOREGROUND);
						}
					}
				}
			}
		}
	}
#endif
#if 0
	HMODULE hFaultRepDll = LoadLibrary (L"FaultRep.dll") ;
	if (hFaultRepDll) {
		pfn_REPORTFAULT pfn = (pfn_REPORTFAULT)GetProcAddress (hFaultRepDll, L"ReportFault");
		if (pfn) {
			EFaultRepRetVal rc = pfn (pExceptionPointers, 0);
			lRet = EXCEPTION_EXECUTE_HANDLER;
		}
		FreeLibrary (hFaultRepDll );
	}
#endif
	return lRet ;
}

#endif

const static GUID GUID_DEVINTERFACE_HID =  { 0x4D1E55B2L, 0xF16F, 0x11CF,
{ 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

typedef ULONG (CALLBACK *SHCHANGENOTIFYREGISTER)
	(HWND hwnd,
	int fSources,
	LONG fEvents,
	UINT wMsg,
	int cEntries,
	const SHChangeNotifyEntry *pshcne);
typedef BOOL (CALLBACK *SHCHANGENOTIFYDEREGISTER)(ULONG ulID);

void addnotifications (HWND hwnd, int remove, int isgui)
{
	static ULONG ret;
	static HDEVNOTIFY hdn;
	static int wtson;
	LPITEMIDLIST ppidl;
	SHCHANGENOTIFYREGISTER pSHChangeNotifyRegister;
	SHCHANGENOTIFYDEREGISTER pSHChangeNotifyDeregister;

	pSHChangeNotifyRegister = (SHCHANGENOTIFYREGISTER)GetProcAddress (
		GetModuleHandle (L"shell32.dll"), "SHChangeNotifyRegister");
	pSHChangeNotifyDeregister = (SHCHANGENOTIFYDEREGISTER)GetProcAddress (
		GetModuleHandle (L"shell32.dll"), "SHChangeNotifyDeregister");

	if (remove) {
		if (ret > 0 && pSHChangeNotifyDeregister)
			pSHChangeNotifyDeregister (ret);
		ret = 0;
		if (hdn)
			UnregisterDeviceNotification (hdn);
		hdn = 0;
		if (os_winxp && wtson && !isgui)
			WTSUnRegisterSessionNotification (hwnd);
		wtson = 0;
	} else {
		DEV_BROADCAST_DEVICEINTERFACE NotificationFilter = { 0 };
		if(pSHChangeNotifyRegister && SHGetSpecialFolderLocation (hwnd, CSIDL_DESKTOP, &ppidl) == NOERROR) {
			SHChangeNotifyEntry shCNE;
			shCNE.pidl = ppidl;
			shCNE.fRecursive = TRUE;
			ret = pSHChangeNotifyRegister (hwnd, SHCNE_DISKEVENTS, SHCNE_MEDIAINSERTED | SHCNE_MEDIAREMOVED,
				WM_USER + 2, 1, &shCNE);
		}
		NotificationFilter.dbcc_size = 
			sizeof(DEV_BROADCAST_DEVICEINTERFACE);
		NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_HID;
		hdn = RegisterDeviceNotification (hwnd,  &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
		if (os_winxp && !isgui)
			wtson = WTSRegisterSessionNotification (hwnd, NOTIFY_FOR_THIS_SESSION);
	}
}

void systray (HWND hwnd, int remove)
{
	NOTIFYICONDATA nid;
	BOOL v;

#ifdef RETROPLATFORM
	if (rp_isactive ())
		return;
#endif
	//write_log (L"notif: systray(%x,%d)\n", hwnd, remove);
	if (!remove) {
		TaskbarRestart = RegisterWindowMessage (L"TaskbarCreated");
		TaskbarRestartHWND = hwnd;
		//write_log (L"notif: taskbarrestart = %d\n", TaskbarRestart);
	} else {
		TaskbarRestart = 0;
		hwnd = TaskbarRestartHWND;
	}
	if (!hwnd)
		return;
	memset (&nid, 0, sizeof (nid));
	nid.cbSize = sizeof (nid);
	nid.hWnd = hwnd;
	nid.hIcon = LoadIcon (hInst, (LPCWSTR)MAKEINTRESOURCE (IDI_APPICON));
	nid.uFlags = NIF_ICON | NIF_MESSAGE;
	nid.uCallbackMessage = WM_USER + 1;
	v = Shell_NotifyIcon (remove ? NIM_DELETE : NIM_ADD, &nid);
	//write_log (L"notif: Shell_NotifyIcon returned %d\n", v);
	if (v) {
		if (remove)
			TaskbarRestartHWND = NULL;
	} else {
		DWORD err = GetLastError ();
		write_log (L"Notify error code = %x (%d)\n",  err, err);
	}
}

static void systraymenu (HWND hwnd)
{
	POINT pt;
	HMENU menu, menu2, drvmenu, cdmenu;
	int drvs[] = { ID_ST_DF0, ID_ST_DF1, ID_ST_DF2, ID_ST_DF3, -1 };
	int i;
	TCHAR text[100], text2[100];

	WIN32GUI_LoadUIString (IDS_STMENUNOFLOPPY, text, sizeof (text) / sizeof (TCHAR));
	WIN32GUI_LoadUIString (IDS_STMENUNOCD, text2, sizeof (text2) / sizeof (TCHAR));
	GetCursorPos (&pt);
	menu = LoadMenu (hUIDLL ? hUIDLL : hInst, MAKEINTRESOURCE (IDM_SYSTRAY));
	if (!menu)
		return;
	menu2 = GetSubMenu (menu, 0);
	drvmenu = GetSubMenu (menu2, 1);
	cdmenu = GetSubMenu (menu2, 2);
	EnableMenuItem (menu2, ID_ST_HELP, pHtmlHelp ? MF_ENABLED : MF_GRAYED);
	i = 0;
	while (drvs[i] >= 0) {
		TCHAR s[MAX_DPATH];
		if (currprefs.floppyslots[i].df[0])
			_stprintf (s, L"DF%d: [%s]", i, currprefs.floppyslots[i].df);
		else
			_stprintf (s, L"DF%d: [%s]", i, text);
		ModifyMenu (drvmenu, drvs[i], MF_BYCOMMAND | MF_STRING, drvs[i], s);
		EnableMenuItem (menu2, drvs[i], currprefs.floppyslots[i].dfxtype < 0 ? MF_GRAYED : MF_ENABLED);
		i++;
	}
	{
		TCHAR s[MAX_DPATH];
		if (currprefs.cdslots[0].inuse && currprefs.cdslots[0].name[0])
			_stprintf (s, L"CD: [%s]", currprefs.cdslots[0].name);
		else
			_stprintf (s, L"CD: [%s]", text2);
		ModifyMenu (cdmenu, ID_ST_CD0, MF_BYCOMMAND | MF_STRING, ID_ST_CD0, s);
		int open = 0;
		struct device_info di;
		if (sys_command_info (0, &di, 1) && di.open)
			open = 1;
		EnableMenuItem (menu2, ID_ST_CD0, open == 0 ? MF_GRAYED : MF_ENABLED);
	}


	if (isfullscreen () <= 0)
		SetForegroundWindow (hwnd);
	TrackPopupMenu (menu2, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
		pt.x, pt.y, 0, hwnd, NULL);
	PostMessage (hwnd, WM_NULL, 0, 0);
	DestroyMenu (menu);
}

static void LLError (HMODULE m, const TCHAR *s)
{
	DWORD err;

	if (m) {
		//	write_log (L"'%s' opened\n", s);
		return;
	}
	err = GetLastError ();
	if (err == ERROR_MOD_NOT_FOUND || err == ERROR_DLL_NOT_FOUND)
		return;
	write_log (L"%s failed to open %d\n", s, err);
}

HMODULE WIN32_LoadLibrary_2 (const TCHAR *name, int expand)
{
	HMODULE m = NULL;
	TCHAR *newname;
	DWORD err = -1;
#ifdef CPU_64_BIT
	TCHAR *p;
#endif
	int round;

	newname = xmalloc (TCHAR, _tcslen (name) + 1 + 10);
	if (!newname)
		return NULL;
	for (round = 0; round < 6; round++) {
		TCHAR s[MAX_DPATH];
		_tcscpy (newname, name);
#ifdef CPU_64_BIT
		switch(round)
		{
		case 0:
			p = _tcsstr (newname, L"32");
			if (p) {
				p[0] = '6';
				p[1] = '4';
			}
			break;
		case 1:
			p = _tcschr (newname, '.');
			_tcscpy (p, L"_x64");
			_tcscat (p, _tcschr (name, '.'));
			break;
		case 2:
			p = _tcschr (newname, '.');
			_tcscpy (p, L"x64");
			_tcscat (p, _tcschr (name, '.'));
			break;
		case 3:
			p = _tcschr (newname, '.');
			_tcscpy (p, L"_64");
			_tcscat (p, _tcschr (name, '.'));
			break;
		case 4:
			p = _tcschr (newname, '.');
			_tcscpy (p, L"64");
			_tcscat (p, _tcschr (name, '.'));
			break;
		}
#endif
		get_plugin_path (s, sizeof s / sizeof (TCHAR), NULL);
		_tcscat (s, newname);
		m = LoadLibrary (s);
		LLError (m ,s);
		if (m)
			goto end;
		m = LoadLibrary (newname);
		LLError (m, newname);
		if (m)
			goto end;
#ifndef CPU_64_BIT
		break;
#endif
	}
end:
	xfree (newname);
	return m;
}
HMODULE WIN32_LoadLibrary (const TCHAR *name)
{
	return WIN32_LoadLibrary_2 (name, TRUE);
}
HMODULE WIN32_LoadLibrary2 (const TCHAR *name)
{
	HMODULE m = LoadLibrary (name);
	LLError (m, name);
	return m;
}

int isdllversion (const TCHAR *name, int version, int revision, int subver, int subrev)
{
	DWORD  dwVersionHandle, dwFileVersionInfoSize;
	LPVOID lpFileVersionData = NULL;
	int ok = 0;

	dwFileVersionInfoSize = GetFileVersionInfoSize (name, &dwVersionHandle);
	if (dwFileVersionInfoSize) {
		if (lpFileVersionData = xcalloc (uae_u8, dwFileVersionInfoSize)) {
			if (GetFileVersionInfo (name, dwVersionHandle, dwFileVersionInfoSize, lpFileVersionData)) {
				VS_FIXEDFILEINFO *vsFileInfo = NULL;
				UINT uLen;
				if (VerQueryValue (lpFileVersionData, TEXT("\\"), (void **)&vsFileInfo, &uLen)) {
					if (vsFileInfo) {
						uae_u64 v1 = ((uae_u64)vsFileInfo->dwProductVersionMS << 32) | vsFileInfo->dwProductVersionLS;
						uae_u64 v2 = ((uae_u64)version << 48) | ((uae_u64)revision << 32) | (subver << 16) | (subrev << 0);
						write_log (L"%s %d.%d.%d.%d\n", name,
							HIWORD (vsFileInfo->dwProductVersionMS), LOWORD (vsFileInfo->dwProductVersionMS),
							HIWORD (vsFileInfo->dwProductVersionLS), LOWORD (vsFileInfo->dwProductVersionLS));
						if (v1 >= v2)
							ok = 1;
					}
				}
			}
			xfree (lpFileVersionData);
		}
	}
	return ok;
}

int get_guid_target (uae_u8 *out)
{
	GUID guid;

	if (CoCreateGuid (&guid) != S_OK)
		return 0;
	out[0] = guid.Data1 >> 24;
	out[1] = guid.Data1 >> 16;
	out[2] = guid.Data1 >>  8;
	out[3] = guid.Data1 >>  0;
	out[4] = guid.Data2 >>  8;
	out[5] = guid.Data2 >>  0;
	out[6] = guid.Data3 >>  8;
	out[7] = guid.Data3 >>  0;
	memcpy (out + 8, guid.Data4, 8);
	return 1;
}

typedef HRESULT (CALLBACK* SHCREATEITEMFROMPARSINGNAME)
	(PCWSTR,IBindCtx*,REFIID,void**); // Vista+ only


void target_addtorecent (const TCHAR *name, int t)
{
	TCHAR tmp[MAX_DPATH];

	tmp[0] = 0;
	GetFullPathName (name, sizeof tmp / sizeof (TCHAR), tmp, NULL);
	if (os_win7) {
		SHCREATEITEMFROMPARSINGNAME pSHCreateItemFromParsingName;
		SHARDAPPIDINFO shard;
		pSHCreateItemFromParsingName = (SHCREATEITEMFROMPARSINGNAME)GetProcAddress (
			GetModuleHandle (L"shell32.dll"), "SHCreateItemFromParsingName");
		if (!pSHCreateItemFromParsingName)
			return;
		shard.pszAppID = WINUAEAPPNAME;
		if (SUCCEEDED (pSHCreateItemFromParsingName (tmp, NULL, IID_IShellItem, (void**)&shard.psi))) {
			SHAddToRecentDocs (SHARD_APPIDINFO, &shard);
			shard.psi->Release();
		}
	} else {
		SHAddToRecentDocs (SHARD_PATH, tmp);
	}
}


void target_reset (void)
{
	clipboard_reset ();
}

uae_u32 emulib_target_getcpurate (uae_u32 v, uae_u32 *low)
{
	*low = 0;
	if (v == 1) {
		LARGE_INTEGER pf;
		pf.QuadPart = 0;
		QueryPerformanceFrequency (&pf);
		*low = pf.LowPart;
		return pf.HighPart;
	} else if (v == 2) {
		LARGE_INTEGER pf;
		pf.QuadPart = 0;
		QueryPerformanceCounter (&pf);
		*low = pf.LowPart;
		return pf.HighPart;
	}
	return 0;
}

void fpux_save (int *v)
{
#ifndef _WIN64
	*v = _controlfp (fpucontrol, _MCW_IC | _MCW_RC | _MCW_PC);
#endif
}
void fpux_restore (int *v)
{
#ifndef _WIN64
	if (v)
		_controlfp (*v, _MCW_IC | _MCW_RC | _MCW_PC);
#else
		_controlfp (fpucontrol, _MCW_IC | _MCW_RC | _MCW_PC);
#endif
}

typedef BOOL (CALLBACK* SETPROCESSDPIAWARE)(void);
typedef BOOL (CALLBACK* CHANGEWINDOWMESSAGEFILTER)(UINT, DWORD);

int PASCAL wWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	SETPROCESSDPIAWARE pSetProcessDPIAware;
	DWORD_PTR sys_aff;
	HANDLE thread;

#if 0
#ifdef _DEBUG
	{
		int tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		//tmp &= 0xffff;
		tmp |= _CRTDBG_CHECK_ALWAYS_DF;
		tmp |= _CRTDBG_CHECK_CRT_DF;
#ifdef MEMDEBUG
		tmp |=_CRTDBG_CHECK_EVERY_16_DF;
		tmp |= _CRTDBG_DELAY_FREE_MEM_DF;
#endif
		_CrtSetDbgFlag(tmp);
	}
#endif
#endif
	SetDllDirectory (L"");
	/* Make sure we do an InitCommonControls() to get some advanced controls */
	InitCommonControls ();

	original_affinity = 1;
	GetProcessAffinityMask (GetCurrentProcess (), &original_affinity, &sys_aff);

	thread = GetCurrentThread ();
	//original_affinity = SetThreadAffinityMask(thread, 1);
	fpucontrol = _controlfp (0, 0) & (_MCW_IC | _MCW_RC | _MCW_PC);

#if 0
#define MSGFLT_ADD 1
	CHANGEWINDOWMESSAGEFILTER pChangeWindowMessageFilter;
	pChangeWindowMessageFilter = (CHANGEWINDOWMESSAGEFILTER)GetProcAddress(
		GetModuleHandle(L"user32.dll"), L"ChangeWindowMessageFilter");
	if (pChangeWindowMessageFilter)
		pChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
#endif

	pSetProcessDPIAware = (SETPROCESSDPIAWARE)GetProcAddress (
		GetModuleHandle (L"user32.dll"), "SetProcessDPIAware");
	if (pSetProcessDPIAware)
		pSetProcessDPIAware ();
	log_open (NULL, 0, 0);

	__try {
		WinMain2 (hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	} __except(WIN32_ExceptionFilter (GetExceptionInformation (), GetExceptionCode ())) {
	}
	//SetThreadAffinityMask (thread, original_affinity);
	return FALSE;
}


