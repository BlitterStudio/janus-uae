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

#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/timer_protos.h>

#include "janus-daemon.h"

LONG *mousebuffer=NULL;

struct MsgPort        *InputMP;
struct InputEvent     *FakeEvent;
struct IEPointerPixel *NeoPix;
struct IOStdReq       *InputIO;

BOOL sync_mouse_device_open=FALSE;

void free_sync_mouse() {

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
}

BOOL init_sync_mouse() {

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
	     return TRUE;
	  }
	}
      }
    }
  }

  printf("ERROR: init_sync_mouse failed (%lx, %lx, %lx, %lx)\n",
	  InputMP, FakeEvent, NeoPix, InputIO);

  free_sync_mouse();

  return FALSE;
}

/* SetMouse is based on SetMouse (Freeware)
 * (C) 1994,1995, Ketil Hunn
 */
static void SetMouse(struct Screen *screen, 
                     WORD x, WORD y, UWORD button) {

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
}

/***************************************
 * we check the aros mouse coords and
 * place ours accordingly
 ***************************************/

void sync_mouse() {
  ULONG result;
  struct Screen *screen;

  screen=(struct Screen *) LockPubScreen(NULL);
  if(!screen) {
    printf("sync_mouse: screen==NULL?!\n");
    return;
  }
  UnlockPubScreen(NULL,screen); 

  if(!mousebuffer) {
    mousebuffer=AllocVec(16,MEMF_CLEAR);
  }

  result = calltrap (AD_GET_JOB, AD_GET_JOB_GET_MOUSE, mousebuffer);

  if(result) {
    if(screen->MouseX != mousebuffer[0] ||
       screen->MouseY != mousebuffer[1]) {
      SetMouse(screen, (WORD) mousebuffer[0], (WORD) mousebuffer[1],0);
    }
    //printf("mouse x %d y %d\n",mousebuffer[0],mousebuffer[1]);
  }
}

