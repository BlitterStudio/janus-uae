/*****************************************************************************
 * WinAPI
 *****************************************************************************/

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
#include <mui/NListtree_mcc.h>
#include <mui/NListview_mcc.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "aros.h"
#include "gui.h"
#include "winnt.h"
#include "mui_data.h"

BOOL flag_editable(ULONG flags);

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
    DebOut("call set(%lx, MUIA_String_Contents, %s)\n", elem[i].obj,lpString);
    DoMethod(elem[i].obj, MUIM_Set, MUIA_String_Contents, lpString);
  }
  else if (elem[i].windows_type==COMBOBOX) {
    DebOut("COMBOBOX: call SendDlgItemMessage instead:\n");
    return SendDlgItemMessage(elem, item, CB_ADDSTRING, 0, (LPARAM) lpString);
  }
  else {
    DebOut("call MUIA_Text_Contents, %s\n", lpString);
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

  /* still not found !? */
  if(i<0) {
    DebOut("ERROR: button %d found nowhere!?\n", button);
    return FALSE;
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
      /* add string  */
      DebOut("CB_ADDSTRING (%s)\n", (TCHAR *) lParam);

      if(!flag_editable(elem[i].flags)) {
        /* remember old selection */
        DebOut("not editable\n");
        GetAttr(MUIA_Cycle_Active, elem[i].obj, (IPTR *) &old_active);
      }

      DebOut("elem[i].mem: %lx\n", elem[i].mem);

      l=0;
      DebOut("old list:\n");
      while(elem[i].mem[l] != NULL) {
        DebOut("  elem[i].mem[%d]: %s\n", l, elem[i].mem[l]);
        l++;
      }
      /* found next free string */
      activate=l;
#warning TODO: free strings again!
      elem[i].mem[l]=strdup((TCHAR *) lParam);
      elem[i].mem[l+1]=NULL;

#if 0
      l=0;
      DebOut("new list:\n");
      while(elem[i].mem[l] != NULL) {
        DebOut("  elem[i].mem[%d]: %s\n", l, elem[i].mem[l]);
        l++;
      }
#endif
 
      if(flag_editable(elem[i].flags)) {
        DoMethod(elem[i].obj, MUIM_List_Insert, (IPTR) elem[i].mem);
        DoMethod(elem[i].obj, MUIM_NoNotifySet, MUIA_String_Contents, (IPTR) lParam);
      }
      else {
        DoMethod(elem[i].obj, MUIM_Set, MUIA_Cycle_Entries, (IPTR) elem[i].mem);
      }

      l=0;
      while(elem[i].mem[l] != NULL) {
        DebOut("  CB_ADDSTRING: elem[%d].mem[%d]: %s\n", i, l, elem[i].mem[l]);
        l++;
      }
      if(!flag_editable(elem[i].flags)) {
        DoMethod(elem[i].obj, MUIM_NoNotifySet, MUIA_Cycle_Active, old_active);
      }
    }
    break;

    case CB_RESETCONTENT:
      /* delete all strings */
      DebOut("CB_RESETCONTENT (elem: %d)\n", i);
      l=0;
      /* free old strings */
      while(elem[i].mem[l] != NULL) {
#warning free them!
        //free(elem[i].mem[l]);
        elem[i].mem[l]=NULL;
        l++;
      }
      //elem[i].var[0]=strdup("<empty>");
      //elem[i].var[1]=NULL;
      elem[i].mem[0]=NULL;
      DebOut("elem[i].obj: %lx\n", elem[i].obj);
      DoMethod(elem[i].obj, MUIM_NoNotifySet, MUIA_List_Entries, (IPTR) elem[i].mem);
      break;
    case CB_FINDSTRING:
      /* return string index */
      DebOut("CB_FINDSTRING\n");
      DebOut("search for string >%s<\n", (TCHAR *) lParam);
      l=0;
      while(elem[i].mem[l] != NULL) {
        DebOut("  compare with >%s<\n", elem[i].mem[l]);
        if(!strcmp(elem[i].mem[l], (TCHAR *) lParam)) {
          return l;
        }
        l++;
      }
      DebOut("return -1\n");
      return -1;

    case CB_GETCURSEL:
      DebOut("CB_GETCURSEL (return elem[i].value: %d)\n", elem[i].value);
      return elem[i].value;
      break;
    case WM_GETTEXT:
    case CB_GETLBTEXT:
      DebOut("[%lx] WM_GETTEXT / CB_GETLBTEXT\n", elem[i].obj);
      {
        TCHAR *string=(TCHAR *) lParam;
        DebOut("[%lx] elem[i].value: %d\n", elem[i].obj, elem[i].value);
        DebOut("[%lx] wParam: %d\n", elem[i].obj, wParam);
        if(elem[i].windows_type==COMBOBOX) {
          if(elem[i].value<0 || elem[i].mem[elem[i].value]==NULL) {
            string[0]=(char) 0;
          }
          else {
            DebOut("return: %s\n", elem[i].mem[elem[i].value]);
            if(Msg == CB_GETLBTEXT) {
              DebOut("CB_GETLBTEXT\n");
              /* CB_GETLBTEXT has no range check! */
              strcpy(string, elem[i].mem[elem[i].value]);
            }
            else {
              strncpy(string, elem[i].mem[elem[i].value], wParam);
            }
          }
        }
        else if(elem[i].windows_type==EDITTEXT) {
          GetAttr(MUIA_String_Contents, elem[i].obj, (IPTR *) &string);
        }
        else {
          TODO();
        }
      }
      break;
    case WM_SETTEXT: {
      LONG found=-1;

      DebOut("WM_SETTEXT\n");

      if(lParam==0 || strlen((TCHAR *)lParam)==0) {
        DebOut("lParam is empty!\n");
        return 0;
      }

      if(elem[i].windows_type==EDITTEXT) {
        /* no combobox or similiar! */
        DebOut("set EDITTEXT to %s\n", lParam);
        SetAttrs(elem[i].obj, MUIA_String_Contents, lParam, TAG_DONE);
        return 1;
      }

      if(elem[i].windows_type!=COMBOBOX) {
        DebOut("ERROR: unknown type %d for WM_SETTEXT!!\n", elem[i].windows_type);
        return CB_ERR;
      }

      if(!flag_editable(elem[i].flags)) {
        DebOut("WM_SETTEXT for !editable!\n");
        /* "It is CB_ERR if this message is sent to a combo box without an edit control." */
        /* REALLY !? */
        return CB_ERR;
      }

      DebOut("i: %d\n", i);
      DebOut("elem[i]: %lx\n", elem[i]);
      DebOut("elem[i].mem[0]: %lx\n", elem[i].mem[0]);

      l=0;
      while(elem[i].mem[l] != NULL) {
        DebOut("  compare with >%s<\n", elem[i].mem[l]);
        DebOut("  l: %d\n", l);
        if(!strcmp(elem[i].mem[l], (TCHAR *) lParam)) {
          found=l;
          DebOut("we already have that string at position %d!\n", found);
          //DebOut("activate string %d in combo box!\n", l);
          //DebOut("elem[i].obj: %lx\n", elem[i].obj);
          //SetAttrs(elem[i].obj, MUIA_String_Contents, lParam, TAG_DONE);
          break;
        }
        l++;
      }

      DebOut("found: %d\n", found);
 
      if(flag_editable(elem[i].flags)) {
        /* custom combo box */
        if(found > -1) {
          DoMethod(elem[i].obj, MUIM_NoNotifySet, MUIA_String_Contents, lParam);
          /* we are done! */
          return 1;
        }
      }
      DebOut("new string, add it to top\n");
      l=0;
      buf=elem[i].mem[l+1];
      while(buf != NULL) {
        elem[i].mem[l+1]=elem[i].mem[l];
        l++;
        buf=elem[i].mem[l+1];
      }
      /* store new string */
      elem[i].mem[0]=strdup((TCHAR *) lParam);
      /* terminate list */
      elem[i].mem[l+1]=NULL;

      if(flag_editable(elem[i].flags)) {
        DoMethod(elem[i].obj, MUIM_List_Insert, elem[i].mem);
      }
      else {
        DoMethod(elem[i].obj, MUIM_Set,         MUIA_Cycle_Entries, (IPTR) elem[i].mem);
        DoMethod(elem[i].obj, MUIM_NoNotifySet, MUIA_Cycle_Active, 0);
      }
 
      return TRUE;
    }
      
    /* An application sends a CB_SETCURSEL message to select a string in the 
     * list of a combo box. If necessary, the list scrolls the string into view. 
     * The text in the edit control of the combo box changes to reflect the new 
     * selection, and any previous selection in the list is removed. 
     *
     * wParam: Specifies the zero-based index of the string to select. 
     * If this parameter is -1, any current selection in the list is removed and 
     * the edit control is cleared.
     *
     * If the message is successful, the return value is the index of the item 
     * selected. If wParam is greater than the number of items in the list or 
     * if wParam is -1, the return value is CB_ERR and the selection is cleared. 
     */
    case CB_SETCURSEL: {
      DebOut("CB_SETCURSEL: elem %d wParam: %d\n", i, wParam);

      if(flag_editable(elem[i].flags)) {
        /* zune combo object */
        if(wParam==-1) {
          DebOut("clear string gadget\n");
          DoMethod(elem[i].obj, MUIM_NoNotifySet, MUIA_String_Contents, (IPTR) "");
          return CB_ERR;
        }
        else {
          /* check, if wParam is valid!? */
          DebOut("activate string: %s\n", elem[i].mem[wParam]);
          DoMethod(elem[i].obj, MUIM_NoNotifySet, MUIA_String_Contents, (IPTR) elem[i].mem[wParam]);
        }
      }
      else {
        /* cycle gadget */
        if(wParam >= 0) {
          DoMethod(elem[i].obj, MUIM_NoNotifySet, MUIA_Cycle_Active, wParam);
        }
        else {
          DebOut("ERROR: what to do with a -1 here !?\n");
          return CB_ERR;
        }
      }

      return wParam;
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
  char *buffer; 

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
  DebOut("elem[%d].obj: %lx\n", i, elem[i].obj);

  if(elem[i].windows_type==EDITTEXT) {
    GetAttr(MUIA_String_Contents, elem[i].obj, (IPTR *) &buffer);
    DebOut("buffer: %s\n", buffer);
    strncpy(lpString, buffer, nMaxCount);
  }
  else {
    DebOut("Warning: elem[i].windows_type=%d, GetText..?\n", elem[i].windows_type);
    DebOut("return elem[i].text: %s\n", elem[i].text);
    strncpy(lpString, elem[i].text, nMaxCount);
  }
 
  DebOut("lpString: %s\n", lpString);

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

#endif

/* Changes the text of the specified window's title bar (if it has one). 
 * If the specified window is a control, the text of the control is changed.
 */
BOOL SetWindowText(HWND hWnd, TCHAR *lpString) {

  if(hWnd!=NULL) {
    DebOut("hWnd: %lx\n", hWnd);
    TODO();
    return FALSE;
  }
  DebOut("lpString: %s\n", lpString);
  SetAttrs(win, MUIA_Window_Title, lpString, TAG_DONE);
}

int GetWindowText(HWND   hWnd, LPTSTR lpString, int nMaxCount) {
  DebOut("hWnd: %lx\n", hWnd);

  if(hWnd!=NULL) {
    DebOut("hWnd: %lx\n", hWnd);
    TODO();
    return 0;
  }

  /* pray for enough space!! */
  GetAttr(MUIA_String_Contents, win, (IPTR *) lpString);

  DebOut("lpString: %s\n", lpString);

}

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

