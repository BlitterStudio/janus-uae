/************************************************************************ 
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
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

#include "j.h"

//int coord_native_to_amiga_x (int x);
int coord_native_to_amiga_y (int y);

extern int diwstrt, diwhigh;
extern int *native2amiga_line_map;
extern int thisframe_y_adjust;

/***********************************************************************
 * nonP96
 *
 * tries to get the coords right for non Picasso screens.
 *
 * This is a wild guess on some parts, sorry.
 *
 * SuperHires/LowRes scaling is done in the native part.
 *
 ***********************************************************************/
static uae_u32 nonP96(struct Screen *screen, ULONG *m68k_results) {
  LONG x,y;
  LONG arosx, arosy;
  WORD  diwx, diwy, diwhx;
  WORD  dx;

  JWLOG("entered\n");

  /* W->BorderLeft and W->BorderTop are already part of XOffset and YOffset */
  arosx=screen->MouseX - W->LeftEdge; 
  arosy=screen->MouseY - W->TopEdge;

  /* AROS mouse x coord to amigaOS x coord */

  //JWLOG("FOO: 0xdff08E: %x\n", diwstrt);
  diwx=diwstrt & 0xb11111111;
  //JWLOG("FOO: diwx:     %x\n", diwx);
  diwhx=diwhigh & 0xb100000; /* H8 bit is nr 5 */
  //JWLOG("FOO: diwhx:    %x\n", diwhx);
  dx=diwx+diwhx;

  JWLOG("FOO: arosx:        %4d\n", arosx);
  JWLOG("FOO: dx offset:    %4d\n", -(dx/2));
  x=arosx - (dx/2);

  JWLOG("FOO:               ----\n");
  JWLOG("FOO: arosx-dx:     %4d\n", x);
  x=x + 0x40; /* DISPLAY_LEFT_SHIFT */
  JWLOG("FOO:      +0x40:   %4d\n", 0x40);
  JWLOG("FOO:               ----\n");
  JWLOG("FOO x:             %4d\n", x);

  x=x + (9*2) + 2; /* 2*DIW_DDF_OFFSET + HBLANK_OFFSET(=4) !? */
  JWLOG("FOO:    +2+9*2:    %4d\n", 22);
  JWLOG("FOO:               ----\n");
  JWLOG("FOO: x:           %4d\n", x);
  JWLOG("FOO: XOffset:    -%4d\n", XOffset);
  JWLOG("FOO:               ----\n");
  x=x - XOffset;
  JWLOG("FOO x:            %4d\n", x);

  /* for superhires: *2, for lores screens: /2 
   * native part does this (janusd) 
   */

  put_long(m68k_results, x); 
  //put_long_p(m68k_results, screen->MouseX); 
  
  JWLOG("FOO: ------------------------------\n");
  /* AROS mouse y coord to amigaOS y coord */
  JWLOG("FOO: arosy:                       %4d\n", arosy);
  JWLOG("FOO: native2amiga_line_map[%3d]:  %4d\n", arosy, native2amiga_line_map[arosy]);
  JWLOG("FOO: thisframe_y_adjust:          %4d\n", thisframe_y_adjust);
  JWLOG("FOO: minfirstline:               -%4d\n", minfirstline);
  JWLOG("FOO: minfirstline:               -  44\n");
  JWLOG("FOO:                             -----\n");
  y=-44 + coord_native_to_amiga_y(arosy) << 1;
  JWLOG("FOO:                              %4d\n", y);
  y=y-YOffset;
  JWLOG("FOO: YOffset:                    -%4d\n", YOffset);
  JWLOG("FOO:                             -----\n");
  JWLOG("FOO: res:                         %4d\n", y);

  put_long(m68k_results+1, y); 
  //put_long_p(m68k_results+1, screen->MouseY); 

  return TRUE;
}
uae_u32 ad_job_get_mouse(ULONG *m68k_results) {
  struct Screen *screen;
  ULONG modeID;

  //JWLOG("ad_job_get_mouse\n");

  screen=IntuitionBase->FirstScreen;

  if(!screen) {
    //DebOut("no screen available\n");
    return TRUE;
  }

  /* only return mouse movement, if one of our windows is active.
   * as we do not access the result, no Semaphore access is
   * necessary. save semaphore access.
   *
   * if we are not coherent, we need to move the mouse nevertheless
   */
  if(!janus_active_window && changed_prefs.jcoherence) {
    return FALSE;
  }

#if 0
  if(custom_screen_active) {
    return FALSE;
  }
#endif
#if 0
  modeID=GetVPModeID(&(screen->ViewPort));
  if(modeID != INVALID_ID) {
    if(IsCyberModeID(modeID)) {
      /* call test */
      return nonP96(screen, m68k_results);
    }
  }
#endif
  if(get_long(m68k_results) == 0) {
    JWLOG("no P96 screen \n");
    nonP96(screen, m68k_results);
  }

  JWLOG("P96 screen \n");
  if(mice[0].enabled) {
  
    put_long_p(m68k_results, screen->MouseX); 
    put_long_p(m68k_results+1, screen->MouseY); 
  }
  else {
    if(!menux && !menuy) {
      return FALSE;
    }
    /* fake for menu selection */
    put_long_p(m68k_results,   menux); 
    put_long_p(m68k_results+1, menuy); 
    /* clear again */
    menux=0;
    menuy=0;
  }

  return TRUE;
}



