
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

#define EMPTY_SELECTION "<-- none -->"

struct Data {
  struct Hook LayoutHook;
  struct Hook MyMUIHook_pushbutton;
  struct Hook MyMUIHook_select;
  struct Hook MyMUIHook_slide;
  struct Hook MyMUIHook_combo;
  ULONG width, height;
  struct Element *src;
  struct TextFont *font;
  int *(*func) (Element *hDlg, UINT msg, ULONG wParam, IPTR lParam);
};

static const char *Cycle_Dummy[] = { NULL };

struct ReqToolsBase *ReqToolsBase=NULL;
struct MUI_CustomClass *CL_Fixed;

static VOID mSet(struct Data *data, APTR obj, struct opSet *msg, ULONG is_new);

extern Object *win;

#if (0)
IPTR xget(Object *obj, IPTR attr) {
  IPTR b = 0;
  GetAttr(attr, obj, (IPTR *)&b);
  return b;
}
#endif

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

  //DebOut("obj: %lx, data %lx\n", obj, data);

  /* first try userdata */
  i=XGET(obj, MUIA_UserData);
  if(i>0) {
    //DebOut("MUIA_UserData: %d\n", i);
    return i;
  }

  while(data->src[i].exists) {
    if(data->src[i].obj == obj) {
      //DebOut("found! i: %d\n", i);
      return i;
    }
    i++;
  }

  DebOut("ERROR: obj %lx, data %lx not found!\n", obj, data);
  return -1;
}

/*****************************************************************************
 * WinAPI
 *****************************************************************************/

/*
 * Sends the specified message to a window or windows. The SendMessage function 
 * calls the window procedure for the specified window and does not return 
 * until the window procedure has processed the message.
 *
 * Usually, this function takes a HWND, but we just take a IDC_ define here.
 * TAKE CARE FOR THIS!
 *
 * MAYBE better rewrite caller to use SendDlgItemMessage!!
 */
LRESULT SendMessage____untested____(int item, UINT Msg, WPARAM wParam, LPARAM lParam) {
  Element *elem;
  LONG i;
#if 0
  struct Data *data = NULL;
  Object *foo;
#endif
  LRESULT res;

  DebOut("item %d, Msg %d, wParam %d, lParam %d\n", item, Msg, wParam, lParam);

  elem=get_elem(item);
  i=get_index(elem, item);

  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", item);
    return 0;
  }
  DebOut("elem: %lx, i: %d\n", elem, i);

#if 0
  /* yes, I know, this is ugly ..
   * accessing objects data from outside
   */
  foo=elem[i].obj;
  while(foo && !data) {
    data=(struct Data *) XGET(foo, MA_Data);
    foo=_parent(foo);
  }
  if(!data) {
    DebOut("ERROR: retrieve data from Object hack failed!\n");
    return 0;
  }
#endif

  switch(Msg) {
    case TBM_GETPOS:
      DebOut("TBM_GETPOS\n");
      /*
       * Retrieves the current logical position of the slider in a trackbar. 
       * The logical positions are the integer values in the trackbar's range 
       * of minimum to maximum slider positions. 
       */
      res=XGET(elem[i].obj, MUIA_Numeric_Default);
      DebOut("result: %d\n", res);
      return res;

    default:
      TODO();
  }

  return FALSE;
}

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
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", item);
    return FALSE;
  }

  DebOut("elem[i].obj: %lx\n", elem[i].obj);

  if(elem[i].windows_type==EDITTEXT && !(elem[i].flags & ES_READONLY)) {
    DebOut("call MUIA_String_Contents, %s", lpString);
    DoMethod(elem[i].obj, MUIM_Set, MUIA_String_Contents, lpString);
  }
  else if (elem[i].windows_type==COMBOBOX) {
    DebOut("COMBOBOX: call SendDlgItemMessage instead:\n");
    return SendDlgItemMessage(elem, item, CB_ADDSTRING, 0, (LPARAM) lpString);
  }
  else {
    DebOut("call MUIA_Text_Contents, %s", lpString);
    DoMethod(elem[i].obj, MUIM_Set, MUIA_Text_Contents,   lpString);
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


  DoMethod(elem[i].obj, MUIM_NoNotifySet, MUIA_Selected, uCheck);
  //SET(elem[i].obj, MUIA_Pressed, uCheck);

  return TRUE;

}

LONG SendDlgItemMessage(struct Element *elem, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam) {
  Object *obj;
  LONG i;
  ULONG l;
  IPTR activate;
  TCHAR *buf;

  DebOut("elem: %lx, nIDDlgItem: %d, lParam: %lx\n", elem, nIDDlgItem, lParam);

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
    {
      ULONG old_active;
      /* add string to popup window */
      DebOut("CB_ADDSTRING (%s)\n", (TCHAR *) lParam);

      /* remember old selection */
      GetAttr(MUIA_Cycle_Active, elem[i].obj, (IPTR *) &old_active);

      l=0;
      //DebOut("old list:\n");
      while(elem[i].var[l] != NULL) {
        //DebOut("  elem[i].var[%d]: %s\n", l, elem[i].var[l]);
        l++;
      }
      /* found next free string */
      //DebOut("  next free line: %d\n", l);
      activate=l;
#warning TODO: free strings again!
      elem[i].var[l]=strdup((TCHAR *) lParam);
      elem[i].var[l+1]=NULL;
      //DebOut("elem[i].obj: %lx\n", elem[i].obj);
      DoMethod(elem[i].obj, MUIM_Set, MUIA_Cycle_Entries, (IPTR) elem[i].mem);
      //DebOut("new list:\n");
      l=0;
      while(elem[i].var[l] != NULL) {
        //DebOut("  elem[i].var[%d]: %s\n", l, elem[i].var[l]);
        l++;
      }
      DoMethod(elem[i].obj, MUIM_Set, MUIA_Cycle_Active, old_active);
    }
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
      DoMethod(elem[i].obj, MUIM_NoNotifySet, MUIA_Cycle_Entries, (IPTR) elem[i].mem);
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
    case CB_GETLBTEXT:
      DebOut("[%lx] WM_GETTEXT / CB_GETLBTEXT\n", elem[i].obj);
      {
        TCHAR *string=(TCHAR *) lParam;
        DebOut("[%lx] elem[i].value: %d\n", elem[i].obj, elem[i].value);
        DebOut("[%lx] wParam: %d\n", elem[i].obj, wParam);
        /* warning: care for non-comboboxes, too! */
        if(elem[i].value<0 || elem[i].var[elem[i].value]==NULL) {
          string[0]=(char) 0;
        }
        else {
          DebOut("return: %s\n", elem[i].var[elem[i].value]);
          if(Msg == CB_GETLBTEXT) {
            DebOut("CB_GETLBTEXT\n");
            /* CB_GETLBTEXT has no range check! */
            strcpy(string, elem[i].var[elem[i].value]);
          }
          else {
            strncpy(string, elem[i].var[elem[i].value], wParam);
          }
        }
      }
      break;
    case WM_SETTEXT:
      DebOut("WM_SETTEXT\n");
      if(lParam==0 || strlen((TCHAR *)lParam)==0) {
        DebOut("lParam is empty\n");
        DebOut("elem[i].obj: %lx\n", elem[i].obj);
        DoMethod(elem[i].obj, MUIM_Set, MUIA_Cycle_Active, 1);
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
          DoMethod(elem[i].obj, MUIM_Set, MUIA_Cycle_Active, l);
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
      DoMethod(elem[i].obj, MUIM_Set, MUIA_Cycle_Entries, (IPTR) elem[i].mem);
      if(flag_editable(elem[i].flags)) {
        DoMethod(elem[i].obj, MUIM_Set, MUIA_Cycle_Active, 1);
      }
      else {
        DoMethod(elem[i].obj, MUIM_Set, MUIA_Cycle_Active, 0);
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
    case CB_SETCURSEL: {
      LONG foo;
#if 0
      LONG empty_idx=-1;
#endif
      DebOut("CB_SETCURSEL\n");
      DebOut("wParam: %d\n", wParam);
      foo=wParam;
      if(flag_editable(elem[i].flags)) {
        foo++;
      }
#if 0
      if(foo<0) {
        /* clearing is not so simple, our field is not editable at all,
         * so we add a empty selection, if there is not already one.
         */

        /* search for empty selection, count entries in l */
        l=0;
        while(elem[i].var[l] != NULL) {
          DebOut("  compare with >%s<\n", elem[i].var[l]);
          if(!strcmp(elem[i].var[l], (TCHAR *) EMPTY_SELECTION)) {
            empty_idx=l;
          }
          l++;
        }
        if(empty_idx==-1) {
          /* add EMPTY_SELECTION */
          elem[i].var[l]=strdup((TCHAR *) EMPTY_SELECTION);
          /* terminate list */
          elem[i].var[l+1]=NULL;
          DoMethod(elem[i].obj, MUIM_Set, MUIA_Cycle_Entries, (IPTR) elem[i].mem);
          empty_idx=l;
        }
        /* select empty index */
        DoMethod(elem[i].obj, MUIM_Set, MUIA_Cycle_Active, empty_idx);

      }
      else {
#endif
        DoMethod(elem[i].obj, MUIM_Set, MUIA_Cycle_Active, foo);
#if 0
      }
#endif
      return TRUE;
      } /* case block */
      break;

    case TBM_SETRANGE:
      /* lParam: The LOWORD specifies the minimum position for the slider, 
       *         and the HIWORD specifies the maximum position.
       */
       DebOut("TBM_SETRANGE %lx (%d, %d)\n", lParam, LOWORD(lParam), HIWORD(lParam));
       DoMethod(elem[i].obj, MUIM_Set, MUIA_Numeric_Min, LOWORD(lParam));
       DoMethod(elem[i].obj, MUIM_Set, MUIA_Numeric_Max, HIWORD(lParam));
       break;
    case TBM_SETPAGESIZE:
      DebOut("WARNING: TBM_SETPAGESIZE seems not to be possible in Zune/MUI !?\n");
      break;
    case TBM_SETPOS:
       DebOut("TBM_SETPOS(%d)\n", lParam);
       DoMethod(elem[i].obj, MUIM_NoNotifySet, MUIA_Numeric_Value, lParam);
      break;
    case TBM_GETPOS:
        LONG res;
        res=XGET(elem[i].obj, MUIA_Numeric_Value);
        DebOut("TBM_GETPOS: %d\n", res);
        return res;
      break;
    default:
      DebOut("WARNING: unkown Windows Message-ID: %d\n", Msg);
      return FALSE;
  }

  /* never reach this */
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
    elem[i].var=(char **) malloc(16 * sizeof(IPTR)); 
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
    DoMethod(elem[i].obj, MUIM_Set, MUIA_String_Contents, elem[i].var[0]);
  }
  else {
    DebOut("elem[i].windows_type!=EDITTEXT\n");
    DoMethod(elem[i].obj, MUIM_Set, MUIA_Text_Contents, elem[i].var[0]);
  }

  return TRUE;
}

/* Adds a check mark to (checks) a specified radio button in a group and removes 
 * a check mark from (clears) all other radio buttons in the group. 
 */
BOOL CheckRadioButton(HWND elem, int nIDFirstButton, int nIDLastButton, int nIDCheckButton) {
  int i=-666;
  int e=-666;
  int set=-666;
  IPTR act;

  DebOut("0x%lx, %d, %d, %d\n", elem, nIDFirstButton, nIDLastButton, nIDCheckButton);

  set=get_index(elem, nIDCheckButton);
  /* might be in a different element..*/
  if(set<0) {
    elem=get_elem(nIDCheckButton);
    set=get_index(elem, nIDCheckButton);
  }
  /* still not found !? */
  if(set<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", i);
    return FALSE;
  }

  if(elem[set].flags2==BS_AUTORADIOBUTTON) {
    if(elem[set].value) {
      DebOut("Button was already active, nothing to do here.\n");
      return TRUE;
    }

    /* 
     * we are an autoradio button. only one will be active anyways, so just "click" on me,
     * notification should reach the right button.
     */
    DoMethod(elem[set].obj, MUIM_Set, MUIA_Selected, TRUE);
    return TRUE;
  }

  DebOut("WARNING: CheckRadioButton called for a non-BS_AUTORADIOBUTTON !?\n");

#if 0
  /* this is not tested..: */
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
      act=XGET(elem[e].obj, MUIA_Selected);

      if( (set==e) != act) {
        DoMethod(elem[e].obj, MUIM_Set, MUIA_Selected, set==e);
        if(elem[e].text) {
          DebOut("set elem[%d] (%s) to %d\n", e, elem[e].text, act);
        }
        else {
          DebOut("set elem[%d] to %d\n", e, act);
        }

      }
    }
    i++;
  }
#endif

  return FALSE;
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
  if(elem[i].text) {
    DebOut("elem[i].text: %s\n", elem[i].text);
  }

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


extern Object *reset;
/* WARNING: not same API call as in Windows */
BOOL EnableWindow(HWND hWnd, DWORD id, BOOL bEnable) {
  int i;
  int res;

  DebOut("elem: %lx, id %d\n", hWnd, id);

  if(id==IDC_RESETAMIGA) {
    /* special case, IDC_RESETAMIGA is handcoded and now Windows button */
    DoMethod(reset, MUIM_Set, MUIA_Disabled, !bEnable);

    return TRUE;
  }

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
  DebOut("SET(hWnd[%d].obj, MUIA_Disabled, %d, TAG_DONE);\n", i, !bEnable);

  DebOut("hWnd[i].obj: %lx\n", hWnd[i].obj);
  DoMethod(hWnd[i].obj, MUIM_Set, MUIA_Disabled, !bEnable);

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

/* WARNING: not same API call as in Windows */
BOOL ShowWindow(HWND hWnd, DWORD id, int nCmdShow) {
  int i;

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

  if(nCmdShow==SW_SHOW) {
    DoMethod(hWnd[i].obj, MUIM_Set, MUIA_ShowMe, TRUE);
  }
  else {
    DoMethod(hWnd[i].obj, MUIM_Set, MUIA_ShowMe, FALSE);
  }

  return TRUE;
}

 

AROS_UFH2(void, MUIHook_combo, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  int i;
  ULONG wParam;

  struct Data *data = (struct Data *) hook->h_Data;


  //DebOut("[%lx] entered\n", obj);
  //DebOut("[%lx] hook.h_Data: %lx\n", obj, hook->h_Data);
  //DebOut("[%lx] obj: %lx\n", obj);

  i=get_elem_from_obj(data, (Object *) obj);

  //DebOut("[%lx] i: %d\n", obj, i);

  data->src[i].value=XGET((Object *) obj, MUIA_Cycle_Active);
  //DebOut("[%lx] MUIA_Cycle_Active: %d (mui obj: %lx)\n", obj, data->src[i].value, obj);

  DebOut("[%lx] state: %d\n", obj, data->src[i].value);

  if(data->func) {
    if(data->src[i].mem[data->src[i].value] && !strcmp(data->src[i].mem[data->src[i].value], EMPTY_SELECTION)) {
      DebOut("[%lx] Empty selection (%s), do nothing\n", obj, EMPTY_SELECTION);
    }
    else {
      if(flag_editable(data->src[i].flags)) {
        data->src[i].value--;
      }
      wParam=MAKELPARAM(data->src[i].idc, CBN_SELCHANGE);
      DebOut("[%lx] call function: %lx(IDC %d, CBN_SELCHANGE)\n", obj, data->func, data->src[i].idc);
      data->func(data->src, WM_COMMAND, wParam, NULL);
    }
  }
  else {
    DebOut("[%lx] function is zero: %lx\n", obj, data->func);
  }

  AROS_USERFUNC_EXIT
}

AROS_UFH2(void, MUIHook_slide, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  struct Data *data = (struct Data *) hook->h_Data;
  int i;
  ULONG wParam;

  DebOut("Sliding..\n");

  i=get_elem_from_obj(data, (Object *) obj);

  if(data->func) {
#if 0
    wParam=MAKELPARAM(data->src[i].idc, CBN_SELCHANGE);
    DebOut("call function: %lx (IDC %d, wParam: %lx)\n", data->func, data->src[i].idc, wParam);
    data->func(data->src, WM_COMMAND, wParam, NULL);
#endif

    /* warning: should we call BN_CLICKED here, too?
     * open console in Paths won't work without .. 
     */
    wParam=MAKELPARAM(data->src[i].idc, BN_CLICKED);
    data->func(data->src, WM_COMMAND, wParam, NULL);
  }
  else {
    DebOut("WARNING: function is zero: %lx\n", data->func);
    /* Solution: add DlgProc in mNew in mui_head.cpp */
  }

  AROS_USERFUNC_EXIT
}

/*
 * Which control sends which default event on click, soo
 * https://msdn.microsoft.com/en-us/library/9y0at277.aspx
 */

AROS_UFH2(void, MUIHook_select, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  int i;
  ULONG t;
  ULONG wParam;
  ULONG newstate;

  struct Data *data = (struct Data *) hook->h_Data;

  DebOut("entered (hook.h_Data: %lx)\n", hook->h_Data);

  i=get_elem_from_obj(data, (Object *) obj);

  if(data->src[i].text) {
    DebOut("obj: %lx => i: %d (%s)\n", obj, i, data->src[i].text);
  }
  else {
    DebOut("obj: %lx => i: %d\n", obj, i);
  }

  newstate=XGET((Object *) obj, MUIA_Selected);
  DebOut("MUIA_Selected is (now): %d (group: %d)\n", newstate, data->src[i].group);

  if((data->src[i].windows_type==CONTROL) && 
     (data->src[i].flags2==BS_AUTORADIOBUTTON) && 
     (data->src[i].group)) {

    /*** AUTORADIOBUTTON / Radio button logic: ***/

    if(data->src[i].value == 1) {
      /* we were already the active one, we want to stay that, too! */
      DebOut("we were already the active one!\n");
      DoMethod(data->src[i].obj, MUIM_NoNotifySet, MUIA_Selected, TRUE);
      /* no event */
      goto DONE;
    }

    if(newstate == 0) {
      /* this is impossible!*/
      DebOut("ERROR: this is impossible\n");
    }

    DebOut("deselect all my brothers\n");
    /* as we are part of a group, unselect all our active friends */
    t=0;
    while(data->src[t].exists) {
      if(data->src[t].value && data->src[i].group == data->src[t].group && t!=i) {
        /* same group */
        DebOut("  => also activated, same group, other guy!\n");
        DoMethod(data->src[t].obj, MUIM_NoNotifySet, MUIA_Selected, FALSE); 
        data->src[t].value=0;
      }
      t++;
    }
  }
  else if(data->src[i].windows_type==COMBOBOX) {

    /*** COMBOBOX ***/

    /* CBN_SELCHANGE: A new combo box list item is selected. */
    if(data->func) {
      wParam=MAKELPARAM(data->src[i].idc, CBN_SELCHANGE);
      DebOut("call function: %lx (IDC %d, wParam: %lx)\n", data->func, data->src[i].idc, wParam);
      data->func(data->src, WM_COMMAND, wParam, NULL);
    }
    goto DONE;
  }
  data->src[i].value=newstate;


  if(data->func) {
    /* warning: should we call BN_CLICKED here, too?
     * open console in Paths won't work without .. 
     */
    wParam=MAKELPARAM(data->src[i].idc, BN_CLICKED);
    data->func(data->src, WM_COMMAND, wParam, NULL);
  }
  else {
    DebOut("WARNING: function is zero: %lx\n", data->func);
    /* Solution: add DlgProc in mNew in mui_head.cpp */
  }




DONE:
  ;
  AROS_USERFUNC_EXIT
}

AROS_UFH2(void, MUIHook_pushbutton, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT
  int i;
  ULONG wParam;

  struct Data *data = (struct Data *) hook->h_Data;

  DebOut("[%lx] entered\n", obj);

  i=get_elem_from_obj(data, (Object *) obj);

  DebOut("i: %d\n", i);

  if(data->func) {
    DebOut("call function: %lx\n", data->func);
    DebOut("IDC: %d\n", data->src[i].idc);
    DebOut("WM_COMMAND: %d\n", WM_COMMAND);

    wParam=MAKELPARAM(data->src[i].idc, BN_CLICKED);
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

#ifdef UAE_ABI_v0
#define ABI_CONST const
#else
#define ABI_CONST 
#endif
static IPTR mNew(struct IClass *cl, APTR obj, Msg msg) {

  struct TagItem *tstate, *tag;
  ULONG i=0;
  IPTR s=0;
  struct Element *src=NULL;
  Object *child      =NULL; /* TODO: remove me */
  int *(*func) (Element *hDlg, UINT msg, ULONG wParam, IPTR lParam);
  func=NULL;

  DebOut("mNew(fixed)\n");

  obj = (APTR) DoSuperMethodA(cl, (Object*) obj, msg);

  if (!obj) {
    DebOut("fixed: unable to create object!");
    return (IPTR) NULL;
  }

  tstate=((struct opSet *)msg)->ops_AttrList;

  while ((tag = (struct TagItem *) NextTagItem((ABI_CONST TagItem**) &tstate))) {
    DebOut("tag->ti_Tag: %lx\n", tag->ti_Tag);
    switch (tag->ti_Tag) {
      case MA_src:
        s = tag->ti_Data;
        break;
      case MA_dlgproc:
        DebOut("MA_dlgproc: %lx\n", tag->ti_Data);
        func=(int* (*)(Element*, uint32_t, ULONG, IPTR)) tag->ti_Data;
        break;
    }
  }

  if(!s) {
    DebOut("mNew: MA_src not supplied!\n");
    return (IPTR) NULL;
  }

  /* place vertically */
  DoMethod(obj, MUIM_Set, MUIA_Group_Horiz, FALSE);

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
    foo=TextObject, MUIA_Text_Contents, (IPTR) "XYZ", End;
    old=(struct TextFont *) XGET(foo, MUIA_Font);
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
          if(!strcmp(src[i].windows_class, "msctls_trackbar32")) {
            /* Proportional/slider gadget! */
            child=VGroup,
                    Child, VSpace(0),
                    Child, SliderObject,
                      MUIA_Group_Horiz, TRUE,
                      MUIA_Slider_Quiet, TRUE,
#if 0
                      MUIA_Numeric_Min, 0,
                      MUIA_Numeric_Max, 10,
                      MUIA_Numeric_Value, 0,
#endif
                      MUIA_Weight, 0,
                    End,
                    Child, VSpace(0),
                  End;

            if(child) {
              src[i].exists=TRUE;
              src[i].obj=child;
#ifdef UAE_ABI_v0
              data->MyMUIHook_slide.h_Entry=(HOOKFUNC) MUIHook_slide;
#else
              data->MyMUIHook_slide.h_Entry=(APTR) MUIHook_slide;
#endif
              data->MyMUIHook_slide.h_Data =(APTR) data;

              DebOut("DoMethod(%lx, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime..)\n", src[i].obj);
              DoMethod(src[i].obj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, (IPTR) src[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_slide, func); 
 
            }
            break;
          }
          if(src[i].windows_class==NULL || !strcmp(src[i].windows_class, "Button") || /* implement those: !!!  */ !strcmp(src[i].windows_class, "SysListView32") || !strcmp(src[i].windows_class, "Static") || !strcmp(src[i].windows_class, "SysTreeView32")) {
            DebOut("Button found\n");
            if(src[i].flags2==BS_AUTORADIOBUTTON) {
              child=HGroup,
                    MUIA_UserData         , i,
                    Child, src[i].obj=ImageObject,
                        NoFrame,
                        //MUIA_CycleChain       , 1,
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

#ifdef UAE_ABI_v0
              data->MyMUIHook_select.h_Entry=(HOOKFUNC) MUIHook_select;
#else
              data->MyMUIHook_select.h_Entry=(APTR) MUIHook_select;
#endif
              data->MyMUIHook_select.h_Data =(APTR) data;

              DoMethod(src[i].obj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, (IPTR) src[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_select, func); 

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
                    //MUIA_CycleChain  , TRUE,
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
                /* button without text must be without text, otherwise zune draws "over the border" */
                child=HGroup,
                  MUIA_UserData         , i,
                  Child, src[i].obj=ImageObject,
                    ImageButtonFrame,
                    MUIA_InputMode   , MUIV_InputMode_Toggle,
                    MUIA_Image_Spec  , MUII_CheckMark,
                    MUIA_Background  , MUII_ButtonBack,
                    MUIA_ShowSelState, FALSE,
                    //MUIA_CycleChain  , TRUE,
                    MUIA_UserData         , i,
                  End,
                End;
              }
#ifdef UAE_ABI_v0
              data->MyMUIHook_select.h_Entry=(HOOKFUNC) MUIHook_select;
#else
              data->MyMUIHook_select.h_Entry=(APTR) MUIHook_select;
#endif
              data->MyMUIHook_select.h_Data =(APTR) data;

              DebOut("DoMethod(%lx, MUIM_Notify, MUIA_Selected, MUIV_EveryTime..)\n", src[i].obj);
              DoMethod(src[i].obj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, (IPTR) src[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_select, func); 
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
          child=HGroup, MUIA_Background, MUII_ButtonBack,
                              ButtonFrame,
                              MUIA_InputMode , MUIV_InputMode_RelVerify,
                              Child, TextObject,
                                MUIA_Font, Topaz8Font,
                                MUIA_Text_PreParse, "\33c",
                                MUIA_Text_Contents, (IPTR) src[i].text,
                              End,
                            End;

          src[i].exists=TRUE;
          src[i].obj=child;
          /* Add hook */
#ifdef UAE_ABI_v0
          data->MyMUIHook_pushbutton.h_Entry=(HOOKFUNC) MUIHook_pushbutton;
#else
          data->MyMUIHook_pushbutton.h_Entry=(APTR) MUIHook_pushbutton;
#endif
          data->MyMUIHook_pushbutton.h_Data =(APTR) data;
          DoMethod(src[i].obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) src[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_pushbutton, func); 
        break;

        case RTEXT:
          child=TextObject,
                        //MUIA_Background, MUII_MARKBACKGROUND,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33r",
                        MUIA_Text_Contents, (IPTR) src[i].text,
                      End;
          src[i].exists=TRUE;
          src[i].obj=child;
        break;

        case LTEXT:
          child=TextObject,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33l",
                        MUIA_Text_Contents, (IPTR) src[i].text,
                      End;
          src[i].exists=TRUE;
          src[i].obj=child;
        break;


        case EDITTEXT:
          if(src[i].flags & ES_READONLY) {
            child=TextObject,
                        TextFrame,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33c",
                        MUIA_Text_Contents, (IPTR) src[i].text,
                      End;
            src[i].obj=child;
          }
          else {
            child=StringObject,
                          StringFrame,
                          MUIA_Font, Topaz8Font,
                          MUIA_String_Contents, (IPTR) src[i].text,
                        End;
            src[i].obj=child;
          }
          src[i].exists=TRUE;
          src[i].var=(char **) malloc(256 * sizeof(IPTR)); // array for cycle texts
          src[i].var[0]=NULL;
        break;


        case COMBOBOX:
          src[i].mem=(char **) malloc(256 * sizeof(IPTR)); // array for cycle texts
          DebOut("flags: %lx\n", src[i].flags);
          if(!flag_editable(src[i].flags)) {
            DebOut("flags: CBS_DROPDOWNLIST\n");
            src[i].var=src[i].mem;
          }
          else {
            /* must contain "<<empty>>" statement */
            DebOut("flags: CBS_DROPDOWN\n");
            src[i].mem[0]=strdup(EMPTY_SELECTION);
            src[i].var=src[i].mem+1;
          }
          src[i].var[0]=NULL;
          src[i].exists=TRUE;
          child=CycleObject,
                       MUIA_Font, Topaz8Font,
                       MUIA_Cycle_Entries, src[i].mem,
                     End;
          src[i].obj=child;
#ifdef UAE_ABI_v0
          data->MyMUIHook_combo.h_Entry=(HOOKFUNC) MUIHook_combo;
#else
          data->MyMUIHook_combo.h_Entry=(APTR) MUIHook_combo;
#endif
          data->MyMUIHook_combo.h_Data =(APTR) data;
          DoMethod(src[i].obj, MUIM_Notify, MUIA_Cycle_Active , MUIV_EveryTime, (IPTR) src[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_combo, func); 
        break;


        default:
          DebOut("ERROR: unknown windows_type %d\n", src[i].windows_type);
      }
      DebOut("RESULT: child:   %lx\n", child);
      DebOut("RESULT: src.obj: %lx\n", src[i].obj);
      if(child) {
        if(src[i].help) {
          DebOut("add HOTHELP: %s\n", src[i].help);
          DoMethod(child, MUIM_Set, MUIA_ShortHelp, (IPTR) src[i].help);
        }
        if(!(src[i].flags & WS_VISIBLE)) {
          DoMethod(child, MUIM_Set, MUIA_ShowMe, (IPTR) 0);
        }
        if(src[i].flags & WS_TABSTOP) {
          DoMethod(child, MUIM_Set, MUIA_CycleChain, (IPTR) 1);
        }
        child=NULL;
      }
      DebOut("  src[%d].obj=%lx\n", i, src[i].obj);
      i++;

    }

    //SETUPHOOK(&data->LayoutHook, LayoutHook, data);
#ifdef UAE_ABI_v0
    data->LayoutHook.h_Entry=(HOOKFUNC) LayoutHook;
#else
    data->LayoutHook.h_Entry=(APTR) LayoutHook;
#endif
    data->LayoutHook.h_Data=data;

    /*
     * ATTENTION: Add children *after* seting the LayoutHook,
     *            otherwise we get recursions!
     */
  
    DoMethod(obj, MUIM_Set, MUIA_Group_LayoutHook, &data->LayoutHook);
    i=0;
    while(src[i].exists) {
      DebOut("i: %d (add %lx to %lx)\n", i, src[i].obj, obj);
      DoMethod((Object *) obj, OM_ADDMEMBER,(IPTR) src[i].obj);
      i++;
    }

    mSet(data, obj, (struct opSet *) msg, 1);
    DoMethod(obj, MUIM_Set, MUIA_Frame, MUIV_Frame_Group);
  }
  return (IPTR)obj;
}

static IPTR mGet(struct Data *data, APTR obj, struct opGet *msg, struct IClass *cl)
{
  IPTR rc;

  //DebOut("mGet..\n");

  switch (msg->opg_AttrID)
  {
    case MA_Data: 
      DebOut("data: %lx\n", data);
      rc = (IPTR) data; 
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

  while ((tag = NextTagItem((ABI_CONST TagItem**)&tstate))) {

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


