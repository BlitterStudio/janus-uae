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

LONG invalidate_lasty=0;

/* invalidate(last mouse position) 
 *
 * mark the area from y to y+30 (last position of mouse pointer)
 * as dirty, so in the next frame, the image of the old
 * mouse pointer gets deleted. Otherwise you could get
 * a trail of mousepointers. Not always, so this seems
 * to be a bug somewhere, but I am not willing to hunt it
 * down ;). This fixes it.
 */
static void invalidate(LONG y) {

  JWLOG("invalidate(%d)\n",y);


  if(y<0) {
    return;
  }

  if(invalidate_lasty != y) {
    /* prefs can set a 26 pixel high pointer ..? */
    DX_Invalidate(invalidate_lasty, invalidate_lasty+30); 
    invalidate_lasty=y;
  }
}

/***********************************************************************
 * nonP96
 *
 * tries to get the coords right for non Picasso screens.
 *
 * This is a wild guess on some parts, sorry.
 *
 * A lot is also done in the native part in sync-mouse.c, x coords
 * are completely done 68k native, we better move y coords there, too.
 * (TODO)
 *
 ***********************************************************************/
#define BIT(n) (1<<n)

static uae_u32 nonP96(struct Screen *screen, ULONG *m68k_results) {

  /* NOT USED ANY MORE !! */
  LONG x,y;
  LONG arosx, arosy;
  WORD  diwx, diwy, diwhx;
  WORD  dx;

  JWLOG("entered\n");

  /* x calculation is done on the m68k side in sync-mouse.c */
  arosx=screen->MouseX - W->LeftEdge - W->BorderLeft; 
  //arosx=screen->MouseX - W->LeftEdge - XOffset;
  put_long_p(m68k_results, arosx); 

  JWLOG("XOffset: %d  W->BorderLeft: %d\n", XOffset, W->BorderLeft);

  /* W->BorderTop is already part of YOffset */
  arosy=screen->MouseY - W->TopEdge;
  
  JWLOG("FOO: ------------------------------------\n");
  /* AROS mouse y coord to amigaOS y coord */
  //JWLOG("FOO: native2amiga_line_map[%3d]:  %4d\n", arosy, native2amiga_line_map[arosy]);
  //JWLOG("FOO: thisframe_y_adjust:          %4d\n", thisframe_y_adjust);
  //JWLOG("FOO: minfirstline:               -%4d\n", minfirstline);
  JWLOG("FOO: arosy:                          %4d\n", arosy);
  JWLOG("FOO: coord_native_to_amiga_y(arosy): %4d\n", coord_native_to_amiga_y(arosy));
  JWLOG("FOO: (const)                        -%4d\n", 44);
  JWLOG("FOO:                                -----\n");
  y=coord_native_to_amiga_y(arosy) - 44;
  JWLOG("FOO:                                 %4d\n", y);
  JWLOG("FOO:                                -----\n");
  JWLOG("FOO:                                   *2  \n");
  JWLOG("FOO:                                -----\n");
  y=y*2;
  JWLOG("FOO:                                 %4d\n", y);
  y=y-YOffset;
  JWLOG("FOO: YOffset:                       -%4d\n", YOffset);
  JWLOG("FOO:                                -----\n");
  JWLOG("FOO: res:                            %4d\n", y);

  put_long_p(m68k_results+1, y); 

  invalidate(y);
  return TRUE;
}

extern int visible_left_border;

uae_u32 ad_job_get_mouse(ULONG *m68k_results) {
  struct Screen *screen;
  ULONG modeID;
  LONG x,y;
  BOOL is_p96;
  //LONG left=0;

  //JWLOG("ad_job_get_mouse\n");

  screen=IntuitionBase->FirstScreen;

  if(!screen) {
    //DebOut("no screen available\n");
    return TRUE;
  }

  /* only return mouse movement, if one of our windows is active.
   * as we do not access the result, no Semaphore access is
   * necessary, so save semaphore access here.
   *
   * if we are not coherent, we need to move the mouse nevertheless
   */
  if(!janus_active_window && changed_prefs.jcoherence) {
    return FALSE;
  }

  /* in this case, classic uae mouse mode is enabled */
  if((!uae_main_window_closed) && ((!changed_prefs.jmouse) || (aos3_task==NULL))) {
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

#if 0
  if(get_long_p(m68k_results) == 0) {
    JWLOG("no P96 screen \n");
    return nonP96(screen, m68k_results);
  }
#endif

  is_p96=get_long_p(m68k_results);
  JWLOG("mouse: is_p96: %d \n", is_p96);

  if(mice[0].enabled) {
    JWLOG("mouse: screen->x,y: %d,%d\n", screen->MouseX, screen->MouseY);
    JWLOG("mouse: W->x,y: %d,%d\n", W->LeftEdge, W->TopEdge);
    JWLOG("mouse: border->x,y: %d,%d\n", W->BorderLeft, W->BorderTop);
    JWLOG("mouse: XOffset: %d\n", XOffset);
    JWLOG("mouse: visible_left_border: %d\n", visible_left_border);

    x=screen->MouseX;
    y=screen->MouseY;

    if(!uae_main_window_closed) {
      x=x - W->LeftEdge - W->BorderLeft;
      y=y - W->TopEdge  - W->BorderTop;
    }

#if 0
    if(is_p96 == 0) {
      JWLOG("mouse: add visible_left_border: %d\n", visible_left_border);
      left=visible_left_border; /* for center image, bigger window  etc. */
    }
#endif

    if(x<0) {
      x=0;
    }
    if(y<0) {
      y=0;
    }

    invalidate(y);
 
    put_long_p(m68k_results,   x);
    put_long_p(m68k_results+1, y);
    put_long_p(m68k_results+2, currprefs.gfx_xcenter); /* center_mode */
    put_long_p(m68k_results+3, currprefs.gfx_ycenter); /* center_mode */
    put_long_p(m68k_results+4, gfxvidinfo.width);      /* width of aros display */
    put_long_p(m68k_results+5, gfxvidinfo.height);     /* height of aros display*/
    put_long_p(m68k_results+6, XOffset);               /* XOffset */
    put_long_p(m68k_results+7, YOffset);               /* YOffset */
    JWLOG("mouse: currprefs.gfx_xcenter: %d\n", currprefs.gfx_xcenter);
    JWLOG("mouse: currprefs.gfx_ycenter: %d\n", currprefs.gfx_ycenter);
    JWLOG("mouse: gfxvidinfo.width: %d\n", gfxvidinfo.width);
    JWLOG("mouse: gfxvidinfo.height: %d\n", gfxvidinfo.height);
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



