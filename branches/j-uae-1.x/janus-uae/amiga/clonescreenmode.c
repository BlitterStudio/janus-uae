 /******************************************************************
  * Copyright © 1995-2012, The AROS Development Team. 
  * All rights reserved.
  *******************************************************************
  * clonescreenmode
  *
  * This file is part of janus-uae. It will *not* work on real
  * Amiga hardware or any other emulator than janus-uae!
  *
  * Parts of this file have been taken from AROS Prefs/screenmode,
  * so this file is APL, too.
  *
  *******************************************************************
  *
  * $Id$
  *
  */

#define DEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <intuition/preferences.h>
#include <prefs/screenmode.h>
#include <prefs/prefhdr.h>
#include <graphics/modeid.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include "janus-daemon.h"
#include "prefs.h"

#define GET_WORD(x) x
#define GET_LONG(x) x

/*
 * d0 is the function to be called (AD_*)
 * d1 is the size of the memory supplied by a0
 * a0 memory, where to put out command in and 
 *    where we store the result
 */
ULONG (*calltrap)(ULONG __asm("d0"),
                  ULONG __asm("d1"),
		  APTR  __asm("a0")) = (APTR) AROSTRAPBASE;

/*********************************************************************************************/

#define PREFS_PATH_ENVARC "ENVARC:SYS/screenmode.prefs"
#define PREFS_PATH_ENV    "ENV:SYS/screenmode.prefs"

/*********************************************************************************************/

struct Library       *IFFParseBase =NULL;
struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase       *GfxBase      =NULL;

struct ScreenModePrefs screenmodeprefs;

/*********************************************************************************************/

static BOOL Prefs_Load(STRPTR from)
{
    BOOL retval = FALSE;

    BPTR fh = Open(from, MODE_OLDFILE);
    if (fh)
    {
        retval = Prefs_ImportFH(fh);
        Close(fh);
    }

    return retval;
}

/*********************************************************************************************/

BOOL Prefs_ImportFH(BPTR fh)
{
    struct ContextNode     *context;
    struct IFFHandle       *handle;
    struct ScreenModePrefs  loadprefs = {{0},0};
    BOOL                    success = TRUE;
    LONG                    error;

    if (!(handle = AllocIFF()))
        return FALSE;

    handle->iff_Stream = fh;
    InitIFFasDOS(handle);

    if ((error = OpenIFF(handle, IFFF_READ)) == 0)
    {
        // FIXME: We want some sanity checking here!
        if ((error = StopChunk(handle, ID_PREF, ID_SCRM)) == 0)
        {
            if ((error = ParseIFF(handle, IFFPARSE_SCAN)) == 0)
            {
                context = CurrentChunk(handle);

                error = ReadChunkBytes
                (
                    handle, &loadprefs, sizeof(struct ScreenModePrefs)
                );

                if (error < 0)
                {
                    fprintf(stderr, "error: ReadChunkBytes() returned %ld!\n", error);
                }
                else
                {
		    CopyMemQuick(loadprefs.smp_Reserved, screenmodeprefs.smp_Reserved, sizeof(screenmodeprefs.smp_Reserved));
		    screenmodeprefs.smp_DisplayID = GET_LONG(loadprefs.smp_DisplayID);
		    screenmodeprefs.smp_Width     = GET_WORD(loadprefs.smp_Width);
		    screenmodeprefs.smp_Height    = GET_WORD(loadprefs.smp_Height);
		    screenmodeprefs.smp_Depth     = GET_WORD(loadprefs.smp_Depth);
		    screenmodeprefs.smp_Control   = loadprefs.smp_Control;
                }
            }
            else
            {
                fprintf(stderr, "error: ParseIFF() failed, returncode %ld!\n", error);
                success = FALSE;
            }
        }
        else
        {
            fprintf(stderr, "error: StopChunk() failed, returncode %ld!\n", error);
            success = FALSE;
        }

        CloseIFF(handle);
    }
    else
    {
        //ShowError(_(MSG_CANT_OPEN_STREAM));
    }

    FreeIFF(handle);

    return success;
}

/*********************************************************************************************/

BOOL Prefs_Write(BPTR fh, WORD width, WORD height, WORD depth)
{
    struct PrefHeader       header = { 0 };
    struct IFFHandle       *handle;
    struct ScreenModePrefs  saveprefs;
    BOOL                    success = TRUE;
    LONG                    error   = 0;
    ULONG id;

    id=BestModeID(BIDTAG_DesiredWidth, width,
                   BIDTAG_DesiredHeight, height);

    if(id == INVALID_ID) {
      fprintf(stderr, "error: Resolution %dx%d not supported in the 68k environment", width, height);
      return FALSE;
    }

    CopyMemQuick(screenmodeprefs.smp_Reserved, saveprefs.smp_Reserved, sizeof(screenmodeprefs.smp_Reserved));
    saveprefs.smp_DisplayID =id; 
    saveprefs.smp_Width     = width;
    saveprefs.smp_Height    = height;
    saveprefs.smp_Depth     = GET_WORD(screenmodeprefs.smp_Depth);
    saveprefs.smp_Control   = screenmodeprefs.smp_Control;

    if ((handle = AllocIFF()))
    {
        handle->iff_Stream = fh;

        InitIFFasDOS(handle);

        if (!(error = OpenIFF(handle, IFFF_WRITE))) /* NULL = successful! */
        {
            PushChunk(handle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */

            header.ph_Version = 0 /*PHV_CURRENT*/;
            header.ph_Type    = 0;

            PushChunk(handle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */

            WriteChunkBytes(handle, &header, sizeof(struct PrefHeader));

            PopChunk(handle);

            error = PushChunk(handle, ID_PREF, ID_SCRM, sizeof(struct ScreenModePrefs));

            if (error != 0) // TODO: We need some error checking here!
            {
                fprintf(stderr, "error: PushChunk() = %ld ", error);
            }

            error = WriteChunkBytes(handle, &saveprefs, sizeof(struct ScreenModePrefs));
            error = PopChunk(handle);

            if (error != 0) // TODO: We need some error checking here!
            {
                fprintf(stderr, "error: PopChunk() = %ld ", error);
            }

            // Terminate the FORM
            PopChunk(handle);
        }
        else
        {
            //ShowError(_(MSG_CANT_OPEN_STREAM));
            success = FALSE;
        }

        CloseIFF(handle);
        FreeIFF(handle);
    }
    else // AllocIFF()
    {
        // Do something more here - if IFF allocation has failed, something isn't right
        //ShowError(_(MSG_CANT_ALLOCATE_IFFPTR));
        success = FALSE;
    }

    return success;
}

void usage(char *myname) {
  printf("Usage: %s [ENV|ENVARC|PRETEND]\n", myname);
  printf("       Clone host screen resolution to 68k ENV\n");
  printf("       screenmode prefs file.\n");
  printf("\n");
  printf(" ENV:\n");
  printf("  save AROS/x86 host default public screen\n");
  printf("  settings to %s\n", PREFS_PATH_ENV);
  printf(" ENVARC:\n");
  printf("  save AROS/x86 host default public screen\n");
  printf("  settings to %s\n", PREFS_PATH_ENVARC);
  printf(" PRETEND:\n");
  printf("  just print resolution and depth to STDOUT\n");
}

int main(int argc, char **argv) {
  BPTR fh           =NULL;
  ULONG *command_mem=NULL;
  ULONG err         =1;
  char prefsname[256];

  /*
   * valid parameters are:
   *
   * ENVARC  - save to PREFS_PATH_ENVARC "ENVARC:SYS/screenmode.prefs"
   * ENV     - save to PREFS_PATH_ENV    "ENV:SYS/screenmode.prefs"
   * PRETEND - just print resolution
   *
   */

  if(argc>2) {
    usage(argv[0]);
    goto EXIT;
  }

  if(argc==2) {
    if(!stricmp(argv[1], "ENV")) {
      strcpy(prefsname, PREFS_PATH_ENV);
    }
    else if((!stricmp(argv[1], "ENVARC"))) {
      strcpy(prefsname, PREFS_PATH_ENVARC);
    }
    else if((!stricmp(argv[1], "PRETEND"))) {
      prefsname[0]=(char) 0;
    }
    else {
      usage(argv[0]);
      goto EXIT;
    }
  }
  else {
    /* default */
    strcpy(prefsname, PREFS_PATH_ENV);
  }

  if (!(GfxBase = (struct GfxBase *) OpenLibrary ("graphics.library", 0L))) {
    fprintf(stderr, "error: Unable to open graphics.library!\n");
    goto EXIT;
  }

  if (!(IntuitionBase = (struct IntuitionBase *) OpenLibrary ("intuition.library", 0L))) {
    fprintf(stderr, "error: Unable to open intuition.library!\n");
    goto EXIT;
  }

  if (!(IFFParseBase = OpenLibrary ("iffparse.library", 0L))) {
    fprintf(stderr, "error: Unable to open iffparse.library!\n");
    goto EXIT;
  }

  command_mem=AllocVec(AD__MAXMEM, MEMF_CLEAR);
  if(!command_mem) {
    fprintf(stderr, "error: Unable to allocate memory!\n");
    goto EXIT;
  }

  calltrap (AD_GET_JOB, AD_GET_JOB_HOST_DATA, command_mem);

  if(command_mem[0] == (ULONG) -1) {
    fprintf(stderr, "error: Could not get host screen resolution!\n");
    goto EXIT;
  }

  if(!prefsname[0]) {
    /* pretend */
    printf("width %ld, height %ld, depth %ld\n", command_mem[0], command_mem[1], command_mem[2]);
    err=0;
    goto EXIT;
  }

  if(!Prefs_Load(PREFS_PATH_ENVARC)) {
    fprintf(stderr, "error: Unable to load old prefs file.\n");
    goto EXIT;
  }

  fh = Open(prefsname, MODE_NEWFILE);

  if(fh) {
    if( Prefs_Write(fh, command_mem[0], command_mem[1], command_mem[2]) ) {
      err=0;
    }
  }
  else {
    fprintf(stderr, "error: Unable to write to %s\n", prefsname);
  }

EXIT:

  if(fh) {
    Close(fh);
  }

  if(command_mem) {
    FreeVec(command_mem);
  }

  if(IFFParseBase) {
    CloseLibrary(IFFParseBase);
  }

  if(IntuitionBase) {
    CloseLibrary((struct Library *) IntuitionBase);
  }

  if(GfxBase) {
    CloseLibrary((struct Library *) GfxBase);
  }

  exit(err);

}
