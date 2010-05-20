/************************************************************************ 
 *
 * Amiga mouse interface
 *
 * Copyright 1996,1997,1998 Samuel Devulder.
 * Copyright 2003-2007 Richard Drummond
 * Copyright 2009-2010 Oliver Brunner - aros<at>oliver-brunner.de
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

# ifdef __amigaos4__
#  define __USE_BASETYPE__
# endif

#include <intuition/pointerclass.h>
#include <proto/layers.h>

/****************************************************************************/

#include "sysconfig.h"
#include "sysdeps.h"

#include "td-amigaos/thread.h"

#if defined(CATWEASEL)
#include "catweasel.h"
#endif

//#define JW_ENTER_ENABLED  1
#define JWTRACING_ENABLED 1
#include "od-amiga/j.h"

#include "uae.h"
#include "ami.h"

#ifdef __amigaos4__ 
static int mouseGrabbed;
static int grabTicks;
#define GRAB_TIMEOUT 50
#endif

/****************************************************************************/

POINTER_STATE pointer_state;

static APTR blank_pointer;
static POINTER_STATE get_pointer_state (const struct Window *w, int mousex, int mousey);

/****************************************************************************/

/*
 * Initializes a pointer object containing a blank pointer image.
 * Used for hiding the mouse pointer
 */
void init_pointer (void) {

    static struct BitMap bitmap;
    static UWORD	 row[2] = {0, 0};

    InitBitMap (&bitmap, 2, 16, 1);
    bitmap.Planes[0] = (PLANEPTR) &row[0];
    bitmap.Planes[1] = (PLANEPTR) &row[1];

    blank_pointer = NewObject (NULL, POINTERCLASS,
			       POINTERA_BitMap,	(ULONG)&bitmap,
			       POINTERA_WordWidth,	1,
			       TAG_DONE);

    if (!blank_pointer)
	write_log ("Warning: Unable to allocate blank mouse pointer.\n");
}

/*
 * Free up blank pointer object
 */
void free_pointer (void) {

    if (blank_pointer) {
	DisposeObject (blank_pointer);
	blank_pointer = NULL;
    }
}

/*
 * Hide mouse pointer for window
 */
void hide_pointer (struct Window *w) {

  if(!blank_pointer) {
    init_pointer();
  }

  if(blank_pointer) {
    pointer_state=INSIDE_WINDOW;
    SetWindowPointer (w, WA_Pointer, (ULONG)blank_pointer, TAG_DONE);
  }
}

/*
 * Restore default mouse pointer for window
 */
void show_pointer (struct Window *w) {

  pointer_state=OUTSIDE_WINDOW;
  SetWindowPointer (w, WA_Pointer, 0, TAG_DONE);
}

/* 
 * show/hide pointer according to the current position
 *
 * x,y are relative to windows topleft edge
 */
void update_pointer(struct Window *w, int x, int y) {

  POINTER_STATE new_state = get_pointer_state (W, x, y);

  if (new_state != pointer_state) {
  //  pointer_state = new_state;
    if (pointer_state == INSIDE_WINDOW)
      hide_pointer (W);
    else
      show_pointer (W);
    }
}


#ifdef __amigaos4__ 
/*
 * Grab mouse pointer under OS4.0. Needs to be called periodically
 * to maintain grabbed status.
 */
static void grab_pointer (struct Window *w) {

    struct IBox box = {
	W->BorderLeft,
	W->BorderTop,
	W->Width  - W->BorderLeft - W->BorderRight,
	W->Height - W->BorderTop  - W->BorderBottom
    };

    SetWindowAttrs (W, WA_MouseLimits, &box, sizeof box);
    SetWindowAttrs (W, WA_GrabFocus, mouseGrabbed ? GRAB_TIMEOUT : 0, sizeof (ULONG));
}
#endif

/****************************************************************************/

static POINTER_STATE get_pointer_state (const struct Window *w, int mousex, int mousey) {

    POINTER_STATE new_state = OUTSIDE_WINDOW;

    /*
     * Is pointer within the bounds of the inner window?
     */
    if ((mousex >= w->BorderLeft)
     && (mousey >= w->BorderTop)
     && (mousex < (w->Width - w->BorderRight))
     && (mousey < (w->Height - w->BorderBottom))) {
	/*
	 * Yes. Now check whetehr the window is obscured by
	 * another window at the pointer position
	 */
	struct Screen *scr = w->WScreen;
	struct Layer  *layer;

	/* Find which layer the pointer is in */
	LockLayerInfo (&scr->LayerInfo);
	layer = WhichLayer (&scr->LayerInfo, scr->MouseX, scr->MouseY);
	UnlockLayerInfo (&scr->LayerInfo);

	/* Is this layer our window's layer? */
	if (layer == w->WLayer) {
	    /*
	     * Yes. Therefore, pointer is inside the window.
	     */
	    new_state = INSIDE_WINDOW;
	}
    }
    return new_state;
}

