/************************************************************************ 
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE.
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
 * along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
 *
 * This file is based on a parts of cbio.c, (c) 1992 Commodore-Amiga.
 *
 * $Id$
 *
 ************************************************************************/

#include "sysconfig.h"
#include "sysdeps.h"

#include <stdlib.h>
#include <stdarg.h>
#include <devices/clipboard.h>
#include <exec/devices.h>
#include <exec/io.h>

#include <clib/alib_protos.h>

#include "j-clipboard.h"

#include "threaddep/thread.h"
#include "memory.h"
#if 0
#include "native2amiga.h"
#else
void uae_Signal(uaecptr task, uae_u32 mask);
#endif
#include "j.h"

#define DEBOUT_ENABLED 1
#if DEBOUT_ENABLED
#define DebOut(...) do { kprintf("%s: %s():%d: ",__FILE__,__func__,__LINE__);kprintf(__VA_ARGS__); } while(0)
#else
#define DebOut(...) do { ; } while(0)
#endif

static struct IOClipReq *ior=NULL;

/****** cbio/CBOpen *************************************************
*
*   NAME
*       CBOpen() -- Open the clipboard.device
*
*   SYNOPSIS
*       ior = CBOpen(unit)
*
*       struct IOClipReq *CBOpen( ULONG )
*
*   FUNCTION
*       Opens the clipboard.device.  A clipboard unit number
*       must be passed in as an argument.  By default, the unit
*       number should be 0 (currently valid unit numbers are
*       0-255).
*
*   RESULTS
*       A pointer to an initialized IOClipReq structure, or
*       a NULL pointer if the function fails.
*
*********************************************************************/
struct IOClipReq *CBOpen(ULONG unit) {
  struct MsgPort *mp;
  struct IORequest *ior;

  if ((mp = CreatePort(0L,0L))) {
    if ((ior=(struct IORequest *)CreateExtIO(mp,sizeof(struct IOClipReq)))) {
      if (!(OpenDevice("clipboard.device", unit, ior, 0L))) {
	return((struct IOClipReq *)ior);
      }
      DeleteExtIO(ior);
    }
  DeletePort(mp);
  }

  DebOut("ERROR opening clipboard.device\n");
  return(NULL);
}

/****** cbio/CBClose ************************************************
*
*   NAME
*       CBClose() -- Close the clipboard.device
*
*   SYNOPSIS
*       CBClose()
*
*       void CBClose()
*
*   FUNCTION
*       Close the clipboard.device unit which was opened via
*       CBOpen().
*
*********************************************************************/
static void CBClose(struct IOClipReq *ior) {
  struct MsgPort *mp;

  if(!ior) {
    return;
  }

  mp = ior->io_Message.mn_ReplyPort;

  CloseDevice((struct IORequest *)ior);
  DeleteExtIO((struct IORequest *)ior);
  DeletePort(mp);
}

/*********************************************************************
 * clipboard_write_raw
 *
 * write data to clipboard. data must already be valid IFF data.
 *********************************************************************/
static BOOL clipboard_write_raw(struct IOClipReq *ior, UBYTE *data, ULONG len) {
  BOOL success;

  DebOut("clipboard_write_raw(%lx, %lx, %d)\n", ior, data, len);

  /* initial set-up for Offset, Error, and ClipID */
  ior->io_Offset = 0;
  ior->io_Error  = 0;
  ior->io_ClipID = 0;

  DebOut("write data ..\n");
  /* Write data */
  ior->io_Data    = data;
  ior->io_Length  = len;
  ior->io_Command = CMD_WRITE;
  DoIO( (struct IORequest *) ior);

  DebOut("update clipboard ..\n");
  /* Tell the clipboard we are done writing */
  ior->io_Command=CMD_UPDATE;
  DoIO( (struct IORequest *) ior);

  /* Check if io_Error was set by any of the preceding IO requests */
  success = ior->io_Error ? FALSE : TRUE;

  if(!success) {
    DebOut("ERROR: %d\n",ior->io_Error);
  }

  return(success);
}




/*******************************************************
 * amiga_clipboard_from_iff_text
 *
 * return a pointer to the text or NULL
 * if(!NULL) the caller has to free the returned
 * memory with free();
 *******************************************************/
static char *amiga_clipboard_from_iff_text (uaecptr ftxt, uae_u32 len) {

    uae_u8 *addr = NULL, *eaddr;
    char *txt = NULL;
    int txtsize = 0;

    addr = get_real_address (ftxt);

    eaddr = addr + len;

    addr += 12; /* skip header */

    while (addr < eaddr) {
      uae_u32 csize = (addr[4] << 24) | (addr[5] << 16) | (addr[6] << 8) | (addr[7] << 0);

      if (addr + 8 + csize > eaddr) {
	break;
      }

      if (!memcmp (addr, "CHRS", 4) && csize) {
	  int prevsize = txtsize;
	  txtsize += csize;
	  txt = realloc (txt, txtsize + 1);
	  memcpy (txt + prevsize, addr + 8, csize);
	  txt[txtsize] = 0;
      }

      addr += 8 + csize + (csize & 1);

      if (csize >= 1 && addr[-2] == 0x0d && addr[-1] == 0x0a && addr[0] == 0) {
	  addr++;
      }
      else if (csize >= 1 && addr[-1] == 0x0d && addr[0] == 0x0a) {
	  addr++;
      }
    }

    if (txt == NULL) {
      txt=malloc(1);
      txt[0]=(char) NULL;
    }

    DebOut("return >%s<\n",txt);

    /* amigatopc(txt) ?? */

    return txt;
}

/*******************************************************
 * amiga_clipboard_get_txt
 *
 * return a pointer to the text or NULL
 * the caller has to free the memory with free.
 *******************************************************/
static char *amiga_clipboard_get_txt (uaecptr data, uae_u32 len) {

  uae_u8 *addr;

  DebOut("entered (data: %lx, len %d)\n", data, len);

  if (len < 18) {
    DebOut("len too small! (<18)\n");
    return NULL;
  }

  if (!valid_address (data, len)) {
    DebOut("invalid_address!!\n");
    return NULL;
  }

  addr = get_real_address (data);

  if (memcmp ("FORM", addr, 4)) {
    DebOut("no FORM header at %lx (%s)\n", addr, addr);
    return NULL;
  }

  if (!memcmp ("FTXT", addr + 8, 4)) {
    return amiga_clipboard_from_iff_text (data, len);
  }
  DebOut("no FTXT header at %lx\n", addr+8);

  return NULL;
      /*
  if (!memcmp ("ILBM", addr + 8, 4))
      from_iff_ilbm (data, len);
      */
}


/*******************************************************
 * copy_clipboard_to_aros();
 *******************************************************/
void copy_clipboard_to_aros(void) {

  DebOut("entered\n");

  if(!clipboard_amiga_changed) {
    /* nothing to do for us */
    DebOut("nothing to do\n");
    return; 
  }

  if(!aos3_clip_task || !aos3_clip_signal) {
    DebOut("no clipd running\n");
  }

  /* 
   * just signal clipd and let him handle all the trouble 
   * in the end, he will call copy_clipboard_to_aros_real
   */ 

  DebOut("send signal %d to clipd task %lx\n", aos3_clip_signal, aos3_clip_task);
  uae_Signal (aos3_clip_task, aos3_clip_signal);

}

void copy_clipboard_to_aros_real(uaecptr data, uae_u32 len) {
  char *content;

  DebOut("entered (data %lx, len %d)\n", data, len);

  if (len < 18) {
    DebOut("len too small! (<18)\n");
    return;
  }

  if (!valid_address (data, len)) {
    DebOut("invalid_address!!\n");
    return;
  }

  if(!ior) {
    ior=CBOpen(0);
  }

  clipboard_write_raw(ior, get_real_address(data), len);

  CBClose(ior);
  ior=NULL;

  /* we are in sync now */
  clipboard_aros_changed =FALSE;
  clipboard_amiga_changed=FALSE;
}


