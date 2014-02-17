/************************************************************************ 
 *
 * Janus-Daemon
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-Daemon.
 *
 * Janus-Daemon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Janus-Daemon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Janus-Daemon. If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id$
 *
 ************************************************************************/


/*****************************************
 * care for mouse
 *
 * uae has those ugly mouse sync problems,
 * maybe we can get rid of them from here!
 *****************************************/

#include <stdio.h>
#include <stdlib.h>

#include <exec/devices.h>
#include <exec/interrupts.h>
#include <exec/nodes.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <intuition/intuitionbase.h>
#include <intuition/preferences.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <devices/timer.h>
#include <hardware/intbits.h>
#include <cybergraphx/cybergraphics.h>
#ifdef __AROS__
#include <proto/cybergraphics.h>
#endif

#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/timer_protos.h>
#include <clib/graphics_protos.h>

#include "janus-daemon.h"

/* cybergfx proto stuff, I could not find gcc includes anywhere..
 * we need only IsCyberModeID, so this should be sufficient
 */
extern struct Library *CyberGfxBase;

#ifndef CLIB_INTUITION_PROTOS_H
#define CLIB_INTUITION_PROTOS_H
#endif

#ifndef __AROS__
#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif
#endif

#ifndef __AROS__
#define IsCyberModeID(modeID) \
         LP1(0x36, BOOL, IsCyberModeID, ULONG, modeID, d0, \
         , CyberGfxBase)
#endif

extern struct IntuitionBase* IntuitionBase;

ULONG *mousebuffer=NULL;

struct MsgPort        *InputMP;
struct InputEvent     *FakeEvent;
struct InputEvent     *FakeClickEvent;
struct IEPointerPixel *NeoPix;
struct IOStdReq       *InputIO;

BOOL sync_mouse_device_open=FALSE;

void free_sync_mouse() {

  ENTER

  if(InputIO) {
    if(sync_mouse_device_open) {
      CloseDevice((struct IORequest *)InputIO);
      sync_mouse_device_open=FALSE;
    }
    DeleteIORequest(InputIO);
    InputIO=NULL;
  }

  if(NeoPix) {
    FreeVec(NeoPix);
    NeoPix=NULL;
  }

  if(FakeEvent) {
    FreeVec(FakeEvent);
    FakeEvent=NULL;
  }

  if(FakeClickEvent) {
    FreeVec(FakeClickEvent);
    FakeClickEvent=NULL;
  }

  if(InputMP) {
    DeleteMsgPort(InputMP);
    InputMP=NULL;
  }

  LEAVE
}

BOOL init_sync_mouse() {

  ENTER

  InputMP=CreateMsgPort();
  if(InputMP) {
    FakeEvent=AllocVec(sizeof(struct InputEvent), MEMF_PUBLIC);
    FakeClickEvent=AllocVec(sizeof(struct InputEvent), MEMF_PUBLIC);
    if(FakeEvent && FakeClickEvent) {
      NeoPix=AllocVec(sizeof(struct IEPointerPixel), MEMF_PUBLIC);
      if(NeoPix) {
        InputIO=CreateIORequest(InputMP, sizeof(struct IOStdReq));
        if(InputIO) {
          if(!OpenDevice((unsigned char *)"input.device", 0, 
              (struct IORequest *)InputIO, 0)) {
             /* Zero if successful, else an error code is returned. */
             sync_mouse_device_open=TRUE;
             LEAVE
             return TRUE;
          }
        }
      }
    }
  }

  printf("ERROR: init_sync_mouse failed (%lx, %lx, %lx, %lx)\n",
          (ULONG) InputMP, (ULONG) FakeEvent, 
          (ULONG) NeoPix, (ULONG) InputIO);

  free_sync_mouse();

  LEAVE

  return FALSE;
}

/* just click ..
 * 
 * not tested, not used. Might work.
 */
void MouseClick(struct Screen *screen, UWORD button,BOOL click, BOOL release) {

  if(!NeoPix || !FakeClickEvent || !InputIO || !screen) {
    DebOut("ERROR: MouseClick assert failed!\n");
    return;
  }

  if(!click && !release) {
    return;
  }

  if(button==IECODE_NOBUTTON) {
    return;
  }

  FakeClickEvent->ie_EventAddress=NULL;
  FakeClickEvent->ie_Class       =IECLASS_RAWMOUSE;
  FakeClickEvent->ie_SubClass    =0;

  if(click) {
    FakeClickEvent->ie_Code=button;
  }
  else {
    FakeClickEvent->ie_Code=button|IECODE_UP_PREFIX;
  }

  InputIO->io_Data   =(APTR)FakeClickEvent;
  InputIO->io_Length =sizeof(struct InputEvent);
  InputIO->io_Command=IND_WRITEEVENT;
 
  DoIO((struct IORequest *)InputIO);
}

/* SetMouse is based on SetMouse (Freeware)
 * (C) 1994,1995, Ketil Hunn
 */
void SetMouse(struct Screen *screen, WORD x, WORD y, UWORD button, 
                                     BOOL click, BOOL release) {

  ENTER

  if(!NeoPix || !FakeEvent || !InputIO || !screen) {
    DebOut("ERROR: SetMouse assert failed!\n");
    printf("ERROR: SetMouse assert failed!\n");
    return;
  }

  NeoPix->iepp_Screen=(struct Screen *)screen;
  NeoPix->iepp_Position.X=x;
  NeoPix->iepp_Position.Y=y;

  FakeEvent->ie_EventAddress=(APTR)NeoPix;
  FakeEvent->ie_NextEvent=NULL;
  FakeEvent->ie_Class=    IECLASS_NEWPOINTERPOS;
  FakeEvent->ie_SubClass= IESUBCLASS_PIXEL;
  FakeEvent->ie_Code=     IECODE_NOBUTTON;
  FakeEvent->ie_Qualifier=0;

  InputIO->io_Data=(APTR)FakeEvent;
  InputIO->io_Length=sizeof(struct InputEvent);
  InputIO->io_Command=IND_WRITEEVENT;
  DoIO((struct IORequest *)InputIO);

  if((button!=IECODE_NOBUTTON) && (click || release)) {
    FakeEvent->ie_EventAddress=NULL;
    FakeEvent->ie_Class=IECLASS_RAWMOUSE;
    FakeEvent->ie_SubClass=0;

    if(click) {
      /* BUTTON DOWN */
      FakeEvent->ie_Code=button;
    }

    if(release) {
      /* BUTTON UP */
      FakeEvent->ie_Code=button|IECODE_UP_PREFIX;
    }
    FakeEvent->ie_Qualifier=0;

    DoIO((struct IORequest *)InputIO);
  }

  LEAVE
}

/**********************************************************
 * no_p96_overscan
 *
 * add overscan offsets to mouse x,y coordinates
 *
 * only for non Picasso 96 screens
 *
 * The ViewPort is relative to the view.
 **********************************************************/
void no_p96_fix_overscan(struct Screen *screen, WORD *x, WORD *y) {

  DebOut("no_p96_fix_overscan(%d, %d)\n", *x, *y);

  DebOut("no_96: screen->ViewPort.DxOffset: %d\n", screen->ViewPort.DxOffset);
  DebOut("no_96: screen->ViewPort.DyOffset: %d\n", screen->ViewPort.DyOffset);

  *x=*x - screen->ViewPort.DxOffset; /* DxOffset is a negative value for overscan */
  *y=*y - screen->ViewPort.DyOffset; /* DxOffset is a negative value for overscan */

  DebOut("no_p96_fix_overscan: x,y: %d, %d\n",*x,*y);
}

/**********************************************************
 * no_p96_fix_resolution
 *
 * scale for diferrent resolutions (lores, hires, interlace)
 *
 * only for non Picasso 96 screens
 **********************************************************/
void no_p96_fix_resolution(struct Screen *screen, WORD *x, WORD *y) {

  ULONG modeID;

  DebOut("no_p96_fix_resolution(%d, %d)\n", *x, *y);
  
  modeID=GetVPModeID(&(screen->ViewPort));

  if(modeID == INVALID_ID) {
    DebOut("no_p96_fix_resolution WARNING: modeID == INVALID_ID\n");
    return;
  }

  if(modeID & SUPERHIRES) {
    *x=*x * 2;
    DebOut("==>SUPER_KEY\n");
  }
  else if(modeID & HIRES) {
    DebOut("==>HIRES_KEY\n");
  }
  else { 
    DebOut("==>LORES_KEY\n");
    *x=*x / 2;
//    y=y/2;
  }
  /* lores */
  if(! (modeID & LACE) ) {
    DebOut("==>no INTERLACE\n");
    *y=*y / 2;
  }

  DebOut("no_p96_fix_resolution: x,y: %d, %d\n",*x,*y);
}

/**********************************************************
 * no_p96_fix_left
 *
 * should correct the centering etc. inside the AROS UAE
 * main window / screen.
 **********************************************************/
void no_p96_fix_left(struct Screen *screen, WORD *x, WORD *y, LONG left) {

  DebOut("no_p96_fix_left(%d, %d, left %d)\n", *x, *y, left);

//  *x=*x + ( 2 * 108); /* 108? why, I don't know ;) */
  *x=*x + ( 2 * (left+2)); 
}

void no_p96_fix_center(struct Screen *screen, 
                       WORD *x, WORD *y, 
                       ULONG gfx_xcenter, ULONG gfx_ycenter, 
                       ULONG aros_width,  ULONG aros_height,
                       LONG XOffset,      LONG YOffset) {

#if 0
  ULONG width, height;
  ULONG modeID;
#endif

  DebOut("no_p96_fix_center(%d, %d, %d, %d, %d, %d)\n", *x, *y, 
                                                        gfx_xcenter, gfx_ycenter,
                                                        aros_width,  aros_height);

#if 0
  modeID=GetVPModeID(&(screen->ViewPort));

  if(modeID == INVALID_ID) {
    DebOut("no_p96_fix_center WARNING: modeID == INVALID_ID\n");
    return;
  }

  if(modeID & SUPERHIRES) {
    width= screen->Width /2;
  }
  else if(modeID & HIRES) {
    width= screen->Width;
  }
  else { 
    width= screen->Width *2;
  }
  /* lores */
#if 0
  if(! (modeID & LACE) ) {
    DebOut("==>no INTERLACE\n");
    *y=*y / 2;
  }
#endif


  DebOut("aros_width %d - width %d = %d\n", aros_width, width, 
                                            aros_width - width);

  *x= *x - ((aros_width - width) / 2); /* center */

  *x= *x + 256;
#endif

  *x=*x-XOffset;
  *y=*y-YOffset;

}


/**********************************************************
 * no_p96_fix_viewoffset
 *
 * The view is the offset you specify in the "Edit
 * graphics" of the overscan prefs. It defines the
 * 0,0 point in distance to the upper left monitor
 * corner. Those are always lores points (?).
 **********************************************************/
void no_p96_fix_viewoffset(struct Screen *screen, WORD *x, WORD *y) {
  struct View          *view;

  DebOut("no_p96_fix_viewoffset(%d, %d)\n", *x, *y);

  view=ViewAddress();

  DebOut("no_96: screen->View.DxOffset: %d\n", view->DxOffset);
  DebOut("no_96: screen->View.DyOffset: %d\n", view->DyOffset);

  *x=*x - (2 * (view->DxOffset - 107)); /* 107? why? I really don't know ;) */
  *y=*y - (2 * (view->DyOffset - 27));  /* 27 ..? */

  DebOut("no_p96_fix_viewoffset: x,y: %d, %d\n",*x,*y);
}

/***************************************
 * is_cyber
 *
 * return, if screen is a Picasso Screen
 ***************************************/
BOOL is_cyber(struct Screen *screen) {
  ULONG modeID;

#ifndef __AROS__
  if(!CyberGfxBase) {  /* no CyberGfxBase, is_p96 is always FALSE */
    return FALSE;
  }
#endif

  if(!screen) {
    DebOut("screen is (still) NULL\n");
    return FALSE;
  }

  /*DebOut("call GetVPModeID (screen %lx, vp: %lx)\n", screen, &(screen->ViewPort));*/

  /* AROS always has CyberGfxBase */
  modeID=GetVPModeID(&(screen->ViewPort));

  if(modeID != INVALID_ID) {
    return IsCyberModeID(modeID);
  }

  DebOut("modeID == INVALID_ID\n");

  return FALSE;
}

/***************************************
 * we check the aros mouse coords and
 * place ours accordingly
 ***************************************/
void sync_mouse() {
  struct Screen        *screen;
  ULONG                 result;
#if 0
  ULONG                 modeID;
#endif
  BOOL                  is_p96=FALSE;
  WORD                  x,y;
  ULONG                 aros_width,   aros_height;
  ULONG                 gfx_xcenter,  gfx_ycenter;
  LONG                  aros_xoffset, aros_yoffset;
#if 0
  UWORD          flags;
#endif

  ENTER

  screen=IntuitionBase->FirstScreen;
  if(!screen) {
    DebOut("sync_mouse: screen==NULL?!\n");
    return;
  }

#if 0
  /* customs screens are *not* synced here */
  flags=screen->Flags & 0x000F;
  if(flags == CUSTOMSCREEN) {
    return;
  }
#endif

  if(!mousebuffer) {
    mousebuffer=AllocVec(AD__MAXMEM, MEMF_CLEAR); /* never free'ed */
  }

  /* do it for every update, as a screen might change the resolution.. */
#if 0
  if(CyberGfxBase) {  /* no CyberGfxBase, is_p96 is always FALSE */
    modeID=GetVPModeID(&(screen->ViewPort));
    /*
    DebOut("modeID=%lx\n", modeID);
    */
    if(modeID != INVALID_ID) {
      is_p96=IsCyberModeID(modeID);
      /*
      DebOut("is_p96: %d\n", is_p96);
      */
    }
  }
  else {
    /* is_p96 stays FALSE */
    is_p96=FALSE;
  }
#endif

  is_p96=is_cyber(screen);

  mousebuffer[0]=is_p96;
  DebOut("is_p96: %d\n",is_p96);

  result = calltrap (AD_GET_JOB, AD_GET_JOB_GET_MOUSE, mousebuffer);

  if(!result) {
    LEAVE
    return;
  }

  x=(WORD) mousebuffer[0];
  y=(WORD) mousebuffer[1];

  DebOut("AD_GET_JOB_GET_MOUSE result: %d, %d\n", x, y);

  if(!is_p96) {
    DebOut("no_p96 =============================\n");

    gfx_xcenter =(ULONG) mousebuffer[2];
    gfx_ycenter =(ULONG) mousebuffer[3];
    aros_width  =(ULONG) mousebuffer[4];
    aros_height =(ULONG) mousebuffer[5];
    aros_xoffset=(LONG)  mousebuffer[6];
    aros_yoffset=(LONG)  mousebuffer[7];

    no_p96_fix_center    (screen, &x, &y, 
                          gfx_xcenter, gfx_ycenter, 
                          aros_width,  aros_height,
                          aros_xoffset, aros_yoffset);
    no_p96_fix_viewoffset(screen, &x, &y);
    no_p96_fix_resolution(screen, &x, &y);
    no_p96_fix_overscan  (screen, &x, &y);

    DebOut("no_96: x,y: %d, %d (after div)\n",x,y);

  }

  if(screen->MouseX != x || screen->MouseY != y) {
    DebOut("set mouse to: %d, %d\n",x,y);
    SetMouse(screen, x, y, 0, FALSE, FALSE);
  }
  else {
    DebOut("mouse already at: %d, %d\n",x,y);
  }

  LEAVE
}

