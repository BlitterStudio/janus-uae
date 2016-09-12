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
#ifndef DONT_USE_URLOPEN
#include <mui/Urltext_mcc.h>
#endif

#define JUAE_DEBUG
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
#if 0
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
  DebOut("elem: 0x%p, i: %d\n", elem, i);

#if 0
  /* yes, I know, this is ugly ..
   * accessing objects data from outside
   */
  foo=elem->obj;
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
      res=XGET(elem->obj, MUIA_Numeric_Default);
      DebOut("result: %d\n", res);
      return res;

    default:
      TODO();
  }

  return FALSE;
}
#endif

BOOL SetDlgItemText(HWND hDlg, int item, const TCHAR *lpString) {
  //LONG i;
  TCHAR empty[1];
  Element *elem;

  if(!lpString) {
    empty[0]=(char) 0;
    lpString=empty;
  }

  DebOut("hDlg: %p\n", hDlg);
  DebOut("item: %d\n", item);
  DebOut("string: %s\n", lpString);

  elem=get_control(hDlg, item);
  DebOut("elem: %p\n", elem);

  if (elem)
  {
      DebOut("elem->obj: 0x%p\n", elem->obj);

      if(elem->windows_type==EDITTEXT && !(elem->flags & ES_READONLY)) {
        DebOut("call set(0x%p, MUIA_String_Contents, %s)\n", elem->obj,lpString);
        DoMethod(elem->obj, MUIM_Set, MUIA_String_Contents, lpString);
      }
      else if (elem->windows_type==COMBOBOX) {
        DebOut("COMBOBOX: call SendDlgItemMessage instead:\n");
        return SendDlgItemMessage(hDlg, item, CB_ADDSTRING, 0, (LPARAM) lpString);
      }
      else {
        if(elem->windows_class && !strcmp(elem->windows_class, "RICHEDIT")) {
#ifndef DONT_USE_URLOPEN
          /* Urltext */
          DebOut("call MUIA_Urltext_Text, %s\n", lpString);
          SetAttrs(elem->obj,
                    //MUIA_Urltext_Text,   lpString,
                    MUIA_Urltext_Text,  lpString,
                    MUIA_Urltext_Url,   "http://TODO/text",
                    TAG_DONE);
#else
          DoMethod(elem->obj, MUIM_Set, MUIA_Text_Contents,   lpString);
#endif
        }
        else {
          DebOut("call MUIA_Text_Contents, %s\n", lpString);
          DoMethod(elem->obj, MUIM_Set, MUIA_Text_Contents,   lpString);
        }
      }

      return TRUE;
  }
  return FALSE;
}

/*
 * select/deselect checkbox
 */
BOOL CheckDlgButton(HWND hDlg, int button, UINT uCheck) {
  Element *elem=GetDlgItem(hDlg, button);

  DebOut("hDlg: 0x%p\n", hDlg);
  DebOut("button: %d\n", button);
  DebOut("uCheck: %d\n", uCheck);
  DebOut("elem: 0x%p\n", elem);
  DebOut("elem->obj: 0x%p\n", elem->obj);

  DoMethod(elem->obj, MUIM_NoNotifySet, MUIA_Selected, uCheck);

  return TRUE;
}

LONG SendDlgItemMessage(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam) {
  //Object *obj;
  //LONG i;
  ULONG l;
  IPTR activate;
  TCHAR *buf;
  Element *elem;

  DebOut("hDlg: %p, nIDDlgItem: %d, lParam: 0x%p\n", hDlg, nIDDlgItem, lParam);

#if 0
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
#endif
  elem=get_control(hDlg, nIDDlgItem);

  DebOut("elem->var: 0x%p\n", elem->var);

  switch(Msg) {

    case CB_ADDSTRING:
    {
      IPTR old_active = NULL;
      /* add string  */
      DebOut("CB_ADDSTRING (%s)\n", (TCHAR *) lParam);

      if(!flag_editable(elem->flags)) {
        /* remember old selection */
        DebOut("not editable\n");
        GetAttr(MUIA_Cycle_Active, elem->obj, (IPTR *) &old_active);
      }

      DebOut("elem->mem: 0x%p\n", elem->mem);

      l=0;
      DebOut("old list:\n");
      while(elem->mem[l] != NULL) {
        DebOut("  elem->mem[%d]: %s\n", l, elem->mem[l]);
        l++;
      }
      /* found next free string */
      activate=l;
#warning TODO: free strings again!
      elem->mem[l]=strdup((TCHAR *) lParam);
      elem->mem[l+1]=NULL;

#if 0
      l=0;
      DebOut("new list:\n");
      while(elem->mem[l] != NULL) {
        DebOut("  elem->mem[%d]: %s\n", l, elem->mem[l]);
        l++;
      }
#endif
 
      if(flag_editable(elem->flags)) {
        DoMethod(elem->obj, MUIM_List_Insert, (IPTR) elem->mem);
        DoMethod(elem->obj, MUIM_NoNotifySet, MUIA_String_Contents, (IPTR) lParam);
      }
      else {
        DoMethod(elem->obj, MUIM_Set, MUIA_Cycle_Entries, (IPTR) elem->mem);
      }

      l=0;
      while(elem->mem[l] != NULL) {
        DebOut("  CB_ADDSTRING: elem->.mem[%d]: %s\n", l, elem->mem[l]);
        l++;
      }
      if(!flag_editable(elem->flags)) {
        DebOut("MUIA_Cycle_Active: old_active %d\n", old_active);
        DoMethod(elem->obj, MUIM_NoNotifySet, MUIA_Cycle_Active, old_active);
      }
    }
    break;

    case CB_RESETCONTENT:
      /* delete all strings */
      DebOut("CB_RESETCONTENT (elem: %p)\n", elem);
      l=0;
      /* free old strings */
      while(elem->mem[l] != NULL) {
#warning free them!
        //free(elem->mem[l]);
        elem->mem[l]=NULL;
        l++;
      }
      //elem->var[0]=strdup("<empty>");
      //elem->var[1]=NULL;
      elem->mem[0]=NULL;
      DebOut("elem->obj: 0x%p\n", elem->obj);
      DoMethod(elem->obj, MUIM_NoNotifySet, MUIA_List_Entries, (IPTR) elem->mem);
      break;
    case CB_FINDSTRING:
      /* return string index */
      DebOut("CB_FINDSTRING\n");
      DebOut("search for string >%s<\n", (TCHAR *) lParam);
      l=0;
      while(elem->mem[l] != NULL) {
        DebOut("  compare with >%s<\n", elem->mem[l]);
        if(!strcmp(elem->mem[l], (TCHAR *) lParam)) {
          return l;
        }
        l++;
      }
      DebOut("return -1\n");
      return -1;

    case CB_GETCURSEL:
      DebOut("CB_GETCURSEL (return elem->value: %d)\n", elem->value);
      return elem->value;
      break;
    case WM_GETTEXT:
    case CB_GETLBTEXT:
      DebOut("[0x%p] WM_GETTEXT / CB_GETLBTEXT\n", elem->obj);
      {
        TCHAR *string=(TCHAR *) lParam;
        DebOut("[0x%p] elem->value: %d\n", elem->obj, elem->value);
        DebOut("[0x%p] wParam: %d\n", elem->obj, wParam);
        if(elem->windows_type==COMBOBOX) {
          if(elem->value<0 || elem->mem[elem->value]==NULL) {
            string[0]=(char) 0;
          }
          else {
            DebOut("return: %s\n", elem->mem[elem->value]);
            if(Msg == CB_GETLBTEXT) {
              DebOut("CB_GETLBTEXT\n");
              /* CB_GETLBTEXT has no range check! */
              strcpy(string, elem->mem[elem->value]);
            }
            else {
              strncpy(string, elem->mem[elem->value], wParam);
            }
          }
        }
        else if(elem->windows_type==EDITTEXT) {
          GetAttr(MUIA_String_Contents, elem->obj, (IPTR *) &string);
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

      if(elem->windows_type==EDITTEXT) {
        /* no combobox or similiar! */
        DebOut("set EDITTEXT to %s\n", lParam);
        SetAttrs(elem->obj, MUIA_String_Contents, lParam, TAG_DONE);
        return 1;
      }

      if(elem->windows_type!=COMBOBOX) {
        DebOut("ERROR: unknown type %d for WM_SETTEXT!!\n", elem->windows_type);
        return CB_ERR;
      }

      if(!flag_editable(elem->flags)) {
        DebOut("WM_SETTEXT for !editable!\n");
        /* "It is CB_ERR if this message is sent to a combo box without an edit control." */
        /* REALLY !? */
        return CB_ERR;
      }

      DebOut("elem: %p\n", elem);
      DebOut("elem->mem[0]: 0x%p\n", elem->mem[0]);

      l=0;
      while(elem->mem[l] != NULL) {
        DebOut("  compare with >%s<\n", elem->mem[l]);
        DebOut("  l: %d\n", l);
        if(!strcmp(elem->mem[l], (TCHAR *) lParam)) {
          found=l;
          DebOut("we already have that string at position %d!\n", found);
          //DebOut("activate string %d in combo box!\n", l);
          //DebOut("elem->obj: 0x%p\n", elem->obj);
          //SetAttrs(elem->obj, MUIA_String_Contents, lParam, TAG_DONE);
          break;
        }
        l++;
      }

      DebOut("found: %d\n", found);
 
      if(flag_editable(elem->flags)) {
        /* custom combo box */
        if(found > -1) {
          DoMethod(elem->obj, MUIM_NoNotifySet, MUIA_String_Contents, lParam);
          /* we are done! */
          return 1;
        }
      }
      DebOut("new string, add it to top\n");
      l=0;
      buf=elem->mem[l+1];
      while(buf != NULL) {
        elem->mem[l+1]=elem->mem[l];
        l++;
        buf=elem->mem[l+1];
      }
      /* store new string */
      elem->mem[0]=strdup((TCHAR *) lParam);
      /* terminate list */
      elem->mem[l+1]=NULL;

      if(flag_editable(elem->flags)) {
        DoMethod(elem->obj, MUIM_List_Insert, elem->mem);
      }
      else {
        DoMethod(elem->obj, MUIM_Set,         MUIA_Cycle_Entries, (IPTR) elem->mem);
        DebOut("MUIA_Cycle_Active\n");
        DoMethod(elem->obj, MUIM_NoNotifySet, MUIA_Cycle_Active, 0);
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
      DebOut("CB_SETCURSEL: elem %p wParam: %d\n", elem, wParam);

      if(flag_editable(elem->flags)) {
        /* zune combo object */
        if(wParam==-1) { /* TODO wParam unsigned!? */
          DebOut("clear string gadget\n");
          DoMethod(elem->obj, MUIM_NoNotifySet, MUIA_String_Contents, (IPTR) "");
          return CB_ERR;
        }
        else {
          /* check, if wParam is valid!? */
          DebOut("activate string: %s\n", elem->mem[wParam]);
          DoMethod(elem->obj, MUIM_NoNotifySet, MUIA_String_Contents, (IPTR) elem->mem[wParam]);
        }
      }
      else {
        /* cycle gadget */
        DebOut("cycle gadget muiobj: 0x%p\n", elem->obj);
        if(wParam >= 0) {
          DebOut("MUIM_NoNotifySet, MUIA_Cycle_Active, %d\n", wParam);
          DoMethod(elem->obj, MUIM_NoNotifySet, MUIA_Cycle_Active, wParam);
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
       DebOut("TBM_SETRANGE 0x%p (%d, %d)\n", lParam, LOWORD(lParam), HIWORD(lParam));
       DoMethod(elem->obj, MUIM_Set, MUIA_Numeric_Min, LOWORD(lParam));
       DoMethod(elem->obj, MUIM_Set, MUIA_Numeric_Max, HIWORD(lParam));
       break;
    case TBM_SETPAGESIZE:
      DebOut("WARNING: TBM_SETPAGESIZE seems not to be possible in Zune/MUI !?\n");
      break;
    case TBM_SETPOS:
       DebOut("TBM_SETPOS(%d) => DoMethod(%p, MUIM_NoNotifySet, MUIA_Numeric_Value, %d)\n", 
               lParam, elem->obj, lParam);
       DoMethod(elem->obj, MUIM_NoNotifySet, MUIA_Numeric_Value, lParam);
      break;
    case TBM_GETPOS:
        LONG res;
        res=XGET(elem->obj, MUIA_Numeric_Value);
        DebOut("TBM_GETPOS: %d\n", res);
        return res;
      break;
    default:
      DebOut("WARNING: unkown Windows Message-ID: %d\n", Msg);
      return FALSE;
  }

  return TRUE;
}

LRESULT SendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

  return (LRESULT) SendDlgItemMessage(hWnd, ((Element *)hWnd)->idc, Msg, wParam, lParam);
}

UINT GetDlgItemText(HWND hDlg, int nIDDlgItem, TCHAR *lpString, int nMaxCount) {
  //int i;
  //UINT ret;
  char *buffer = NULL; 
  //IPTR t;
  Element *elem;

#if 0
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
#endif

  elem=get_control(hDlg, nIDDlgItem);

  DebOut("elem->obj: 0x%p\n", elem->obj);
  DebOut("elem->windows_type: %d\n", elem->windows_type);

  if(elem->windows_type==EDITTEXT || elem->windows_type==COMBOBOX) {
    if(!flag_editable(elem->flags)) {
      TODO(); /* or not to do? */
    }
    GetAttr(MUIA_String_Contents, elem->obj, (IPTR *) &buffer);
    DebOut("buffer: %s\n", buffer);
    /* getting a NULL here is ok, for example a combo box might contain no entries */ 
    if(!buffer) {
      /* we must not return a NULL, but an empty string */
      strncpy(lpString, "", nMaxCount);
    }
    else {
      strncpy(lpString, buffer, nMaxCount);
    }
  }
  else {
    bug("Warning: elem->windows_type=%d, GetText..?\n", elem->windows_type);
    DebOut("Warning: elem->windows_type=%d, GetText..?\n", elem->windows_type);
    DebOut("return elem->text: %s\n", elem->text);
    strncpy(lpString, elem->text, nMaxCount);
  }
 
  DebOut("lpString: %s\n", lpString);

  return strlen(lpString);
}

/*
 * Sets the text of a control in a dialog box to the string representation of a specified integer value.
 */
BOOL SetDlgItemInt(HWND hDlg, int item, UINT uValue, BOOL bSigned) {
  //LONG i;
  TCHAR tmp[64];
  Element *elem;

  DebOut("hDlg: %p\n", hDlg);
  DebOut("item: %d\n", item);
  DebOut("value: %d\n", uValue);
  DebOut("bSigned: %d\n", bSigned);

  elem=get_control(hDlg, item);
  DebOut("elem: %p\n", elem);

#if 0
  i=get_index(elem, item);
  if(i<0) {
    elem=get_elem(item);
    i=get_index(elem, item);
  }

  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", item);
    return FALSE;
  }
#endif

  snprintf(tmp, 64, "%d", uValue); /* '-' is printed with %d anyways, no need to care for bSigned (?) */

  DebOut("set to: %s\n", tmp);

  if(!elem->var) {
    /* array for text (only one line, but respect data structure here */
    elem->var=(char **) calloc(16, sizeof(IPTR)); 
    elem->var[0]=NULL;
  }

  if(elem->var[0]) {
    /* free old content */
    free(elem->var[0]);
  }

  elem->var[0]=strdup(tmp);
  elem->var[1]=NULL;

  DebOut("elem->obj: 0x%p\n", elem->obj);
  if(elem->windows_type==EDITTEXT) {
    DebOut("elem->windows_type==EDITTEXT\n");
    DoMethod(elem->obj, MUIM_Set, MUIA_String_Contents, elem->var[0]);
  }
  else {
    DebOut("elem->windows_type!=EDITTEXT\n");
    DoMethod(elem->obj, MUIM_Set, MUIA_Text_Contents, elem->var[0]);
  }

  return TRUE;
}

/*
 * Translates the text of a specified control in a dialog box into an integer value.
 *
 * lpTranslated is optional!
 */
UINT GetDlgItemInt(HWND hDlg, int item, BOOL *lpTranslated,  BOOL bSigned) {
  //LONG i;
  char *content;
  long long res;
  char *end;
  Element *elem;

  DebOut("hDlg 0x%p, nIDDlgItem %d\n", hDlg, item);

  if(lpTranslated) {
    *lpTranslated=FALSE;
  }

#if 0
  i=get_index(elem, item);
  if(i<0) {
    elem=get_elem(item);
    i=get_index(elem, item);
  }

  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", item);
    return FALSE;
  }
#endif
  elem=get_control(hDlg, item);
  DebOut("elem: %p\n", elem);

  content=(char *) XGET(elem->obj, MUIA_String_Contents);
  DebOut("content: %s\n", content);

  res=strtoll(content, &end, 10);

  DebOut("end: >%s<\n", end);

  if(end && end[0] != '\0') {
    DebOut("ERROR: %s is not a number\n", content);
    return 0;
  }

  if(lpTranslated) {
    *lpTranslated=TRUE;
  }
  DebOut("res: %d\n", res);

  return (UINT) res;
}

/* Adds a check mark to (checks) a specified radio button in a group and removes 
 * a check mark from (clears) all other radio buttons in the group. 
 */
BOOL CheckRadioButton(HWND hDlg, int nIDFirstButton, int nIDLastButton, int nIDCheckButton) {
  //int i=-666;
  //int e=-666;
  //int set=-666;
  //IPTR act;
  Element *elem;

  DebOut("%p, nIDFirstButton %d, nIDLastButton %d, nIDCheckButton%d\n", 
         hDlg, nIDFirstButton, nIDLastButton, nIDCheckButton);
  elem=get_control(hDlg, nIDCheckButton);
  DebOut("Element: %p\n", elem);

#if 0
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
#endif

  if(elem->flags2==BS_AUTORADIOBUTTON) {
    if(elem->value) {
      DebOut("Button was already active, nothing to do here.\n");
      return TRUE;
    }

    /* 
     * we are an autoradio button. only one will be active anyways, so just "click" on me,
     * notification should reach the right button.
     */
    DoMethod(elem->obj, MUIM_Set, MUIA_Selected, TRUE);
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

UINT IsDlgButtonChecked(HWND hDlg, int item) {
  //int i;
  Element *elem;

  DebOut("hDlg: 0x%p\n", hDlg);
  DebOut("item: %d\n", item);
  elem=get_control(hDlg, item);
  DebOut("elem: %d\n", elem);
  if(!elem) {
    bug("ERROR: can't find item %d in hDlg %p! (fatal error)\n", item, hDlg);
    /* crash .. */
  }

#if 0
  i=get_index(elem, item);
  if(i<0) {
    elem=get_elem(item);
    i=get_index(elem, item);
  }

  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n");
    return FALSE;
  }
#endif

  DebOut("elem->obj: 0x%p\n", elem->obj);
  DebOut("elem->value: 0x%p\n", elem->value);
  if(elem->text) {
    DebOut("elem->text: %s\n", elem->text);
  }

  return elem->value;
}

extern Object *reset;

BOOL EnableWindow(HWND hWnd, BOOL bEnable) {
  //int i;
  Element *elem=(Element *) hWnd;

  DebOut("elem: %p\n", elem);

  if(elem->idc == IDC_RESETAMIGA) {
    /* special case, IDC_RESETAMIGA is handcoded and no Windows button */
    DoMethod(reset, MUIM_Set, MUIA_Disabled, !bEnable);

    return TRUE;
  }
#if 0
  i=get_index(hWnd, id);
  if(i<0) {
    hWnd=get_elem(id);
    i=get_index(hWnd, id);
  }

  if(i<0) {
    DebOut("ERROR: could not find elem 0x%p id %d\n", hWnd, id);
    return FALSE;
  }
#endif

  DebOut("elem->obj: %p\n", elem->obj);
  DebOut("SET(elem->obj, MUIA_Disabled, %d, TAG_DONE);\n", !bEnable);

  DoMethod(elem->obj, MUIM_Set, MUIA_Disabled, !bEnable);

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
    DebOut("ERROR: nIDDlgItem %d not found in 0x%p!?\n", nIDDlgItem, hDlg);
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

  LONG i;
  LONG item=-1;
  Element *e;

  if(hWnd==NULL) {
    DebOut("hWnd: 0x%p\n", hWnd);
    TODO();
    return FALSE;
  }
  DebOut("lpString: %s\n", lpString);
  DebOut("hWnd:     %p\n", hWnd);

  /* Check if it is a control first, not nice, I know. But I need the index here. */
  i=0;
  while(WIN_RES[i].idc) {
    if(hWnd == &WIN_RES[i]) {
      item=i;
      break;
    }
    i++;
  }

  if(item>-1) {
    DebOut("seems to be a control and not our main window, call SetDlgItemText instead\n");
    /* in our "AROS" mapping layer, it is ok to supply a NULL handle here, item 
       numbers are unique and will be found anyways */
    SetDlgItemText(NULL, item, lpString);
  }
  else {
    /* are we really sure, it is now our main window? */
    SetAttrs(win, MUIA_Window_Title, lpString, TAG_DONE);
  }
  return TRUE;
}

int GetWindowText(HWND   hWnd, LPTSTR lpString, int nMaxCount) {
  DebOut("hWnd: 0x%p\n", hWnd);

  if(hWnd==NULL) {
    DebOut("hWnd is NULL!: 0x%p\n", hWnd);
    TODO();
    return 0;
  }

  /* pray for enough space!! */
  GetAttr(MUIA_String_Contents, win, (IPTR *) lpString);

  DebOut("lpString: %s\n", lpString);
  return strlen(lpString);

}


/* WARNING: not same API call as in Windows */
int GetWindowText(HWND   hWnd, DWORD id, LPTSTR lpString, int nMaxCount) {
  return GetWindowText(hWnd, lpString, nMaxCount);
}

BOOL ShowWindow(struct Window *win, int nCmdShow) {
  TODO();
  return TRUE;
}

BOOL ShowWindow(HWND hWnd, int nCmdShow) {
  //int i;
  Element *elem=(Element *) hWnd;

  DebOut("elem: 0x%p\n", hWnd);

#if 0
  i=get_index(hWnd, id);
  if(i<0) {
    hWnd=get_elem(id);
    i=get_index(hWnd, id);
  }

  if(i<0) {
    DebOut("ERROR: could not find elem 0x%p id %d\n", hWnd, id);
    return FALSE;
  }
#endif

  DebOut("elem->obj: 0x%p\n", elem->obj);

  if(nCmdShow==SW_SHOW) {
    DoMethod(elem->obj, MUIM_Set, MUIA_ShowMe, TRUE);
  }
  else {
    DoMethod(elem->obj, MUIM_Set, MUIA_ShowMe, FALSE);
  }

  return TRUE;
}

