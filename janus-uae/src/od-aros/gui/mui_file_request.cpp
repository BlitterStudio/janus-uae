#include <exec/types.h>
#include <libraries/mui.h>
 
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>
#include <libraries/asl.h>

#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "gui.h"
#include "winnt.h"
#include "mui_data.h"

/*
 * lpstrFilter ("*.TXT;*.DOC;*.BAK")
 * lpstrFile (file name used to initialize the File Name edit control)
 * lpstrFileTitle (The file name and extension (without path information) of the selected file.)
 * lpstrInitialDir (initial directory)
 * lpstrTitle (title bar string)
 */

BOOL mui_get_filename(TCHAR *lpstrTitle, TCHAR *lpstrInitialDir, TCHAR *lpstrFile, 
                      TCHAR *lpstrFilter, TCHAR *lpstrFileTitle, ULONG flags) {

  struct FileRequester *req;
  struct Window *win;
  BOOL ret;

  DebOut("lpstrTitle: %s\n", lpstrTitle);
  DebOut("lpstrInitialDir: %s\n", lpstrInitialDir);
  DebOut("lpstrFile: %s\n", lpstrFile);
  DebOut("lpstrFilter: %s\n", lpstrFilter);
  DebOut("lpstrFileTitle: %s\n", lpstrFileTitle);

  win=(struct Window *) xget(app, MUIA_Window_Window);

  req=(struct FileRequester *) MUI_AllocAslRequestTags(ASL_FileRequest,
                ASLFR_Window,win ,
                ASLFR_TitleText, lpstrTitle,
                ASLFR_InitialDrawer  , lpstrInitialDir,
                ASLFR_InitialFile, lpstrFile,
                ASLFR_InitialPattern , lpstrFilter,
                ASLFR_DoPatterns,TRUE,
/*                ASLFR_DoSaveMode     , save,
                  ASLFR_DoPatterns     , TRUE,*/
                ASLFR_RejectIcons    , TRUE,
                /*ASLFR_UserData       , widget,*/
                TAG_DONE);

  if(!req) {
    fprintf(stderr, "Unable to MUI_AllocAslRequestTags!\n");
    return FALSE;
  }

  ret=MUI_AslRequestTags(req,TAG_DONE);

  strncpy(lpstrFile, req->fr_Drawer, MAX_DPATH);
  AddPart(lpstrFile, req->fr_File, MAX_DPATH);
  DebOut("lpstrFile: %s\n", lpstrFile);

  strncpy(lpstrFileTitle, req->fr_File, MAX_DPATH);
  DebOut("lpstrFileTitle: %s\n", lpstrFile);

  MUI_FreeAslRequest(req);

  return FALSE;


}