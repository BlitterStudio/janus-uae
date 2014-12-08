#include <exec/types.h>
#include <libraries/mui.h>
 
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/diskfont.h>
#include <proto/graphics.h>
#include <clib/alib_protos.h>

#include <graphics/gfxbase.h>
#include <mui/Rawimage_mcc.h>
#include <proto/reqtools.h>

#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "aros.h"
#include "gui.h"
#include "winnt.h"
#include "mui_data.h"

//#define RESIZE 155 / 100
#define RESIZE_X 150 /100
#define RESIZE_Y 140 /100

#define MAX_ENTRIES 255

#define MA_Data 0xfece0000

struct Data {
  struct Hook LayoutHook;
  struct Hook MyMUIHook_control;
  struct Hook MyMUIHook_select;
  struct Hook MyMUIHook_combo;
  ULONG width, height;
  struct Element *src;
  struct TextFont *font;
  int *(*func) (Element *hDlg, UINT msg, ULONG wParam, ULONG lParam);
};

static const char *Cycle_Dummy[] = { NULL };

struct ReqToolsBase *ReqToolsBase=NULL;
struct MUI_CustomClass *CL_Fixed;

static VOID mSet(struct Data *data, APTR obj, struct opSet *msg, ULONG is_new);

extern Object *win;

ULONG xget(Object *obj, ULONG attr) {
  ULONG b = 0;
  GetAttr(attr, obj, (IPTR *)&b);
  return b;
}

/* return index in elem[] for item IDC_* */
static LONG get_index(Element *elem, int item) {
  int i=0;

  if(!elem) {
    return -1;
  }

  while(elem[i].exists) {
    if(elem[i].idc == item) {
      return i;
    }
    i++;
  }

  DebOut("item %d (0x%lx) not found in element %lx!!\n", item, item, elem);
  return -1;
}

/* return elem of item IDC_* */
Element *get_elem(int nIDDlgItem) {
  ULONG i=0;
  int res;

  while(IDD_ELEMENT[i]!=NULL) {
    res=get_index(IDD_ELEMENT[i], nIDDlgItem);
    if(res!=-1) {
      DebOut("found %d in Element number %d\n", nIDDlgItem, i);
      return IDD_ELEMENT[i];
    }
    i++;
  }
  
  return NULL;
}

static BOOL flag_editable(ULONG flags) {

  if((flags & CBS_DROPDOWNLIST)==CBS_DROPDOWNLIST) {
    return FALSE;
  }
  return TRUE;
}

/*************************************
 * get index of Zune object
 *************************************/
static int get_elem_from_obj(struct Data *data, Object *obj) {

  int i=0;

  DebOut("obj: %lx\n", obj);
  DebOut("data: %lx\n", data);

  /* first try userdata */
  i=xget(obj, MUIA_UserData);
  if(i>0) {
    DebOut("MUIA_UserData: %d\n", i);
    return i;
  }

  while(data->src[i].exists) {
    if(data->src[i].obj == obj) {
      DebOut("found! i: %d\n", i);
      return i;
    }
    i++;
  }

  DebOut("ERROR: not found!\n");
  return -1;
}

/*****************************************************************************
 * WinAPI
 *****************************************************************************/
BOOL SetDlgItemText(Element *elem, int item, TCHAR *lpString) {
  Object *obj;
  LONG i;

  DebOut("elem: %lx\n", elem);
  DebOut("item: %d\n", item);
  DebOut("string: %s\n", lpString);

  i=get_index(elem, item);
  if(i<0) {
    elem=get_elem(item);
    i=get_index(elem, item);
  }

  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n");
    return FALSE;
  }

  DebOut("elem[i].obj: %lx\n", elem[i].obj);

  if(elem[i].windows_type==EDITTEXT) {
    SetAttrs(elem[i].obj, MUIA_String_Contents, lpString, TAG_DONE);
  }
  else if (elem[i].windows_type==COMBOBOX) {
    DebOut("COMBOBOX: call SendDlgItemMessage instead:\n");
    return SendDlgItemMessage(elem, item, CB_ADDSTRING, 0, (LPARAM) lpString);
  }
  else {
    SetAttrs(elem[i].obj, MUIA_Text_Contents,   lpString, TAG_DONE);
  }

  return TRUE;
}

/*
 * select/deselect checkbox
 */
BOOL CheckDlgButton(Element *elem, int button, UINT uCheck) {
  LONG i;

  DebOut("elem: %lx\n", elem);
  DebOut("button: %d\n", button);
  DebOut("uCheck: %d\n", uCheck);

  i=get_index(elem, button);

  if(i<0) {
    elem=get_elem(button);
    i=get_index(elem, button);
  }

  DebOut("elem[i].obj: %lx\n", elem[i].obj);


  SetAttrs(elem[i].obj, MUIA_Selected, uCheck, TAG_DONE);
  //SetAttrs(elem[i].obj, MUIA_Pressed, uCheck, TAG_DONE);

  return TRUE;

}

LONG SendDlgItemMessage(struct Element *elem, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam) {
  Object *obj;
  LONG i;
  ULONG l;
  ULONG activate;
  TCHAR *buf;

  DebOut("== entered ==\n");
  DebOut("elem: %lx, nIDDlgItem: %d\n", elem, nIDDlgItem);

  i=get_index(elem, nIDDlgItem);
  /* might be in a different element..*/
  if(i<0) {
    elem=get_elem(nIDDlgItem);
    i=get_index(elem, nIDDlgItem);
  }

  /* still not found !? */
  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", nIDDlgItem);
    return FALSE;
  }
  DebOut("index: %d\n", i);
  //DebOut("elem[%d].var: %lx\n", i, elem[i].var);

  switch(Msg) {

    case CB_ADDSTRING:
      /* add string to popup window */
      DebOut("CB_ADDSTRING\n");
      DebOut("new string: >%s<\n", (TCHAR *) lParam);
      l=0;
      DebOut("old list:\n");
      while(elem[i].var[l] != NULL) {
        DebOut("  elem[i].var[%d]: %s\n", l, elem[i].var[l]);
        l++;
      }
      /* found next free string */
      DebOut("  next free line: %d\n", l);
      activate=l;
#warning TODO: free strings again!
      elem[i].var[l]=strdup((TCHAR *) lParam);
      elem[i].var[l+1]=NULL;
      DebOut("elem[i].obj: %lx\n", elem[i].obj);
      SetAttrs(elem[i].obj, MUIA_Cycle_Entries, (ULONG) elem[i].mem, TAG_DONE);
      DebOut("new list:\n");
      l=0;
      while(elem[i].var[l] != NULL) {
        DebOut("  elem[i].var[%d]: %s\n", l, elem[i].var[l]);
        l++;
      }
      SetAttrs(elem[i].obj, MUIA_Cycle_Active, activate, TAG_DONE);
      break;

    case CB_RESETCONTENT:
      /* delete all strings */
      DebOut("CB_RESETCONTENT\n");
      l=0;
      /* free old strings */
      while(elem[i].var[l] != NULL) {
        free(elem[i].var[l]);
        elem[i].var[l]=NULL;
        l++;
      }
      //elem[i].var[0]=strdup("<empty>");
      //elem[i].var[1]=NULL;
      elem[i].var[0]=NULL;
      DebOut("elem[i].obj: %lx\n", elem[i].obj);
      SetAttrs(elem[i].obj, MUIA_Cycle_Entries, (ULONG) elem[i].mem, MUIA_NoNotify, TRUE, TAG_DONE);
      break;
    case CB_FINDSTRING:
      /* return string index */
      DebOut("CB_FINDSTRING\n");
      DebOut("search for string >%s<\n", (TCHAR *) lParam);
      l=0;
      while(elem[i].var[l] != NULL) {
        DebOut("  compare with >%s<\n", elem[i].var[l]);
        if(!strcmp(elem[i].var[l], (TCHAR *) lParam)) {
          return l;
        }
        l++;
      }
      DebOut("return -1\n");
      return -1;

    case CB_GETCURSEL:
      DebOut("CB_GETCURSEL\n");
      return elem[i].value;
      break;
    case WM_GETTEXT:
      DebOut("[%lx] WM_GETTEXT\n", elem[i].obj);
      {
        TCHAR *string=(TCHAR *) lParam;
        DebOut("[%lx] elem[i].value: %d\n", elem[i].obj, elem[i].value);
        DebOut("[%lx] wParam: %d\n", elem[i].obj, wParam);
        /* warning: care for non-comboboxes, too! */
        if(elem[i].value<0) {
          string[0]=(char) 0;
        }
        else {
          DebOut("return: %s\n", elem[i].var[elem[i].value]);
          strncpy(string, elem[i].var[elem[i].value], wParam);
        }
      }
      break;
    case WM_SETTEXT:
      DebOut("WM_SETTEXT\n");
      if(lParam== NULL || strlen((TCHAR *)lParam)==0) {
        DebOut("lParam is empty\n");
        DebOut("elem[i].obj: %lx\n", elem[i].obj);
        SetAttrs(elem[i].obj, MUIA_Cycle_Active, 1, TAG_DONE);
        return 0;
      }
      /* check, if the string is already available */
      DebOut("search for string >%s<\n", (TCHAR *) lParam);

      l=0;
      while(elem[i].var[l] != NULL) {
        DebOut("  compare with >%s<\n", elem[i].var[l]);
        DebOut("  l: %d\n", l);
        if(!strcmp(elem[i].var[l], (TCHAR *) lParam)) {
          DebOut("we already have that string!\n");
          DebOut("activate string %d in combo box!\n", l);
          DebOut("elem[i].obj: %lx\n", elem[i].obj);
          SetAttrs(elem[i].obj, MUIA_Cycle_Active, l, TAG_DONE);
          return 0;
        }
        l++;
      }
      /* new string, add it to top */

      DebOut("new string, add it to top\n");

      l=0;
      DebOut("old list:\n");
      while(elem[i].var[l] != NULL) {
        DebOut("  elem[i].var[%d]: %s\n", l, elem[i].var[l]);
        l++;
      }

      l=0;
      buf=elem[i].var[l+1];
      while(buf != NULL) {
        elem[i].var[l+1]=elem[i].var[l];
        l++;
        buf=elem[i].var[l+1];
      }
      /* store new string */
      elem[i].var[0]=strdup((TCHAR *) lParam);
      /* terminate list */
      elem[i].var[l+1]=NULL;
      DebOut("elem[i].obj: %lx\n", elem[i].obj);
      SetAttrs(elem[i].obj, MUIA_Cycle_Entries, (ULONG) elem[i].mem, TAG_DONE);
      if(flag_editable(elem[i].flags)) {
        SetAttrs(elem[i].obj, MUIA_Cycle_Active, 1, TAG_DONE);
      }
      else {
        SetAttrs(elem[i].obj, MUIA_Cycle_Active, 0, TAG_DONE);
      }

      DebOut("new list:\n");
      l=0;
      while(elem[i].var[l]!=NULL) {
        DebOut("  elem[i].var[%d]: %s\n", l, elem[i].var[l]);
        l++;
      }
      return TRUE;
      
      /* An application sends a CB_SETCURSEL message to select a string in the 
       * list of a combo box. If necessary, the list scrolls the string into view. 
       * The text in the edit control of the combo box changes to reflect the new 
       * selection, and any previous selection in the list is removed. 
       *
       * wParam: Specifies the zero-based index of the string to select. 
       * If this parameter is 1, any current selection in the list is removed and 
       * the edit control is cleared.
       *
       * If the message is successful, the return value is the index of the item 
       * selected. If wParam is greater than the number of items in the list or 
       * if wParam is 1, the return value is CB_ERR and the selection is cleared. 
       */
    case CB_SETCURSEL:
      LONG foo;
      DebOut("CB_SETCURSEL\n");
      DebOut("wParam: %d\n", wParam);
      foo=wParam;
      if(flag_editable(elem[i].flags)) {
        foo++;
      }
      SetAttrs(elem[i].obj, MUIA_Cycle_Active, foo, TAG_DONE);
      return TRUE;

    default:
      DebOut("WARNING: unkown Windows Message-ID: %d\n", Msg);
      return FALSE;
  }

  return TRUE;
}

UINT GetDlgItemText(HWND elem, int nIDDlgItem, TCHAR *lpString, int nMaxCount) {
  int i;
  UINT ret;
  char buffer[256]; // ATTENTION: this might crash badly..

  i=get_index(elem, nIDDlgItem);
  /* might be in a different element..*/
  if(i<0) {
    elem=get_elem(nIDDlgItem);
    i=get_index(elem, nIDDlgItem);
  }

  /* still not found !? */
  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", nIDDlgItem);
    return 0;
  }
  DebOut("index: %d\n", i);
  DebOut("elem[i].obj: %lx\n", elem[i].obj);

  GetAttr(MUIA_String_Contents, elem[i].obj, (IPTR *) &buffer);

  strncpy(lpString, buffer, nMaxCount);

  return strlen(lpString);
}

/*
 * Sets the text of a control in a dialog box to the string representation of a specified integer value.
 */
BOOL SetDlgItemInt(HWND elem, int item, UINT uValue, BOOL bSigned) {
  Object *obj;
  LONG i;
  TCHAR tmp[64];

  DebOut("elem: %lx\n", elem);
  DebOut("item: %d\n", item);
  DebOut("value: %d\n", uValue);
  DebOut("bSigned: %d\n", bSigned);

  i=get_index(elem, item);
  if(i<0) {
    elem=get_elem(item);
    i=get_index(elem, item);
  }

  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n");
    return FALSE;
  }

  if(bSigned) {
    snprintf(tmp, 64, "-%d", uValue);
  }
  else {
    snprintf(tmp, 64, "%d", uValue);
  }

  DebOut("set to: %s\n", tmp);

  if(!elem[i].var) {
    /* array for text (only one line, but respect data structure here */
    elem[i].var=(char **) malloc(16 * sizeof(ULONG *)); 
    elem[i].var[0]=NULL;
  }

  if(elem[i].var[0]) {
    /* free old content */
    free(elem[i].var[0]);
  }

  elem[i].var[0]=strdup(tmp);
  elem[i].var[1]=NULL;

  DebOut("elem[i].obj: %lx\n", elem[i].obj);
  if(elem[i].windows_type==EDITTEXT) {
    DebOut("elem[i].windows_type==EDITTEXT\n");
    SetAttrs(elem[i].obj, MUIA_String_Contents, elem[i].var[0], TAG_DONE);
  }
  else {
    DebOut("elem[i].windows_type!=EDITTEXT\n");
    SetAttrs(elem[i].obj, MUIA_Text_Contents, elem[i].var[0], TAG_DONE);
  }

  return TRUE;
}

/* Adds a check mark to (checks) a specified radio button in a group and removes 
 * a check mark from (clears) all other radio buttons in the group. 
 */
BOOL CheckRadioButton(HWND elem, int nIDFirstButton, int nIDLastButton, int nIDCheckButton) {
  int i;
  int e;
  i=nIDFirstButton;
  while(i<nIDLastButton) {
    e=get_index(elem, i);
    /* might be in a different element..*/
    if(e<0) {
      elem=get_elem(i);
      e=get_index(elem, i);
    }

    /* still not found !? */
    if(e<0) {
      DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", i);
    }
    else {
      DebOut("index: %d\n", e);
      SetAttrs(elem[e].obj, MUIA_Selected, FALSE, TAG_DONE);
    }
    i++;
  }

  e=get_index(elem, nIDCheckButton);
  /* might be in a different element..*/
  if(e<0) {
    elem=get_elem(i);
    e=get_index(elem, i);
  }

  /* still not found !? */
  if(e<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", i);
  }
  else {
    DebOut("index: %d\n", e);
    DebOut("elem[i].obj: %lx\n", elem[i].obj);
    SetAttrs(elem[e].obj, MUIA_Selected, TRUE, TAG_DONE);
  }
}

UINT IsDlgButtonChecked(HWND elem, int item) {
  int i;
  int res;

  DebOut("elem: %lx\n", elem);
  DebOut("item: %d\n", item);

  i=get_index(elem, item);
  if(i<0) {
    elem=get_elem(item);
    i=get_index(elem, item);
  }

  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n");
    return FALSE;
  }

  DebOut("elem[i].obj: %lx\n", elem[i].obj);
  DebOut("elem[i].value: %lx\n", elem[i].value);

  return elem[i].value;
}

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
    {RT_TextAttr, (ULONG) &myta},
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

  window=(struct Window *)xget(win, MUIA_Window_Window);
  if(window) {
    DebOut("window: %lx\n", window);
    tags[1].ti_Data=(ULONG) window;
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

  DebOut("MessageBox(): %s \n", lpText);

  if(!uType & MB_OK) {
    DebOut("WARNING: unsupported type: %lx\n", uType);
  }

  req.es_StructSize=sizeof(struct EasyStruct);
  req.es_TextFormat=lpText;
  if(lpCaption) {
    req.es_Title=lpCaption;
  }
  else {
    req.es_Title="Error";
  }
  req.es_GadgetFormat="Ok";

  EasyRequestArgs(NULL, &req, NULL, NULL );
}


/* WARNING: not same API call as in Windows */
BOOL EnableWindow(HWND hWnd, DWORD id, BOOL bEnable) {
  int i;
  int res;

  DebOut("elem: %lx, id %d\n", hWnd, id);

  i=get_index(hWnd, id);
  if(i<0) {
    hWnd=get_elem(id);
    i=get_index(hWnd, id);
  }

  if(i<0) {
    DebOut("ERROR: could not find elem %lx id %d\n", hWnd, id);
    return FALSE;
  }

  DebOut("hWnd[%d].obj: %lx\n", i, hWnd[i].obj);
  DebOut("SetAttrs(hWnd[%d].obj, MUIA_Disabled, %d, TAG_DONE);\n", i, !bEnable);

  DebOut("hWnd[i].obj: %lx\n", hWnd[i].obj);
  SetAttrs(hWnd[i].obj, MUIA_Disabled, !bEnable, TAG_DONE);

  return TRUE;
}
/*
 * Retrieves a handle to a control in the specified dialog box.
 */
#if 0
HWND GetDlgItem(HWND hDlg, int nIDDlgItem) {
  int i;
  UINT ret;
  char buffer[256]; // ATTENTION: this might crash badly..

  i=get_index(hDlg, nIDDlgItem);

  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d not found in %lx!?\n", nIDDlgItem, hDlg);
    return NULL;
  }
  DebOut("index: %d\n", i);

  return hDlg[i];
}


/* Changes the text of the specified window's title bar (if it has one). 
 * If the specified window is a control, the text of the control is changed.
 */
BOOL SetWindowText(HWND hDlg, TCHAR *lpString) {

  DebOut("lpString: %s\n", lpString);
  DebOut("hWnd: %lx\n", hWnd);
}
#endif
BOOL ShowWindow(HWND hWnd, int nCmdShow) {
  TODO();
}; 

AROS_UFH2(void, MUIHook_combo, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  int i;
  ULONG wParam;

  struct Data *data = (struct Data *) hook->h_Data;


  DebOut("[%lx] entered\n", obj);
  DebOut("[%lx] hook.h_Data: %lx\n", obj, hook->h_Data);
  DebOut("[%lx] obj: %lx\n", obj);

  i=get_elem_from_obj(data, (Object *) obj);

  DebOut("[%lx] i: %d\n", obj, i);

  data->src[i].value=xget((Object *) obj, MUIA_Cycle_Active);
  DebOut("[%lx] MUIA_Cycle_Active: %d\n", obj, data->src[i].value);
  if(flag_editable(data->src[i].flags)) {
    data->src[i].value--;
  }

  DebOut("[%lx] We are in state: %d\n", obj, data->src[i].value);

  if(data->func) {
    DebOut("[%lx] call function: %lx\n", obj, data->func);
    DebOut("[%lx] IDC: %d\n", obj, data->src[i].idc);
    wParam=MAKELPARAM(data->src[i].idc, CBN_SELCHANGE);
    DebOut("[%lx] wParam: %lx\n", obj, wParam);
    data->func(data->src, WM_COMMAND, wParam, NULL);
  }
  else {
    DebOut("[%lx] function is zero: %lx\n", obj, data->func);
  }

  AROS_USERFUNC_EXIT
}


AROS_UFH2(void, MUIHook_select, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  int i;
  ULONG wParam;

  struct Data *data = (struct Data *) hook->h_Data;


  DebOut("entered\n");
  DebOut("hook.h_Data: %lx\n", hook->h_Data);
  DebOut("obj: %lx\n", obj);

  i=get_elem_from_obj(data, (Object *) obj);

  DebOut("i: %d\n", i);

  data->src[i].value=xget((Object *) obj, MUIA_Selected);

  DebOut("We are in state Selected: %d\n", (ULONG) xget((Object *) obj, MUIA_Selected));

  if(data->func) {
    DebOut("call function: %lx\n", data->func);
    DebOut("IDC: %d\n", data->src[i].idc);
    DebOut("WM_COMMAND: %d\n", WM_COMMAND);
    wParam=MAKELPARAM(data->src[i].idc, CBN_SELCHANGE);
    DebOut("wParam: %lx\n", wParam);
    data->func(data->src, WM_COMMAND, wParam, NULL);
  }
  else {
    DebOut("function is zero: %lx\n", data->func);
  }

  AROS_USERFUNC_EXIT
}

AROS_UFH2(void, MUIHook_control, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT
  int i;
  ULONG wParam;

  struct Data *data = (struct Data *) hook->h_Data;


  DebOut("entered\n");
  DebOut("hook.h_Data: %lx\n", hook->h_Data);
  DebOut("obj: %lx\n", obj);

  i=get_elem_from_obj(data, (Object *) obj);

  DebOut("i: %d\n", i);

  if(data->func) {
    DebOut("call function: %lx\n", data->func);
    DebOut("IDC: %d\n", data->src[i].idc);
    DebOut("WM_COMMAND: %d\n", WM_COMMAND);
    wParam=MAKELPARAM(data->src[i].idc, CBN_SELCHANGE);
    DebOut("wParam: %lx\n", wParam);
    data->func(data->src, WM_COMMAND, wParam, NULL);
    wParam=MAKELPARAM(data->src[i].idc, BN_CLICKED);
    DebOut("wParam: %lx\n", wParam);
    data->func(data->src, WM_COMMAND, wParam, NULL);
  }
  else {
    DebOut("function is zero: %lx\n", data->func);
  }

  AROS_USERFUNC_EXIT
}

/*****************************************************************************
 * Class
 *****************************************************************************/
AROS_UFH3(static ULONG, LayoutHook, AROS_UFHA(struct Hook *, hook, a0), AROS_UFHA(APTR, obj, a2), AROS_UFHA(struct MUI_LayoutMsg *, lm, a1)) 

  switch (lm->lm_Type) {
    case MUILM_MINMAX: {

      struct Data *data = (struct Data *) hook->h_Data;
      ULONG i=0;
      struct Element *element=data->src;
      ULONG mincw, minch, defcw, defch, maxcw, maxch;

      //DebOut("MUILM_MINMAX obj %lx\n", obj);
      //DebOut("data:        %lx\n", data);
      //DebOut("data->src:   %lx\n", data->src);

      lm->lm_MinMax.MinWidth  = data->width * RESIZE_X;
      lm->lm_MinMax.MinHeight = data->height * RESIZE_Y;
      lm->lm_MinMax.DefWidth  = data->width * RESIZE_X;
      lm->lm_MinMax.DefHeight = data->height * RESIZE_Y;
      lm->lm_MinMax.MaxWidth  = data->width * RESIZE_X;
      lm->lm_MinMax.MaxHeight = data->height * RESIZE_Y;

      //DebOut("  mincw=%d\n",lm->lm_MinMax.MinWidth);
      //DebOut("  minch=%d\n",lm->lm_MinMax.MinHeight);
      //DebOut("  maxcw=%d\n",lm->lm_MinMax.MaxWidth);
      //DebOut("  maxch=%d\n",lm->lm_MinMax.MaxHeight);

      //DebOut("MUILM_MINMAX done\n");
    }
    return 0;

    case MUILM_LAYOUT: {
      struct Data *data = (struct Data *) hook->h_Data;
      ULONG i=0;
      struct Element *element=data->src;

      //DebOut("MUILM_LAYOUT obj %lx\n", obj);
      //DebOut("data:        %lx\n", data);
      //DebOut("data->src:   %lx\n", data->src);

      while(element[i].exists) {
        //DebOut("  child i=%d\n", i);
        //DebOut("   obj[%d]: %lx\n", i, element[i].obj);
        //DebOut("   idc: %d (dummy is %d)\n", element[i].idc, IDC_dummy);
        //DebOut("   x, y: %d, %d  w, h: %d, %d\n", element[i].x, element[i].y, element[i].w, element[i].h);
        if(element[i].obj != obj) {
          if (!MUI_Layout(element[i].obj,
                          element[i].x * RESIZE_X,
                          element[i].y * RESIZE_Y,
                          element[i].w * RESIZE_X,
                          element[i].h * RESIZE_Y, 0)
             ) {
            DebOut("MUI_Layout failed!\n");
            return FALSE;
          }
        }
        else {
          DebOut("   RECURSION !?!?!: %lx\n", element[i].obj);
        }
        i++;
      }
      //DebOut("MUILM_LAYOUT done\n");
    }
    return TRUE;
  }

  return MUILM_UNKNOWN;
}

static ULONG mNew(struct IClass *cl, APTR obj, Msg msg) {

  struct TagItem *tstate, *tag;
  ULONG i=0;
  ULONG s=0;
  struct Element *src=NULL;
  Object *child      =NULL; /* TODO: remove me */
  int *(*func) (Element *hDlg, UINT msg, ULONG wParam, ULONG lParam);
  func=NULL;

  DebOut("mNew(fixed)\n");

  obj = (APTR) DoSuperMethodA(cl, (Object*) obj, msg);

  if (!obj) {
    DebOut("fixed: unable to create object!");
    return (ULONG) NULL;
  }

  tstate=((struct opSet *)msg)->ops_AttrList;

  while ((tag = (struct TagItem *) NextTagItem((TagItem**) &tstate))) {
    DebOut("tag->ti_Tag: %lx\n", tag->ti_Tag);
    switch (tag->ti_Tag) {
      case MA_src:
        s = tag->ti_Data;
        break;
      case MA_dlgproc:
        DebOut("MA_dlgproc: %lx\n", tag->ti_Data);
        func=(int* (*)(Element*, uint32_t, ULONG, ULONG)) tag->ti_Data;
        break;
    }
  }

  if(!s) {
    DebOut("mNew: MA_src not supplied!\n");
    return (ULONG) NULL;
  }

  /* place vertically */
  SetAttrs(obj, MUIA_Group_Horiz, FALSE, TAG_DONE);

  src=(struct Element *) s;

  DebOut("receive: %lx\n", s);
  {
    GETDATA;
    //struct TextAttr ta;
    struct TextFont *old;
    Object *foo;

    data->width =396;
    data->height=320;
    data->src   =src;
    data->func  =func;

    DebOut("YYYYYYYYYYYYYYYYYY\n");

    struct TextAttr ta = { "Vera Sans", 12, 0, 0 };
    TextFont *Topaz8Font = OpenDiskFont(&ta);
    DebOut("Topaz8Font: %lx\n", Topaz8Font);
#if 0
    foo=TextObject, MUIA_Text_Contents, (ULONG) "XYZ", End;
    old=(struct TextFont *) xget(foo, MUIA_Font);
    //GetAttr(MUIA_Font, (Object *)foo, (IPTR *)old);
    DebOut("MUIA_Font: %s\n", old->tf_Message.mn_Node.ln_Name);
    DebOut("MUIA_Font: %d\n", old->tf_YSize);
    DebOut("YYYYYYYYYYYYYYYYYY\n");


    GetAttr(MUIA_Font, (Object *)obj, (IPTR *) &old);
    ta.ta_Name = GfxBase->DefaultFont->tf_Message.mn_Node.ln_Name;
    ta.ta_YSize = GfxBase->DefaultFont->tf_YSize;
    ta.ta_Style = GfxBase->DefaultFont->tf_Style;
    ta.ta_Flags = GfxBase->DefaultFont->tf_Flags;
    DebOut("OpenFont: %s\n", ta.ta_Name);
    data->font=OpenDiskFont(&ta);
    DebOut("data->font: %lx\n", data->font);
#endif


    DebOut("XXXXXXXXXXXXXXXXXXXXXX\n");
    i=0;
    while(src[i].windows_type) {
      DebOut("========== i=%d =========\n", i);
      DebOut("add %s\n", src[i].text);
      switch(src[i].windows_type) {
        case GROUPBOX:
          child=HGroup, MUIA_Frame, MUIV_Frame_Group,
                              MUIA_FrameTitle, src[i].text,
                              End;
          src[i].exists=TRUE;
          src[i].obj=child;
        break;

        case CONTROL:
          if(src[i].windows_class==NULL || !strcmp(src[i].windows_class, "Button") || /* implement those: !!!  */ !strcmp(src[i].windows_class, "SysListView32") || !strcmp(src[i].windows_class, "msctls_trackbar32") || !strcmp(src[i].windows_class, "Static") || !strcmp(src[i].windows_class, "SysTreeView32")) {
            DebOut("Button found\n");
            if(src[i].flags2==BS_AUTORADIOBUTTON) {
              child=HGroup,
                    MUIA_UserData         , i,
                    Child, src[i].obj=ImageObject,
                        NoFrame,
                        MUIA_CycleChain       , 1,
                        MUIA_InputMode        , MUIV_InputMode_Toggle,
                        MUIA_Image_Spec       , MUII_RadioButton,
                        MUIA_ShowSelState     , FALSE,
                        MUIA_Selected         , FALSE,
                        MUIA_UserData         , i,
                      End,
                    Child, TextObject,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_Contents, src[i].text,
                        MUIA_UserData         , i,
                      End,

                  End;

            }
            else {
              if(src[i].text && (src[i].text[0]!=(char) 0)) {
                child=HGroup,
                  MUIA_UserData         , i,
                  Child, src[i].obj=ImageObject,
                    ImageButtonFrame,
                    MUIA_InputMode   , MUIV_InputMode_Toggle,
                    MUIA_Image_Spec  , MUII_CheckMark,
                    MUIA_Background  , MUII_ButtonBack,
                    MUIA_ShowSelState, FALSE,
                    MUIA_CycleChain  , TRUE,
                    MUIA_UserData         , i,
                  End,
                  Child, TextObject,
                    MUIA_Font, Topaz8Font,
                    MUIA_Text_Contents, src[i].text,
                    MUIA_UserData         , i,
                    End,
                End;
              }
              else {
                /* buton without text must be without text, otherwise zune draws "over the border" */
                child=HGroup,
                  MUIA_UserData         , i,
                  Child, src[i].obj=ImageObject,
                    ImageButtonFrame,
                    MUIA_InputMode   , MUIV_InputMode_Toggle,
                    MUIA_Image_Spec  , MUII_CheckMark,
                    MUIA_Background  , MUII_ButtonBack,
                    MUIA_ShowSelState, FALSE,
                    MUIA_CycleChain  , TRUE,
                    MUIA_UserData         , i,
                  End,
                End;
              }
              data->MyMUIHook_select.h_Entry=(APTR) MUIHook_select;
              data->MyMUIHook_select.h_Data =(APTR) data;
              DebOut("DoMethod(%lx, MUIM_Notify, MUIA_Selected, MUIV_EveryTime..)\n", src[i].obj);
              DoMethod(src[i].obj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, (ULONG) src[i].obj, 2, MUIM_CallHook,(ULONG) &data->MyMUIHook_select, func); 

            }
          }
          else if(!strcmp(src[i].windows_class, "RICHEDIT")) {
            DebOut("RICHEDIT found (text: %s\n", src[i].text);
            child=TextObject,
                    MUIA_Font, Topaz8Font,
                     MUIA_Text_PreParse, "\33c",
                    MUIA_Text_Contents, src[i].text,
                    MUIA_UserData         , i,
                  End;
          }
          else {
            fprintf(stderr, "ERROR: Unknown windows class \"%s\" found!\n", src[i].windows_class);
            DebOut("ERROR: Unknown windows class \"%s\" found!\n", src[i].windows_class);
            DebOut("We *will* crash on this!!\n");
          }
          if(child) {
            src[i].exists=TRUE;
            src[i].obj=child;
          }
        break;

        case PUSHBUTTON:
          //src[i].obj=MUI_MakeObject(MUIO_Button, (ULONG) src[i].text);
          child=HGroup, MUIA_Background, MUII_ButtonBack,
                              ButtonFrame,
                              MUIA_InputMode , MUIV_InputMode_RelVerify,
                              Child, TextObject,
                                MUIA_Font, Topaz8Font,
                                MUIA_Text_PreParse, "\33c",
                                MUIA_Text_Contents, (ULONG) src[i].text,
                              End,
                            End;

          src[i].exists=TRUE;
          src[i].obj=child;
          /* Add hook */
          /* TODO !!! */
          //Move this to the "..." gadget!! and call func then..
          data->MyMUIHook_control.h_Entry=(APTR) MUIHook_control;
          data->MyMUIHook_control.h_Data =(APTR) data;
          DoMethod(src[i].obj, MUIM_Notify, MUIA_Pressed, FALSE, (ULONG) src[i].obj, 2, MUIM_CallHook,(ULONG) &data->MyMUIHook_control, func); 
        break;

        case RTEXT:
          child=TextObject,
                        //MUIA_Background, MUII_MARKBACKGROUND,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33r",
                        MUIA_Text_Contents, (ULONG) src[i].text,
                      End;
          src[i].exists=TRUE;
          src[i].obj=child;
        break;

        case LTEXT:
          child=TextObject,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33l",
                        MUIA_Text_Contents, (ULONG) src[i].text,
                      End;
          src[i].exists=TRUE;
          src[i].obj=child;
        break;


        case EDITTEXT:
          if(src[i].flags & ES_READONLY) {
            child=TextObject,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33c",
                        MUIA_Text_Contents, (ULONG) src[i].text,
                      End;
            src[i].obj=child;
          }
          else {
            child=StringObject,
                          StringFrame,
                          MUIA_Font, Topaz8Font,
                          MUIA_String_Contents, (ULONG) src[i].text,
                        End;
            src[i].obj=child;
          }
          src[i].exists=TRUE;
          src[i].var=(char **) malloc(256 * sizeof(ULONG *)); // array for cycle texts
          src[i].var[0]=NULL;
        break;


        case COMBOBOX:
          src[i].mem=(char **) malloc(256 * sizeof(ULONG *)); // array for cycle texts
          DebOut("flags: %lx\n", src[i].flags);
          if(!flag_editable(src[i].flags)) {
            DebOut("flags: CBS_DROPDOWNLIST\n");
            src[i].var=src[i].mem;
          }
          else {
            /* must contain "<<empty>>" statement */
            DebOut("flags: CBS_DROPDOWN\n");
            src[i].mem[0]=strdup("<<empty>>");
            src[i].var=src[i].mem+1;
          }
          src[i].var[0]=NULL;
          src[i].exists=TRUE;
          child=CycleObject,
                       MUIA_Font, Topaz8Font,
                       MUIA_Cycle_Entries, src[i].mem,
                     End;
          src[i].obj=child;
          data->MyMUIHook_combo.h_Entry=(APTR) MUIHook_combo;
          data->MyMUIHook_combo.h_Data =(APTR) data;
          DoMethod(src[i].obj, MUIM_Notify, MUIA_Cycle_Active , MUIV_EveryTime, (ULONG) src[i].obj, 2, MUIM_CallHook,(ULONG) &data->MyMUIHook_combo, func); 
        break;


        default:
          DebOut("ERROR: unknown windows_type %d\n", src[i].windows_type);
      }
      DebOut("RESULT: child:   %lx\n", child);
      DebOut("RESULT: src.obj: %lx\n", src[i].obj);
      if(child) {
        if(src[i].help) {
          DebOut("add HOTHELP: %s\n", src[i].help);
          SetAttrs(child, MUIA_ShortHelp, (ULONG) src[i].help, TAG_DONE);
        }
        if(!(src[i].flags & WS_VISIBLE)) {
          SetAttrs(child, MUIA_ShowMe, (ULONG) 0, TAG_DONE);
        }
        child=NULL;
      }
      DebOut("  src[%d].obj=%lx\n", i, src[i].obj);
      i++;

    }

    //SETUPHOOK(&data->LayoutHook, LayoutHook, data);
    data->LayoutHook.h_Entry=(APTR) LayoutHook;
    data->LayoutHook.h_Data=data;

    /*
     * ATTENTION: Add children *after* seting the LayoutHook,
     *            otherwise we get recursions!
     */
  
    SetAttrs(obj, MUIA_Group_LayoutHook, &data->LayoutHook, TAG_DONE);
    i=0;
    while(src[i].exists) {
      DebOut("i: %d (add %lx to %lx\n", i, src[i].obj, obj);
      DoMethod((Object *) obj, OM_ADDMEMBER,(LONG) src[i].obj);
      i++;
    }

    mSet(data, obj, (struct opSet *) msg, 1);
    SetAttrs(obj, MUIA_Frame, MUIV_Frame_Group, TAG_DONE);
  }
  return (ULONG)obj;
}

static ULONG mGet(struct Data *data, APTR obj, struct opGet *msg, struct IClass *cl)
{
  ULONG rc;

  //DebOut("mGet..\n");

  switch (msg->opg_AttrID)
  {
    case MA_Data: 
      DebOut("data: %lx\n", data);
      rc = (ULONG) data; 
      break;

    default: return DoSuperMethodA(cl, (Object *) obj, (Msg)msg);
  }

  *msg->opg_Storage = rc;

  return TRUE;
}

static VOID mSet(struct Data *data, APTR obj, struct opSet *msg, ULONG is_new)
{
  struct TagItem *tstate, *tag;

  tstate = msg->ops_AttrList;

  while ((tag = NextTagItem((TagItem**)&tstate))) {

    switch (tag->ti_Tag) {
//      case MA_Fixed_Move:
 //       fixed_move(data,obj,(struct MA_Fixed_Move_Data *)tag->ti_Data);
  //      break;
    }
  }
}

static VOID mRemMember(struct Data *data, struct opMember *msg) {
#if 0
  struct FixedNode *node;

  ForeachNode(&data->ChildList, node) {
    if (GtkObj(node->widget) == msg->opam_Object)
    {
      REMOVE(node);
      mgtk_freemem(node, sizeof(*node));
      return;
    }
  }
#endif
}

BEGINMTABLE
GETDATA;

//DebOut("(%lx)->msg->MethodID: %lx\n",obj,msg->MethodID);


  switch (msg->MethodID)
  {
    case OM_NEW         : return mNew           (cl, obj, msg);
  	case OM_SET         :        mSet           (data, obj, (opSet *)msg, FALSE); break;
    case OM_GET         : return mGet           (data, obj, (opGet *)msg, cl);
    case OM_REMMEMBER   :        mRemMember     (data,      (opMember *)msg); break;
  }

ENDMTABLE

int init_class(void) {

  if(!CL_Fixed) {
    CL_Fixed = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct Data), (APTR)&mDispatcher);
  }

  return CL_Fixed ? 1 : 0;
}

void delete_class(void) {

  if (CL_Fixed) {
    MUI_DeleteCustomClass(CL_Fixed);
    CL_Fixed=NULL;
  }
}


