/************************************************************************ 
 *
 * AROS logging
 *
*  Copyright 1997-1998 Mathias Ortmann
*  Copyright 1997-1999 Brian King
 * Copyright 2011      Oliver Brunner - aros<at>oliver-brunner.de
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

#include <proto/dos.h>
#include <proto/timer.h>
#include <time.h>

#define JUAE_DEBUG

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "aros.h"


#define LOG_BOOT   _T("janus-uae_bootlog.txt")
#define LOG_NORMAL _T("janus-uae_log.txt")
#define WRITE_LOG_BUF_SIZE 4096
#define SHOW_CONSOLE 0

static void writeconsole (const TCHAR *buffer);

BPTR debugfile=NULL;

int consoleopen = 0;
int console_logging = 0;
int always_flush_log = 0;

static int logging_started;
static int realconsole;
static int bootlogmode;
static int nodatestamps = 0;
static int lfdetected = 1;
static BOOL got_timer=FALSE;

struct MsgPort *timer_msgport;
struct timerequest *timer_ioreq;
struct Device *TimerBase=NULL;

static int opentimer(ULONG unit){

	if(got_timer) {
		//DebOut("we already opened our timer\n");
		return 1;
	}

	//DebOut("entered(unit=%d)\n", unit);
	timer_msgport = CreateMsgPort();
	timer_ioreq = (timerequest *) CreateIORequest(timer_msgport, sizeof(*timer_ioreq));
	if (timer_ioreq){
		if (OpenDevice(TIMERNAME, unit, (struct IORequest *) timer_ioreq, 0) == 0){
			TimerBase = (struct Device *) timer_ioreq->tr_node.io_Device;
			got_timer=TRUE;
			return 1;
		}
	}
	bug("ERROR: unable to open timer.device\n");
	return 0;
}

static void closetimer(void){

	//DebOut("entered\n");

	if(!got_timer) {
		return;
	}

	if (TimerBase){
		CloseDevice((struct IORequest *) timer_ioreq);
		TimerBase = NULL;
	}
	if(timer_ioreq) {
		DeleteIORequest(timer_ioreq);
		timer_ioreq = NULL;
	}
	if(timer_msgport) {
		DeleteMsgPort(timer_msgport);
		timer_msgport = NULL;
	}
	got_timer=FALSE;
	//DebOut("timer closed\n");
}

BPTR log_open (const TCHAR *name, int append, int bootlog)
{
	BPTR f = NULL;

	//DebOut("name=%s append=%d bootlog=%d\n", name, append, bootlog);

	if (name != NULL) {
		f = Open (name, MODE_NEWFILE);
		bootlogmode = bootlog;
	} else 
	{//if (1) {
		//TCHAR *c = GetCommandLine ();
		//if (_tcsstr (c, L" -console")) {
			//if (GetStdHandle (STD_INPUT_HANDLE) && GetStdHandle (STD_OUTPUT_HANDLE)) {
				consoleopen = -1;
				realconsole = 1;
				//_setmode(_fileno (stdout), _O_U16TEXT);
				//_setmode(_fileno (stdin), _O_U16TEXT);
			//}
		//}
	}

	return f;
}

void log_close (BPTR f)
{
	Close (f);
	closetimer();
}

void logging_open (int bootlog, int append)
{
	TCHAR debugfilename[MAX_DPATH];

	opentimer(UNIT_VBLANK);

  if(!logpath[0]) {
    snprintf(logpath, MAX_DPATH-1, "PROGDIR:%s", LOG_NORMAL);
    fullpath(logpath, MAX_DPATH-1);
  }
  if(!bootlogpath[0]) {
    snprintf(bootlogpath, MAX_DPATH-1, "PROGDIR:%s", LOG_BOOT);
    fullpath(bootlogpath, MAX_DPATH-1);
  }



	debugfilename[0] = 0;
#ifndef	SINGLEFILE
	if (currprefs.win32_logfile)
	{
		_stprintf (debugfilename, _T("%s%s"), start_path_data, LOG_NORMAL);
	}
	if (bootlog)
	{
		_stprintf (debugfilename, _T("%s%s"), start_path_data, LOG_BOOT);
	}
	if (debugfilename[0]) 
	{
		if (!debugfile) 
		{

			debugfile = log_open (debugfilename, append, bootlog);
		}
	}
#endif

}

static void premsg (void) 
{
}

static TCHAR *writets (void)
{
	static TCHAR out[100];
	static TCHAR secs[20];
	struct timeval acttime;

	//DebOut("entered (TimerBase %lx, timer_ioreq %lx, timer_msgport %lx\n", TimerBase, timer_ioreq, timer_msgport);

  opentimer(UNIT_VBLANK); /* does nothing, if already open */

	GetSysTime(&acttime);

	//DebOut("got sys time..\n");

	snprintf(secs, 20, "%u", acttime.tv_secs);

	snprintf(out, 99, "%s-%u: ", secs+strlen(secs)-3, acttime.tv_micro/1000);

#if 0
	if (vsync_counter != 0xffffffff)
		_stprintf (p, L" [%d %03dx%03d]", vsync_counter, current_hpos (), vpos);
	_tcscat (p, L": ");
#endif
	return out;
}

void flush_log (void)
{
	/* not necessary ? */
}

static void writeconsole (const TCHAR *buffer)
{
	printf (buffer);
	fflush (stdout);
}

void write_log (const TCHAR *format, ...)
{
	int count;
	TCHAR buffer[WRITE_LOG_BUF_SIZE], *ts;
	char  aros_buf[WRITE_LOG_BUF_SIZE];
	int bufsize = WRITE_LOG_BUF_SIZE;
	TCHAR *bufp;
	va_list parms;

	premsg ();

	//EnterCriticalSection (&cs);
	va_start (parms, format);
	bufp = buffer;
	for (;;) {
		count = _vsntprintf (bufp, bufsize - 1, format, parms);
		if (count < 0) {
			bufsize *= 10;
			if (bufp != buffer)
				xfree (bufp);
			bufp = xmalloc (TCHAR, bufsize);
			continue;
		}
		break;
	}
	bufp[bufsize - 1] = 0;
	if (!_tcsncmp (bufp, _T("write "), 6))
		bufsize--;
	ts = writets ();
	if (bufp[0] == '*')
		count++;
	if (SHOW_CONSOLE || console_logging) {
		if (lfdetected && ts)
			writeconsole (ts);
		writeconsole (bufp);
	}
	if (debugfile) {
		if (lfdetected && ts) {
			sprintf (aros_buf, _T("%s"), ts);
			Write(debugfile, aros_buf, strlen(aros_buf));
		}
		bug("uae: %s", bufp);
		sprintf (aros_buf, _T("%s"), bufp);
		Write(debugfile, aros_buf, strlen(aros_buf));
	}
	lfdetected = 0;
	if (_tcslen (bufp) > 0 && bufp[_tcslen (bufp) - 1] == '\n')
		lfdetected = 1;
	va_end (parms);

	if (bufp != buffer)
		xfree (bufp);
	if (always_flush_log)
		flush_log ();
	//LeaveCriticalSection (&cs);
}

void logging_init (void)
{
	static int started;
	static int first;
	TCHAR tmp[MAX_DPATH];
	TCHAR tmp2[MAX_DPATH];
	BPTR lock;
	TCHAR sep[2];

	//DebOut("entered (first=%d)\n", first);

	opentimer(UNIT_VBLANK);

	if (first > 1) {
		write_log (_T("** LOGGING RESTART **\n"));
		return;
	}
	//DebOut("ping\n");
	if (first == 1) {
		write_log (_T("Log (%s): '%s%s'\n"), currprefs.win32_logfile ? _T("enabled") : _T("disabled"),
			start_path_data, LOG_NORMAL);
		if (debugfile) {
			//DebOut("log_close!!!!!!\n");
			log_close (debugfile);
		}
		debugfile = 0;
	}
	//DebOut("ping\n");
	logging_open (first ? 0 : 1, 0);
	logging_started = 1;
	first++;
	//DebOut("ping\n");

	write_log(_T("\n%s"), VersionStr);

	write_log (_T("\n(c) 1995-2001 Bernd Schmidt   - Core UAE concept and implementation.")
		_T("\n(c) 1998-2015 Toni Wilen      - Win32 port, core code updates.")
		_T("\n(c) 1996-2001 Brian King      - Win32 port, Picasso96 RTG, and GUI.")
		_T("\n(c) 1996-1999 Mathias Ortmann - Win32 port and bsdsocket support.")
		_T("\n(c) 2000-2001 Bernd Meyer     - JIT engine.")
		_T("\n(c) 2000-2005 Bernd Roesch    - MIDI input, many fixes.")
		_T("\n(c) 2011-2015 Oliver Brunner  - AROS port")
		_T("\n"));
	GetCurrentDirName(tmp , sizeof tmp  / sizeof (TCHAR));
	GetProgramName   (tmp2, sizeof tmp2 / sizeof (TCHAR));
	if(tmp[strlen(tmp)-1] == ':') {
		sep[0]=0;
	}
	else {
		sep[0]='/';
	}
	sep[1]=0;
	write_log (_T("'%s%s%s'\n"), tmp, sep, tmp2);
	//write_log (L"EXE: '%s', DATA: '%s', PLUGIN: '%s'\n", start_path_exe, start_path_data, start_path_plugins);
	//regstatus ();

}

void logging_deinit(void) {

  DebOut("debugfile: %lx\n", debugfile);

  closetimer();

  if (debugfile) {
    log_close (debugfile);
  }
  debugfile=0;
}

void activate_console (void)
{
  if (!consoleopen) return;
  TODO();
}
