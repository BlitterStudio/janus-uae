
#include <exec/types.h>
#include <libraries/mui.h>
 
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

#include <graphics/gfxbase.h>

#define JUAE_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "aros.h"
#include "gui.h"
#include "winnt.h"
#include "mui_data.h"
#include "gui_mui.h"

#include "mui_class.h"

#include "registry.h"
#include "win32gui.h"

#include "mui_dialog.h"
/*
 * Retrieves the identifier of the specified control. 
 */
int GetDlgCtrlID(HWND hwndCtl) {
  TODO();
  return 0;
}

BOOL EndDialog(HWND hDlg, int nResult) {
  TODO();
  return FALSE;
}

/*
 * Translates the text of a specified control in a dialog box into an integer value.
 */
UINT GetDlgItemInt(HWND hDlg, int  nIDDlgItem, BOOL *lpTranslated,  BOOL bSigned) {
  TODO();
  return 0;
}

Object* FixedProcObj(IPTR src, IPTR proc);
Object* FixedObj(IPTR src);
extern Object *app;
extern Object *win;


INT_PTR DialogBoxIndirect(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, INT_PTR (CALLBACK FAR *func) (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam))  {

  Object *mui_win;
  Object *mui_root;
  Object *mui_content;

  DebOut("lpTemplate: %lx\n", lpTemplate);
  DebOut("lpDialogFunc: %lx\n", func);

  mui_content=FixedObj((IPTR) lpTemplate);
  DebOut("content: %lx\n", mui_content);
  if(!mui_content) {
    DebOut("ERROR: could create window content!?\n");
    return 0;
  }

  mui_win=MUI_NewObject(MUIC_Window,
                    MUIA_Window_Title, (IPTR) "Properties", /* CAPTION of DIALOGEX is not available.. */
                    MUIA_Window_RootObject, 
                      mui_content
                    );

  if(!mui_win) {
    DebOut("ERROR: could not open window!?\n");
    return 0;
  }

  //SetAttrs(win, MUIA_Window_Sleep, TRUE, TAG_DONE);
  DoMethod(app, OM_ADDMEMBER, (IPTR) mui_win);

  func ((Element *)lpTemplate, WM_INITDIALOG, 0, 0);
  SetAttrs(mui_win, MUIA_Window_Open, TRUE, TAG_DONE);

  return 1;
}

struct newresource *scaleresource (struct newresource *res, HWND parent, int resize, DWORD exstyle) {

  DebOut("res: %lx\n", res);

  res->resource=(LPCDLGTEMPLATEW) res->tmpl;

  return res;
}

void freescaleresource (struct newresource *ns) {
  TODO();
}

