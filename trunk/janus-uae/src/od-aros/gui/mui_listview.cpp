
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
//#include <mui/NList_mcc.h>
//#include <mui/NListview_mcc.h>
#include <mui/NFloattext_mcc.h>
#include <mui/NBitmap_mcc.h>

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

struct hook_data{
  struct Element *elem;
  int item;
};

#define MAX_COL 31
struct view_line {
  char column[MAX_COL+1][128];
};

static char foo[]="hey ho";
/* 
 * ListView display hook
 */
AROS_UFH3S(LONG, display_func, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(char **, array, A2), AROS_UFHA(struct view_line *, entry, A1)) {

  AROS_USERFUNC_INIT

  struct hook_data *h_data = (struct hook_data *) hook->h_Data;
  struct Element *elem;
  int i;
  ULONG t;
  ULONG c=0;

  /* get elem for this object */
  DebOut("Display hook entered (entry=%lx)\n", entry);

  elem=h_data->elem;
  i   =h_data->item;
  DebOut("elem: %lx, i: %d\n", elem, i);

  if(!elem[i].mem || !elem[i].mem[0]) {
    DebOut("no columns..!?\n");
    *array++ = foo;
    *array++ = foo;
    goto EXIT;
  }

  if(!entry) {
    /* title */
    while(elem[i].mem[c]) {
      DebOut("elem[i].mem[c]: %s\n", elem[i].mem[c]);
      *array++ = elem[i].mem[c];
      c++;
    }
  }
  else {
    /* count columns */
    while(elem[i].mem[c]) {
      c++;
    }
    for(t=0;t<c;t++) {
      *array++ = (STRPTR) entry->column[t];
    }
  }

EXIT:
  return 0;
  AROS_USERFUNC_EXIT
}

AROS_UFH3S(APTR, construct_func, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, pool, A2), AROS_UFHA(struct view_line *, entry, A1)) {

  AROS_USERFUNC_INIT

  struct view_line *new_line;
  ULONG i;

  DebOut("Construct hook entered (pool: %p)\n", pool);

  if((new_line = (struct view_line *) AllocPooled(pool, sizeof(struct view_line)))) {
    DebOut("AllocPooled(pool %p)=%p\n", pool, entry);
    for(i=0; i<MAX_COL; i++) {
      if(entry->column[i][0]) {
        DebOut("copy %s\n", entry->column[i]);
        strcpy(new_line->column[i], entry->column[i]);
      }
      else {
        new_line->column[i][0]=(char) 0;
      }
    }
  }

  return new_line;

  AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, destruct_func, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, pool, A2), AROS_UFHA(struct view_line *, entry, A1)) {
  AROS_USERFUNC_INIT

  DebOut("Destruct hook entered (pool: %p)\n", pool);
  DebOut("FreePooled(pool %p, entry %p)\n", pool, entry);

  FreePooled(pool, entry, sizeof(struct view_line));

  AROS_USERFUNC_EXIT
}


/*
 * return new listview object
 * return according list in **list
 */
Object *new_listview(struct Element *elem, ULONG i, void *f, struct Data *data, Object **list) {

  Object *listview=NULL;
  Object *nlist;
  //int *(*func) (Element *hDlg, UINT msg, ULONG wParam, IPTR lParam);
  //func=(int* (*)(Element*, uint32_t, ULONG, IPTR)) f;
  //ULONG t=0;
  DebOut("i (not used): %d\n", i);

  nlist=ListObject,
        InputListFrame,
        MUIA_List_Title, TRUE, /* title can't be switched on/off in Zune !? */
       End;

  DebOut("new ListObject: %lx\n", nlist);


  DoMethod(nlist, MUIM_List_Clear, TRUE);
  DebOut("ListObject %lx cleared\n", nlist);

  listview=ListviewObject,
            MUIA_Listview_List, nlist,
            //MUIA_List_MultiSelect, MUIV_List_MultiSelect_None,
            //MUIA_Listview_ScrollerPos, MUIV_Listview_ScrollerPos_None,
           End;

  DebOut("new ListViewObject: %lx\n", listview);
  //char *str = "New entry";
  //DoMethod(*list,MUIM_List_Insert,&str,1,MUIV_List_Insert_Bottom);

  if(!listview) {
    DebOut("ERROR: unable to create tree!\n");
    return NULL;
  }

  *list=nlist;


#if 0
  /* setup hooks */
#ifdef UAE_ABI_v0
  data->MyMUIHook_tree_active.h_Entry=(HOOKFUNC) tree_active;
#else
  data->MyMUIHook_tree_active.h_Entry=(APTR) tree_active;
#endif
  data->MyMUIHook_tree_active.h_Data =(APTR) data;

  DoMethod(tree, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR) tree, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_tree_active, func); 

#ifdef UAE_ABI_v0
  data->MyMUIHook_tree_double.h_Entry=(HOOKFUNC) tree_double;
#else
  data->MyMUIHook_tree_double.h_Entry=(APTR) tree_double;
#endif
  data->MyMUIHook_tree_double.h_Data =(APTR) data;

  DoMethod(tree, MUIM_Notify, MUIA_List_DoubleClick, MUIV_EveryTime, (IPTR) tree, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_tree_double, func); 
#endif

  return listview;
}

/*
 * return number of entries
 *
 * see Warning above
 */
int ListView_GetItemCount(int nIDDlgItem) {
  HWND elem;
  int i;
  int nr=0;

  DebOut("nIDDlgItem: %d\n", nIDDlgItem);

  elem=get_elem(nIDDlgItem);
  if(!elem) return 0;
  i=get_index(elem, nIDDlgItem);
  if(i<0) return 0;

  DebOut("list obj: %lx\n", elem[i].action);
  nr=(int) XGET(elem[i].action, MUIA_List_Entries);
  DebOut("MUIA_List_Entries: %d\n", nr);
 
  return nr;
}

void ListView_SetItemState(int hwnd, int i, UINT state, UINT mask) {
  TODO();
}


LONG Button_SetElevationRequiredState(HWND hDlg, int nIDDlgItem, BOOL fRequired) {
  TODO();
  return 1;
}

/*
 * This macro retrieves the index of the topmost visible item when in list 
 * or report view. You can use this macro or send the LVM_GETTOPINDEX message 
 * explicitly. 
 */
/* WARNING: on windows, this function gets a HWND, which does not work for us.
 * maybe it was a bad decision, to fake HWND.. we can only supply one parameter here,
 * so we use the nIDDlgItem and search for the correct HWND..
 */
int ListView_GetTopIndex(int nIDDlgItem) {
  TODO();
  return 1;
}

/*
 * Calculates the number of items that can fit vertically in the visible area of a 
 * list-view control when in list or report view. Only fully visible items are counted.
 *
 * See WARNING above!
 */
int ListView_GetCountPerPage(int hwnd) {
  TODO(); /* return number of items.. */
  return 100;
}

/*
 * Sets extended styles for list-view controls using the style mask.
 * (ignored so far..)
 * See WARNING above!
 */
void ListView_SetExtendedListViewStyleEx(int hwndLV, DWORD dwExMask, DWORD dwExStyle) {
  TODO();
}

/* 
 * Removes all groups from a list-view control. 
 * See WARNING above!
 */
void ListView_RemoveAllGroups(int hwnd) {
  TODO();
}

/*
 * Removes all items from a list-view control.
 * See WARNING above!
 */
BOOL ListView_DeleteAllItems(int nIDDlgItem) {
  struct Element *elem;
  int i;

  elem=get_elem(nIDDlgItem);
  if(!elem) return FALSE;
  i=get_index(elem, nIDDlgItem);
  if(i<0) return FALSE;
  DebOut("elem: %lx, i: %d elem[i].action: %lx\n", elem, i, elem[i].action);

  DoMethod(elem[i].action, MUIM_List_Clear);

  return TRUE;
}

/*
 * Gets the attributes of a list-view control's column.
 * 
 * See WARNING above!
 * WARNING: for WinUAE it is enough, to return FALSE, if there are no columns and
 *          TRUE, if there are already colums.
 */
BOOL ListView_GetColumn(int nIDDlgItem, int iCol, LPLVCOLUMN pcol) {
  HWND elem;
  int i;

  DebOut("nIDDlgItem: %d\n", nIDDlgItem);

  elem=get_elem(nIDDlgItem);
  if(!elem) return FALSE;
  i=get_index(elem, nIDDlgItem);
  if(i<0) return FALSE;

  if(elem[i].mem) {
    DebOut("we have columns: TRUE!\n");
    return TRUE;
  }

  DebOut("FALSE!\n");
  return FALSE;
}


/* 
 * Inserts a new column in a list-view control. 
 *
 * For us it should be enough to just care for pszText
 * We store all columns in elem->mem..
 * See WARNING above!
 */
int ListView_InsertColumn(int nIDDlgItem, int iCol, const LPLVCOLUMN pcol) {
  HWND elem;
  ULONG i=0;
  ULONG t=0;
  ULONG u=0;
  char format[128];
  struct Hook *display_hook;
  struct Hook *construct_hook;
  struct Hook *destruct_hook;
  struct hook_data *h_data; 

  DebOut("pszText: %s\n", pcol->pszText);

  elem=get_elem(nIDDlgItem);
  if(!elem) return 0;
  i=get_index(elem, nIDDlgItem);
  if(i<0) return 0;
  DebOut("elem: %lx, i: %d\n", elem, i);

  if(!elem[i].mem) {
    elem[i].mem=(char **) calloc(32, sizeof(IPTR)); /* 32 columns should be enough for everybody.. */
    elem[i].mem[0]=NULL;
  }

  while(elem[i].mem[t]!=NULL) {
    t++;
  }

  if(t>31) {
    DebOut("ERROR: too many columns!\n");
    return 0;
  }

  DebOut("t: %d\n", t);
  elem[i].mem[t]=strdup(pcol->pszText);
  elem[i].mem[t+1]=NULL;

  strcpy(format,"BAR");
  format[3]=(char) 0;

  for(u=0;u<t;u++) {
    strcpy(format+strlen(format),",BAR");
  }

  DebOut("format: >%s<\n", format);
  DebOut("elem[i].action: %lx\n", elem[i].action);


  /* if this is our first column, we need to setup the display hook */
  if(!elem[i].var_data) {
    /* store hooks in var_data*/
    DebOut("init hooks..\n");
    elem[i].var_data   =(IPTR *) calloc(16, sizeof(IPTR));
    elem[i].var_data[0]=(IPTR)   calloc( 1, sizeof(struct hook_data));
    elem[i].var_data[1]=(IPTR)   calloc( 1, sizeof(struct Hook)); /* display hook */
    elem[i].var_data[2]=(IPTR)   calloc( 1, sizeof(struct Hook)); /* construct hook */
    elem[i].var_data[3]=(IPTR)   calloc( 1, sizeof(struct Hook)); /* destruct hook */

    h_data      =(struct hook_data *) elem[i].var_data[0];
    h_data->elem=elem;
    h_data->item=i;

    display_hook         =(struct Hook *) elem[i].var_data[1];
    display_hook->h_Entry=(APTR) display_func;
    display_hook->h_Data =(APTR) h_data;

    construct_hook         =(struct Hook *) elem[i].var_data[2];
    construct_hook->h_Entry=(APTR) construct_func;
    construct_hook->h_Data =(APTR) h_data;

    destruct_hook         =(struct Hook *) elem[i].var_data[3];
    destruct_hook->h_Entry=(APTR) destruct_func;
    destruct_hook->h_Data =(APTR) h_data;

    //SetAttrs(elem[i].action, MUIA_List_Title, FALSE, TAG_DONE);

    DebOut("set MUIA_List_ConstructHook for object %lx\n", elem[i].action);
    SetAttrs(elem[i].action, MUIA_List_DisplayHook,   display_hook,
                             MUIA_List_ConstructHook, construct_hook,
                             MUIA_List_DestructHook,  destruct_hook,
                             MUIA_List_Format,        format, 
                             MUIA_List_Title,         TRUE, 
                             TAG_DONE);
  }
  else {
    SetAttrs(elem[i].action, MUIA_List_Format, format, TAG_DONE);
  }

  return 1;
}

/* 
 * Inserts a new item in a list-view control.
 *
 * Seems to insert a new line, text is text of first row.. returns new line number
 *
 * see WARNING above! 
 */
int ListView_InsertItem(int nIDDlgItem, const LPLVITEM pitem) {

  struct Element *elem;
  int i;
  struct view_line *new_line;
  ULONG pos;

  DebOut("pszText: %s\n", pitem->pszText);
#if 0
  if (pitem->iItem) DebOut("iItem (ignored!!): %s\n", pitem->iItem);
  if (pitem->iSubItem) DebOut("iSubItem (ignored!!): %s\n", pitem->iSubItem);
  if (pitem->iImage) DebOut("iImage (ignored!!): %s\n", pitem->iImage);
#endif

  elem=get_elem(nIDDlgItem);
  if(!elem) return -1;
  i=get_index(elem, nIDDlgItem);
  if(i<0) return -1;
  DebOut("elem: %lx, i: %d\n", elem, i);

  new_line=(struct view_line *) AllocVec(sizeof(struct view_line), MEMF_CLEAR);
  if(!new_line) return -1;

  DebOut("sizeof(struct view_line): %d\n", sizeof(struct view_line));

  strcpy(new_line->column[0], pitem->pszText);
  DebOut("elem[%d].action: %lx\n", i, elem[i].action);
  DoMethod(elem[i].action, MUIM_List_InsertSingle, new_line, MUIV_List_Insert_Bottom);
  FreeVec(new_line);
  new_line=NULL;

  DebOut("..\n");
  pos=XGET(elem[i].action, MUIA_List_InsertPosition);
  //pos--; /* why --? Seems as if Zune always return "one too much"? */
  DebOut("pos: %d\n", pos);

  return pos;
}

/* 
 * Changes the text of a list-view item or subitem. 
 *
 * i is line number from ListView_InsertItem
 * iSubItem is column number
 * pszText is item text
 * see WARNING above! 
 *
 * Zune has no function to modify single columns. So we fetch the old line,
 * copy contents over and replace just the single column. Then we remove
 * the old line and insert our new one..
 */
VOID ListView_SetItemText(int nIDDlgItem, int line, int iSubItem, const char *pszText) {

  struct Element *elem;
  int i;
  struct view_line *old_line;
  struct view_line *new_line;
  LONG pos;

  DebOut("line %d, column %d: %s\n", line, iSubItem, pszText);

  /* usual "find ourselves" */
  elem=get_elem(nIDDlgItem);
  if(!elem) return;
  i=get_index(elem, nIDDlgItem);
  if(i<0) return;
  DebOut("elem: %lx, i: %d\n", elem, i);

  /* build new line. not on the stack, as it is "big" */
  new_line=(struct view_line *) AllocVec(sizeof(struct view_line), MEMF_CLEAR);
  if(!new_line) return;

  /* fetch current content */
  DoMethod(elem[i].action, MUIM_List_GetEntry, line, &old_line);
  if(!old_line) {
    DebOut("old line not found !? so maybe we are the first line!?\n");
    for(pos=0; pos<MAX_COL; pos++) {
      if(pos==iSubItem) {
        /* out new  column! */
        DebOut("repl column %d: %s\n", pos, pszText);
        strcpy(new_line->column[pos], pszText);
      }
      else {
        DebOut("just empty strings at %d\n", pos);
        strcpy(new_line->column[pos], " ");
      }
    }
  }
  else {
    /* copy/replace content */
    for(pos=0; pos<MAX_COL; pos++) {
      if(pos==iSubItem) {
        /* replace column! */
        DebOut("repl column %d: %s\n", pos, pszText);
        strcpy(new_line->column[pos], pszText);
      }
      else {
        DebOut("copy column %d: %s\n", pos, old_line->column[pos]);
        strcpy(new_line->column[pos], old_line->column[pos]);
      }
    }
    /* replace old line with new one */
    DoMethod(elem[i].action, MUIM_List_Remove, line);
  }
  DebOut("MUIM_List_InsertSingle %d\n", line);
  DoMethod(elem[i].action, MUIM_List_InsertSingle, new_line, line);
  /* TODO (?): FreeVec */
}

int ListView_GetStringWidth(int nIDDlgItem, const char *psz) {

  TODO();
  return 1;
}

/*
 * Gets the state of a list-view item. 
 *
 * see WARNING above! 
 */
UINT ListView_GetItemState(int nIDDlgItem, int  nr, UINT mask) {
  struct Element *elem;
  int i;
  IPTR act;

  DebOut("nIDDlgItem %d: %d\n", nIDDlgItem);

  /* usual "find ourselves" */
  elem=get_elem(nIDDlgItem);
  if(!elem) return 0;
  i=get_index(elem, nIDDlgItem);
  if(i<0) return 0;
  DebOut("elem: %lx, i: %d\n", elem, i);

  if(mask!=LVIS_SELECTED) {
    DebOut("mask %d not implemented!\n", mask);
    TODO();
    return FALSE;
  }

  act=XGET(elem[i].action, MUIA_List_Active);
  DebOut("active entry: %d\n", act);

  if((IPTR) nr== act) {
    return LVIS_SELECTED;
  }

  return 0;
}
