/************************************************************************ 
 *
 * AROS Drawing
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

 /* THIS IS OBSOLETE. USE arosgfx.cpp!! */


#include <proto/intuition.h>
#include <proto/graphics.h>

#include <libraries/cybergraphics.h>
#include <proto/cybergraphics.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "audio.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "traps.h"
#include "xwin.h"
#include "keyboard.h"
#include "drawing.h"
#include "gfx.h"
#include "picasso96_aros.h"
#include "inputdevice.h"
#include "gui.h"
#include "serial.h"
#include "gfxfilter.h"
#include "sampler.h"
#include "sounddep/sound.h"
#ifdef RETROPLATFORM
#include "rp.h"
#endif

static uae_u8 scrlinebuf[4096 * 4]; /* this is too large, but let's rather play on the safe side here */

static BOOL doInit (void);

void enumeratedisplays (int multi) {
    bug("[JUAE:Gfx] %s()\n", __PRETTY_FUNCTION__);

  if (multi) {
    /*
    int cnt = 1;
    DirectDraw_EnumDisplays (displaysCallback);
    EnumDisplayMonitors (NULL, NULL, monitorEnumProc, (LPARAM)&cnt);
    */
    write_log (_T("ERROR: Multimonitor does not work!\n"));
  }
  write_log (_T("Multimonitor detection disabled\n"));
  Displays[0].primary = 1;
  Displays[0].adaptername = my_strdup(_T("Display"));
  Displays[0].monitorname = my_strdup(_T("Display"));

  //Displays[0].disabled = 0;
}


