/************************************************************************ 
 *
 * UAE - The Un*x Amiga Emulator
 *
 * dummy GUI stubs
 *
 * Copyright 1996 Bernd Schmidt
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
#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "gui.h"

static void sigchldhandler(int foo)
{
	TODO();
}

int gui_init (void)
{
	TODO();
    return 0;
}

int gui_update (void)
{
	TODO();
    return 0;
}

void gui_exit (void)
{
	TODO();
}

void gui_fps (int x)
{
	TODO();
}

void gui_led (int led, int on)
{
	TODO();
}

void gui_hd_led (int led)
{
	TODO();
}

void gui_cd_led (int led)
{
	TODO();
}

void gui_filename (int num, const char *name)
{
	TODO();
}

static void getline (char *p)
{
	TODO();
}

void gui_handle_events (void)
{
	TODO();
}

void gui_changesettings (void)
{
	TODO();
}

void gui_update_gfx (void)
{
	TODO();
}

void gui_lock (void)
{
	TODO();
}

void gui_unlock (void)
{
	TODO();
}

void gui_message (const char *format,...)
{
       char msg[2048];
       va_list parms;

       va_start (parms,format);
       vsprintf ( msg, format, parms);
       va_end (parms);

       write_log (msg);
}

#if 0
/* needed for AROS!? */
void gui_display (int shortcut) {
	TODO();

}

void gui_flicker_led (int led, int unitnum, int status) {
	TODO();
}

void gui_disk_image_change (int unitnum, const TCHAR *name, bool writeprotected) {
	TODO();
}

void gui_fps (int fps, int idle) {
	TODO();
}

void gui_gameport_axis_change (int port, int axis, int state, int max) {
	TODO();
}

void gui_gameport_button_change (int port, int button, int onoff) {
	TODO();
}
#endif

