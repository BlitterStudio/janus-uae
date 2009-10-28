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

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#define IsCyberModeID(modeID) \
	 LP1(0x36, BOOL, IsCyberModeID, ULONG, modeID, d0, \
	 , CyberGfxBase)

extern struct IntuitionBase* IntuitionBase;

ULONG *mousebuffer=NULL;

struct MsgPort        *InputMP;
struct InputEvent     *FakeEvent;
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
    if(FakeEvent) {
      NeoPix=AllocVec(sizeof(struct IEPointerPixel), MEMF_PUBLIC);
      if(NeoPix) {
	InputIO=CreateIORequest(InputMP, sizeof(struct IOStdReq));
	if(InputIO) {
	  if(!OpenDevice("input.device", NULL, 
	      (struct IORequest *)InputIO, NULL)) {
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

/* SetMouse is based on SetMouse (Freeware)
 * (C) 1994,1995, Ketil Hunn
 */
void SetMouse(struct Screen *screen, WORD x, WORD y, UWORD button) {

  ENTER

  NeoPix->iepp_Screen=(struct Screen *)screen;
  NeoPix->iepp_Position.X=x;
  NeoPix->iepp_Position.Y=y;

  FakeEvent->ie_EventAddress=(APTR)NeoPix;
  FakeEvent->ie_NextEvent=NULL;
  FakeEvent->ie_Class=IECLASS_NEWPOINTERPOS;
  FakeEvent->ie_SubClass=IESUBCLASS_PIXEL;
  FakeEvent->ie_Code=0;
  FakeEvent->ie_Qualifier=NULL;

  InputIO->io_Data=(APTR)FakeEvent;
  InputIO->io_Length=sizeof(struct InputEvent);
  InputIO->io_Command=IND_WRITEEVENT;
  DoIO((struct IORequest *)InputIO);

  if(button!=IECODE_NOBUTTON) {
    /* BUTTON DOWN */
    FakeEvent->ie_EventAddress=NULL;
    FakeEvent->ie_Class=IECLASS_RAWMOUSE;
    FakeEvent->ie_Code=button;
    DoIO((struct IORequest *)InputIO);

    /* BUTTON UP */
    FakeEvent->ie_Code=button|IECODE_UP_PREFIX;
    DoIO((struct IORequest *)InputIO);
  }

  LEAVE
}

#if 0
#define PREFS_SIZE 102
/* test for winuae mouse sync results */
void foo(void) {
  struct Screen      *firstscreen;
  ULONG               modeID;
  struct Preferences *prefs;
  BYTE                xoffset, yoffset;
  UWORD              *bla;

  firstscreen=IntuitionBase->FirstScreen;
  if(!firstscreen) {
    printf("sync_mouse: screen==NULL?!\n");
    return;
  }

  modeID=GetVPModeID(&(firstscreen->ViewPort));
  if(modeID == INVALID_ID) {
    return;
  }

  prefs=(struct Preferences *) AllocVec(PREFS_SIZE+1, MEMF_CLEAR);

  prefs=GetPrefs(prefs, PREFS_SIZE);

  /*  move.w 100(a0),d4 ? */
  xoffset=prefs->ViewXOffset;
  yoffset=prefs->ViewYOffset;

  DebOut("winuae: xoffset: %d\n", xoffset);
  DebOut("winuae: yoffset: %d\n", yoffset);

  bla=prefs;
  DebOut("winuae: bla: %x\n", bla[100]);

/* for overscan: -52, -10 .. !*/
  DebOut("winuae: ViewPort->Dx: %d\n", firstscreen->ViewPort.DxOffset);
  DebOut("winuae: ViewPort->Dy: %d\n", firstscreen->ViewPort.DyOffset);


}
#endif

#if 0
  struct DimensionInfo  dimension_info;
  DisplayInfoHandle     display_info_handle;

    //modeID=screen->ViewPort.Modes;

    DebOut("modeID: %lx\n", modeID);
    x=x - screen->ViewPort.DxOffset; /* DxOffset is a negative value for overscan */
    y=y - screen->ViewPort.DyOffset; /* DxOffset is a negative value for overscan */
    DebOut("DOffsets: %d, %d\n",screen->ViewPort.DxOffset, screen->ViewPort.DyOffset);

    //display_info_handle=FindDisplayInfo(modeID);
    /* DTAG_DIMS: (DimensionInfo) - default dimensions and overscan info.*/

    result=GetDisplayInfoData(display_info_handle, (UBYTE *) &dimension_info, 
                          sizeof(struct DimensionInfo), 
                          DTAG_DIMS, modeID);
      /* if zero, no information for ID was available */


    if(result) {
      /* find out, how much bigger our screen is, compared to TextOverscan */
      overscan_x= ( screen->Width - 
                    ( dimension_info.TxtOScan.MaxX - 
		      dimension_info.TxtOScan.MinX + 1
		    ) 
		  ) /2;
      overscan_y= ( screen->Height - 
                    ( dimension_info.TxtOScan.MaxY - 
		      dimension_info.TxtOScan.MinY + 1
		    ) 
		  ) /2;
      DebOut("dimension_info screen: %d x %d  \n",screen->Width, screen->Height);
      DebOut("dimension_info.TxtOScan.MinX: %d\n",dimension_info.TxtOScan.MinX);
      DebOut("dimension_info.TxtOScan.MaxX: %d\n",dimension_info.TxtOScan.MaxX);
      DebOut("dimension_info.TxtOScan.MinY: %d\n",dimension_info.TxtOScan.MinY);
      DebOut("dimension_info.TxtOScan.MaxY: %d\n",dimension_info.TxtOScan.MaxY);
      DebOut("dimension_info overscan_x: %d\n",overscan_x);
      DebOut("dimension_info overscan_y: %d\n",overscan_y);
      DebOut("dimension_info\n");
    }
    else {
      DebOut("dimension_info: ERROR in getting one (result=0)!?\n");
    }

    x=x+overscan_x;
    y=y+overscan_x;
#endif
/**********************************************************
 * no_p96_overscan
 *
 * add overscan offsets to mouse x,y coordinates
 *
 * only for non Picasso 96 screens
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

void no_p96_fix_viewoffset(struct Screen *screen, WORD *x, WORD *y) {
  struct View          *view;

  DebOut("no_p96_fix_viewoffset(%d, %d)\n", *x, *y);

  view=ViewAddress();

  DebOut("no_96: screen->View.DxOffset: %d\n", view->DxOffset);
  DebOut("no_96: screen->View.DyOffset: %d\n", view->DyOffset);

#if 0
  *y=*y + view->DyOffset - 77;
#endif

  *x=*x - ((view->DxOffset -108)*2); /* 108? why, I don't know ;) */

  *y=*y + view->DyOffset - 77; /* TODO */


  DebOut("no_p96_fix_viewoffset: x,y: %d, %d\n",*x,*y);
}

/***************************************
 * we check the aros mouse coords and
 * place ours accordingly
 ***************************************/
void sync_mouse() {
  struct Screen        *screen;
  ULONG                 result;
  ULONG                 modeID;
  BOOL                  is_p96=FALSE;
  WORD                  x,y;
#if 0
  UWORD          flags;
#endif

  ENTER

  screen=IntuitionBase->FirstScreen;
  if(!screen) {
    printf("sync_mouse: screen==NULL?!\n");
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
  if(CyberGfxBase) {  /* no CyberGfxBase, is_p96 is always FALSE */
    DebOut("CyberGfxBase found\n");
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
    DebOut("CyberGfxBase *NOT* found\n");
  }

  mousebuffer[0]=is_p96;

  result = calltrap (AD_GET_JOB, AD_GET_JOB_GET_MOUSE, mousebuffer);

  if(!result) {
    LEAVE
    return;
  }

  x=(WORD) mousebuffer[0];
  y=(WORD) mousebuffer[1];

  DebOut("AD_GET_JOB_GET_MOUSE result: %d, %d\n",x,y);

  if(!is_p96) {
    DebOut("no_p96 =============================\n");

    no_p96_fix_viewoffset(screen, &x, &y);
    no_p96_fix_resolution(screen, &x, &y);
    no_p96_fix_overscan  (screen, &x, &y);

    DebOut("no_96: x,y: %d, %d (after div)\n",x,y);

  }

  if(screen->MouseX != x || screen->MouseY != y) {
    DebOut("set mouse to: %d, %d\n",x,y);
    SetMouse(screen, x, y, 0);
  }
  else {
    DebOut("mouse already at: %d, %d\n",x,y);
  }
    //printf("mouse x %d y %d\n",mousebuffer[0],mousebuffer[1]);

  LEAVE
}

