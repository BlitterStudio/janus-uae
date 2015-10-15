/************************************************************************
 *
 * mui_file_request.cpp
 *
 * Copyright 2015 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE2.
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
 ************************************************************************/
#include <exec/types.h>
#include <libraries/mui.h>
 
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>
#include <libraries/asl.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "aros.h"
#include "gui.h"
#include "mui_data.h"

/*
 * lpstrFilter ("*.TXT;*.DOC;*.BAK")
 * lpstrFile (file name used to initialize the File Name edit control)
 * lpstrFileTitle (The file name and extension (without path information) of the selected file.)
 * lpstrInitialDir (initial directory)
 * lpstrTitle (title bar string)
 * lpstrDefExt (default extension, must be added, if selected file contains no extension)
 */
BOOL mui_get_filename(TCHAR *lpstrTitle, TCHAR *lpstrInitialDir, TCHAR *lpstrFile, 
                      TCHAR *lpstrFilter, TCHAR *lpstrFileTitle, ULONG flags, TCHAR *lpstrDefExt) {

  struct FileRequester *req;
  struct Window *win;
  BOOL ret;
  char filter[256];
  char *b, *e;
  unsigned int i=0;

  DebOut("lpstrTitle:      %s\n", lpstrTitle);
  DebOut("lpstrInitialDir: %s\n", lpstrInitialDir);
  DebOut("lpstrFile:       %s\n", lpstrFile);
  DebOut("lpstrFilter:     %s\n", lpstrFilter);
  DebOut("lpstrDefExt:     %s\n", lpstrDefExt);

  win=(struct Window *) XGET(app, MUIA_Window_Window);
  DebOut("win: %lx\n", win);
  GetAttr(MUIA_Window_Window, app, (IPTR *)&win);
  DebOut("win: %lx\n", win);
  /* TODO: win is always NULL!? */

  /* convert Windows filter like 
   *   "Hard disk image files (*.hdf;*.vhd;*.rdf;*.hdz;*.rdz;*.chd)"
   * to AROS patterns like 
   *   "#?.hdf|#?.vhd|#?.rdf|#?.hdz|#?.rdz|#?.ch"
   */
  if(b=strchr(lpstrFilter, (int) '(')) {
    /* windows filter */
    if(e=strchr(lpstrFilter, (int) ')')) {
      DebOut("found Windows pattern:  %s\n", lpstrFilter);
      b++;
      e--;
      while(b < e) {
        if(b[0]=='*') {
          filter[i++]='#';
          filter[i++]='?';
        }
        else if(b[0]==';') {
          filter[i++]='|';
        }
        else {
          filter[i++]=b[0];
        }
        b++;
      }
    }
    filter[i++]=(char) 0;
    DebOut("converted AROS pattern: %s\n", filter);
  }
  else {
    strncpy(filter, lpstrFilter, 255);
    filter[255]=(char) 0;
  }

  req=(struct FileRequester *) MUI_AllocAslRequestTags(ASL_FileRequest,
                ASLFR_Window,          win,
                ASLFR_TitleText,       lpstrTitle,
                ASLFR_InitialDrawer,   lpstrInitialDir,
                ASLFR_InitialFile,     lpstrFile,
                ASLFR_InitialPattern,  filter,
                ASLFR_DoPatterns,      TRUE,
/*                ASLFR_DoSaveMode     , save,*/
                ASLFR_RejectIcons    , TRUE,
                TAG_DONE);

  if(!req) {
    fprintf(stderr, "Unable to MUI_AllocAslRequestTags!\n");
    return FALSE;
  }

  ret=MUI_AslRequestTags(req,TAG_DONE);

  if(ret) {
    strncpy(lpstrFile, req->fr_Drawer, MAX_DPATH);
    AddPart(lpstrFile, req->fr_File, MAX_DPATH);
    DebOut("lpstrFile: %s\n", lpstrFile);

    strncpy(lpstrFileTitle, req->fr_File, MAX_DPATH);
    DebOut("lpstrFileTitle: %s\n", lpstrFile);
    if(lpstrDefExt && !strchr(lpstrFile, '.')) {
      /* add .lpstrDefExt */
      strncat(lpstrFile, ".", MAX_DPATH);
      strncat(lpstrFile, lpstrDefExt, MAX_DPATH);
    }
    DebOut("lpstrFileTitle: %s\n", lpstrFile);
  }
  else {
    DebOut("Canceled!\n");
  }

  MUI_FreeAslRequest(req);

  return ret;
}

BOOL mui_get_dir(TCHAR *lpstrTitle, TCHAR *path) {

  struct FileRequester *req;
  struct Window *win;
  BOOL ret;

  DebOut("lpstrTitle: %s\n", lpstrTitle);
  if(path==NULL) {
    DebOut("ERROR: NULL path detected!!\n");
    return FALSE;
  }

  if(path[0]=(char) 0) {
    strcpy(path, "PROGDIR:");
  }
  DebOut("path: %s\n", path);

  win=(struct Window *) XGET(app, MUIA_Window_Window);

  req=(struct FileRequester *) MUI_AllocAslRequestTags(ASL_FileRequest,
                ASLFR_Window,win ,
                //ASLFR_TitleText, lpstrTitle,
                ASLFR_InitialDrawer, path,
                ASLFR_RejectIcons, TRUE,
                ASLFR_DrawersOnly, TRUE,
                TAG_DONE);

  if(!req) {
    fprintf(stderr, "Unable to MUI_AllocAslRequestTags!\n");
    return FALSE;
  }

  ret=MUI_AslRequestTags(req,TAG_DONE);

  if(ret) {
    strncpy(path, req->fr_Drawer, MAX_DPATH);
  }
  else {
    DebOut("Canceled!\n");
  }

  MUI_FreeAslRequest(req);

  return ret;
}
