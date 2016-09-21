
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
#include "muigui.h"

#include "mui_dialog.h"
/*
 * Retrieves the identifier of the specified control. 
 */
int GetDlgCtrlID(HWND hwndCtl) {
  TODO();
  return 0;
}

static int return_value;

BOOL EndDialog(HWND hDlg, int nResult) {

  return_value=nResult;

  Signal(FindTask(NULL), SIGBREAKF_CTRL_C);

  return nResult;
}

Object* FixedProcObj(ULONG idc, IPTR proc);
extern Object *app;
extern Object *win;


static void aros_dialog_loop(void) {

  ULONG signals = 0;
  
  DebOut("entered..\n");
  
  while (DoMethod(app, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit) {
    signals = Wait(signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_F);
    if (signals & SIGBREAKF_CTRL_C) break;
    if (signals & SIGBREAKF_CTRL_D) break;
    if (signals & SIGBREAKF_CTRL_F) {
      DebOut("SIGBREAKF_CTRL_F received!\n");
    }
  }
  
  DebOut("left\n");
}


/* Creates a modal dialog box from a dialog box template in memory. */
/* return 1 on success, 0 for cancel */

/* original windows API: 
INT_PTR DialogBoxIndirect(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, INT_PTR (CALLBACK FAR *func) (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam))  {
*/
/* WARNING: contrary to Windows, we expect an idc in hInstance, not a HINSTANCE  !! */
INT_PTR DialogBoxIndirect(ULONG idc, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, INT_PTR (CALLBACK FAR *func) (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam))  {

  Object  *mui_win;
  Object  *mui_content;
  Element *e;

  DebOut("lpTemplate: 0x%p\n", lpTemplate); /* don't use! */
  DebOut("lpDialogFunc: 0x%p\n", func);
  DebOut("idc: %d\n", idc);

  e=get_control(NULL, idc);
  DebOut("e: 0x%p\n", e);

  mui_content=FixedProcObj(idc, (IPTR)func);
  DebOut("content: 0x%p\n", mui_content);
  if(!mui_content) {
    DebOut("ERROR: could create window content!?\n");
    return 0;
  }

  mui_win=MUI_NewObject(MUIC_Window,
                    MUIA_Window_Title, (IPTR) "Properties", /* TODO: CAPTION of DIALOGEX */
                    MUIA_Window_RootObject, 
                      mui_content
                    );

  if(!mui_win) {
    DebOut("ERROR: could not open window!?\n");
    return 0;
  }

  return_value=0;

  DebOut("OM_ADDMEMBER..\n");
  DoMethod(app, OM_ADDMEMBER, (IPTR) mui_win);
  SetAttrs(win, MUIA_Window_Sleep, TRUE, TAG_DONE);

  func (e, WM_INITDIALOG, 0, 0);

  SetAttrs(mui_win, MUIA_Window_Open, TRUE, TAG_DONE);

  DebOut("start aros_dialog_loop\n");
  aros_dialog_loop();

  DebOut("shut down dialog..\n");

  SetAttrs(mui_win, MUIA_Window_Open, FALSE, TAG_DONE);
  DoMethod(app, OM_REMMEMBER, (IPTR) mui_win);
  DisposeObject(mui_win);
  mui_win=NULL;
  SetAttrs(win, MUIA_Window_Sleep, FALSE, TAG_DONE);

  DebOut("DialogBoxIndirect closed! return %d\n", return_value);

  return return_value;
}

struct newresource *scaleresource (struct newresource *res, HWND parent, int resize, int fullscreen, DWORD exstyle) {

  DebOut("res: 0x%p\n", res);

  struct newresource *ns;
  ns = xcalloc (struct newresource, 1);
  ns->inst = res->inst;
  ns->size = res->size;
  ns->tmpl = res->tmpl;
  ns->resource = (LPCDLGTEMPLATEW)xmalloc (uae_u8, ns->size + 32);
  memcpy ((void*)ns->resource, res->resource, ns->size);

  return ns;
}

void freescaleresource (struct newresource *ns) {

  DebOut("ns: 0x%p\n", ns);

  xfree ((void*)ns->resource);
  xfree (ns);
}

