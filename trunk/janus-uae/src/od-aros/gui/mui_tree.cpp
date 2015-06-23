
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
#include <mui/NList_mcc.h>
#include <mui/NListtree_mcc.h>
#include <mui/NListview_mcc.h>
#include <mui/NFloattext_mcc.h>

#include <SDI/SDI_hook.h>

#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "aros.h"
#include "gui.h"
#include "winnt.h"
#include "mui_data.h"
#include "gui_mui.h"

/* Deletes all items from a tree-view control. 
 * WARNING: nIDDlgItem is not in the Windows API!
 */
BOOL TreeView_DeleteAllItems(HWND elem, int item) {

  LONG i;

  i=get_index(elem, item);
  if(i<0) {
    elem=get_elem(item);
    i=get_index(elem, item);
  }
  
  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", item);
    return NULL;
  }

  DebOut("elem[i].obj: %lx\n", elem[i].obj);

  DoMethod(elem[i].obj, MUIM_List_Clear);
}

HTREEITEM TreeView_InsertItem(HWND elem, int item, LPTVINSERTSTRUCT lpis) {

  LONG i;

  DebOut("elem: %lx\n", elem);
  DebOut("item: %d\n", item);
  DebOut("is.itemex.pszText: %s\n", lpis->itemex.pszText);
  
  i=get_index(elem, item);
  if(i<0) {
    elem=get_elem(item);
    i=get_index(elem, item);
  }
  
  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", item);
    return NULL;
  }

  DebOut("elem[i].obj: %lx\n", elem[i].obj);
  DebOut("lpis: %lx\n", lpis);

  /* save config (lParam) in UserData */
  DoMethod(elem[i].obj, MUIM_NListtree_Insert, lpis->itemex.pszText, (IPTR) lpis->itemex.lParam,
                        MUIV_NListtree_Insert_ListNode_Root,
                        MUIV_NListtree_Insert_PrevNode_Tail,
                        MUIV_NListtree_Insert_Flag_Active);
  
  // care for hInsertAfter = TVI_ROOT : TVI_SORT .. */
  TODO();
  // care for is.itemex.lParam = (LPARAM)config;
}

BOOL TreeView_DeleteItem(HWND hwndTV, int nIDDlgItem, HTREEITEM hitem) {
  TODO();
}

HTREEITEM TreeView_GetSelection(HWND elem, int item) {

  TODO();
}

BOOL TreeView_GetItem(HWND elem, int item, APTR pitem_void) {

  LPTVITEM pitem=(LPTVITEM) pitem_void; /* gcc does no automatic cast here, as MS C++ does */

  TODO();
}

/**********************************************************************************************/

#if 0
#warning THIS IS WRONG!! Defined in win32gui.cpp!!
struct ConfigStruct {
	TCHAR Name[MAX_DPATH];
	TCHAR Path[MAX_DPATH];
	TCHAR Fullpath[MAX_DPATH];
	TCHAR HostLink[MAX_DPATH];
	TCHAR HardwareLink[MAX_DPATH];
};

struct TreeLine {
  TCHAR name[MAX_DPATH];
  struct ConfigStruct *config;
};

HOOKPROTONHNO(TreeView_DisplayHookFunction, void, struct MUIP_NListtree_DisplayMessage *ndm) {

  struct TreeLine *line=(struct TreeLine *) ndm->TreeNode->tn_User;

  DebOut("line: %lx\n", line);

  if(line) {
    DebOut("line->: %s\n", line->name);
    *ndm->Array++=line->name;
    //ndm->strings[0]="FOO";
    //ndm->preparses[0]=NULL;
  }
  else {
    /* we don't have a header bar! */
    DebOut("ERROR!?\n");
  }
}

HOOKPROTONHNO(TreeView_ConstructHookFunction, APTR, struct MUIP_NListtree_ConstructMessage *ncm) {
  DebOut("ncm: %lx\n", ncm);

  struct TreeLine *line=(struct TreeLine *) AllocVec(sizeof(struct TreeLine), 0);

  TVINSERTSTRUCT *is=(TVINSERTSTRUCT *) ncm->UserData;
  DebOut("is: %lx\n", is);
  DebOut("ncm->name: %s\n", ncm->Name);

  if(line && is) {
    line->config=(struct ConfigStruct *) AllocVec(sizeof(struct ConfigStruct), 0);

    if(is->itemex.pszText) {
      DebOut("is->itemex.pszText: %s\n", is->itemex.pszText);
      strncpy(line->name, is->itemex.pszText, MAX_DPATH-1);
    }

    if(is->itemex.lParam) {
      memcpy((void *) line->config, (const  void *) is->itemex.lParam, sizeof(struct ConfigStruct));
    }
  }
  else {
    DebOut("NOT ENOUGH MEMORY!\n");
  }

  DebOut("return: %lx\n", line);

  return line;
}

HOOKPROTONHNO(TreeView_DestructHook, void, struct NList_DestructMessage *ndm) {

  struct TreeLine *line;

  DebOut("ndm: %lx\n", ndm);

  if (ndm->entry) {
    line=(struct TreeLine *) ndm->entry;
    if(line->config) {
      FreeVec((void *) line->config);
    }
    FreeVec((void *) line);
  }

}
#endif

Object *new_tree(ULONG i, struct Hook *construct_hook, struct Hook *destruct_hook, struct Hook *display_hook) {

  Object *tree=NULL;

#if 0
  construct_hook->h_Entry=(APTR) TreeView_ConstructHookFunction;
  construct_hook->h_Data=NULL;
  destruct_hook->h_Entry=(APTR) TreeView_DestructHook;
  destruct_hook->h_Data=NULL;
  display_hook->h_Entry=(APTR) TreeView_DisplayHookFunction;
  display_hook->h_Data=NULL;
#endif

  tree=HGroup,
          MUIA_UserData, i,
          Child, (NListviewObject,
            MUIA_UserData, i,
            MUIA_NListview_NList, (NListtreeObject,
                MUIA_UserData, i,
                ReadListFrame,
                MUIA_CycleChain, TRUE,
                MUIA_NList_Title, FALSE,
                //MUIA_NListtree_DisplayHook,   display_hook,
                //MUIA_NList_ConstructHook2, construct_hook,
                //MUIA_NListtree_DestructHook,  destruct_hook,
                MUIA_NListtree_DragDropSort,  FALSE,
                //MUIA_NListtree_ShowTree, FALSE,
              End),
            End),
          End;

  return tree;
}
