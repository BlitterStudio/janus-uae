#include <exec/types.h>
#include <libraries/mui.h>
 
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/reqtools.h>
#include <proto/diskfont.h>
#include <clib/alib_protos.h>


#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "aros.h"
#include "gui.h"
#include "winnt.h"
#include "mui_data.h"

extern Object *win;

/*
 * Open a requester with a fixed font.
 *
 * Only reqtools.library seems to have the ability to use custom fonts.
 * This crashes, if I don't open requtools.library manually. Is this no auto-open library !?
 *
 * Is there really so much code necessary, just to open a fixed width requester !?
 *
 * This is no WinAPI call.
 */
int MessageBox_fixed(HWND hWnd, TCHAR *lpText, TCHAR *lpCaption, UINT uType/*, UINT size*/) {
  rtReqInfo *req;
  ULONG ret;
  BOOL i_opened_it=false;
  struct TextFont *font;
  struct Window *window;
  struct TextAttr myta = {
      "fixed.font",
      8,
      0,
      NULL
  };

  struct TagItem tags[]={
    {RT_TextAttr, (IPTR) &myta},
    {RT_Window, NULL},
    {RTEZ_ReqTitle , (IPTR) lpCaption},
    {RT_ReqPos, REQPOS_CENTERSCR},
    {RT_WaitPointer, TRUE},
    {RT_LockWindow, TRUE},
    {TAG_DONE}
  };

  DebOut("Caption: %s\n", lpCaption);
  DebOut("Text: %s\n", lpText);

  font=OpenDiskFont(&myta);
  if(!font) {
    DebOut("unable to open fixed.font\n");
    tags[0].ti_Tag=TAG_IGNORE;
  }

  window=(struct Window *)XGET(win, MUIA_Window_Window);
  if(window) {
    DebOut("window: %lx\n", window);
    tags[1].ti_Data=(IPTR) window;
  }
  else {
    DebOut("ERROR: no window !?\n");
    tags[1].ti_Tag=TAG_IGNORE;
  }

  /* do I really have to open it !? */
  if(!ReqToolsBase) {
    i_opened_it=TRUE;
    ReqToolsBase = (struct ReqToolsBase *)OpenLibrary("reqtools.library", 0);
  }

  if(!ReqToolsBase) {
    fprintf(stderr, "ERROR: Unable to open reqtools.library!\n");
    DebOut("ERROR: Unable to open reqtools.library!\n");
    return FALSE;
  }

  ret=rtEZRequestA(lpText, "Ok", NULL, NULL, tags);

  if(i_opened_it) {
    CloseLibrary((struct Library *)ReqToolsBase);
    ReqToolsBase=NULL;
  }

  if(font) {
    CloseFont(font);
  }
  return TRUE;
}

int MessageBox(HWND hWnd, TCHAR *lpText, TCHAR *lpCaption, UINT uType) {

  struct EasyStruct req;
  LONG res;

  DebOut("MessageBox(): %s \n", lpText);

  if(!uType & MB_OK) {
    DebOut("WARNING: unsupported type: %lx\n", uType);
  }

  req.es_StructSize=sizeof(struct EasyStruct);
  req.es_Flags = 0;
  req.es_TextFormat=lpText;

  if(lpCaption) {
    req.es_Title=lpCaption;
  }
  else {
    req.es_Title="Error";
  }

  if(uType & MB_YESNO) {
    req.es_GadgetFormat="Yes|No";
  }
  else {
    /* MB_OK 0x00000000L is the default */
    req.es_GadgetFormat="Ok";
  }

  res=EasyRequestArgs(0, &req, 0, 0);

  if(uType & MB_YESNO) {
    if(res==1) {
      return IDYES;
    }
    return IDNO;
  }

  return IDOK;
}
