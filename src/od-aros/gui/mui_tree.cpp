
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
#include <mui/NBitmap_mcc.h>

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
#if 0
#include "png2c/misc.h"
#include "png2c/folder.h"
#include "png2c/quickstart.h"
#endif

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

  DoMethod(elem[i].obj, MUIM_NListtree_Clear, NULL, 0);

#if 0
  DoMethod(elem[i].obj, MUIM_NListtree_Insert, "TEST >\33o[0]< TEST", NULL,
                        MUIV_NListtree_Insert_ListNode_Root,
                        MUIV_NListtree_Insert_PrevNode_Tail,
                        MUIV_NListtree_Insert_Flag_Active);
#endif
 
}

HTREEITEM TreeView_InsertItem(HWND elem, int item, LPTVINSERTSTRUCT lpis) {

  LONG i;
  struct MUI_NListtree_TreeNode *listnode=NULL;
  struct MUI_NListtree_TreeNode *prevtreenode=NULL;
  struct MUI_NListtree_TreeNode *newnode=NULL;
  ULONG flags=0;

  DebOut("elem: %lx, item: %d\n", elem, item);
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

  DebOut("elem[%d].obj: %lx\n", i, elem[i].obj);

  /* Handle to the parent item. 
   * If this member is the TVI_ROOT value or NULL, the item is inserted 
   * at the root of the tree-view control.
   */
#if 0
  if(!lpis->hParent || lpis->hParent==TVI_ROOT) {
    DebOut("New Root node\n");
  }
  else {
    DebOut("insert in hParent (TODO)!!\n");
    DebOut("hParent: %lx\n", lpis->hParent);
  }
#endif

  /* 
   * TVI_FIRST Inserts the item at the beginning of the list.
   * TVI_LAST  Inserts the item at the end of the list.
   * TVI_ROOT  Add the item as a root item.
   * TVI_SORT  Inserts the item into the list in alphabetical order.
   */
  if(lpis->itemex.state & TVIS_EXPANDED) {
    DebOut("TVIS_EXPANDED\n");
    flags=TNF_LIST | TNF_OPEN;
  }

  if(lpis->itemex.state & TVIS_SELECTED) {
    DebOut("TVIS_SELECTED\n");
    flags=flags | MUIV_NListtree_Insert_Flag_Active;
  }

  listnode=(struct MUI_NListtree_TreeNode *) MUIV_NListtree_Insert_ListNode_Root;
  if(lpis->hParent && lpis->hParent!=TVI_ROOT) {
    DebOut("we need to insert into node: %lx\n", lpis->hParent);
    listnode=(struct MUI_NListtree_TreeNode *) lpis->hParent;
  }

  /* save config (lParam) in UserData */
  newnode=(struct MUI_NListtree_TreeNode *) DoMethod(elem[i].obj, MUIM_NListtree_Insert, 
                    lpis->itemex.pszText, (IPTR) lpis->itemex.lParam,
                    listnode,
                    MUIV_NListtree_Insert_PrevNode_Tail,
                    flags);

  DebOut("newnode: %lx\n", newnode);
  
  return (HTREEITEM) newnode;
}

BOOL TreeView_DeleteItem(HWND hwndTV, int nIDDlgItem, HTREEITEM hitem) {
  TODO();
}

/* WARNING: this is just a minimal implementation to get win32gui.cpp/LoadSaveDlgProc working!*/
HTREEITEM TreeView_GetSelection(HWND elem, int item) {

  LONG i;

  DebOut("elem: %lx, item: %d\n", elem, item);
  
  i=get_index(elem, item);
  if(i<0) {
    elem=get_elem(item);
    i=get_index(elem, item);
  }
  
  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", item);
    return NULL;
  }

  DebOut("WARNING: This is just a fake function, only working for win32gui.cpp/LoadSaveDlgProc!\n");
  return (HTREEITEM) 666;
}

/* we need to return the selected config in pitem.lParam
 * WARNING: this is just a minimal implementation to get win32gui.cpp/LoadSaveDlgProc working!
 */
BOOL TreeView_GetItem(HWND elem, int item, APTR pitem_void) {

  LPTVITEM pitem=(LPTVITEM) pitem_void; /* gcc does no automatic cast here, as MS C++ does */

  LONG i;
  struct MUI_NListtree_TreeNode *act_node;

  DebOut("elem: %lx, item: %d\n", elem, item);
  
  i=get_index(elem, item);
  if(i<0) {
    elem=get_elem(item);
    i=get_index(elem, item);
  }
  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", item);
    return NULL;
  }

  act_node=(struct MUI_NListtree_TreeNode *) XGET(elem[i].obj, MUIA_NListtree_Active);

  DebOut("act_node: %lx\n", act_node);
  DebOut("act_node->tn_User: %lx\n", act_node->tn_User);

  /* is this correct !? */
  if(act_node == MUIV_NListtree_Active_Off) {
    pitem->lParam=(LPARAM) NULL;
    return FALSE;
  }

  pitem->lParam=(LPARAM) act_node->tn_User;
  DebOut("pitem->lParam: %lx\n", pitem->lParam);

  return TRUE;
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

static void tree_send_notify(ULONG type, Object *obj, struct Data *data, struct ConfigStruct *config) {

  int i;
  ULONG state;
  LPNMHDR nm=NULL;
  LPNMTREEVIEW tv;
  struct MUI_NListtree_TreeNode *node;

  DebOut("[%lx] entered\n", obj);

  i=get_elem_from_obj(data, (Object *) obj);

  if(!data->func) {
    DebOut("[%lx] function is zero: %lx\n", obj, data->func);
    goto TREE_SEND_NOTIFY_EXIT;
  }

  node=(struct MUI_NListtree_TreeNode *) XGET((Object *) obj, MUIA_NListtree_Active);
  if(!node || node==(struct MUI_NListtree_TreeNode *) MUIA_NListtree_Active) {
    DebOut("no node / MUIA_Listtree_Active_Off! Do nothing!\n");
    goto TREE_SEND_NOTIFY_EXIT;
  }

  DebOut("node: %lx\n", node);
 
  /* LPNMHDR is sent in itemNew.lParam */
  /* WARNING: all other fields are not correctly set */
  /* NMTREEVIEW contains NMHDR */
  nm=(LPNMHDR) AllocVec(sizeof(NMTREEVIEW), MEMF_CLEAR);
  nm->idFrom=NULL; /* not used by WinUAE! */
  nm->code=type;
  nm->hwndFrom=IDC_CONFIGTREE; 

  /* LPNMTREEVIEW is sent in itemNew.lParam */
  tv=(LPNMTREEVIEW) nm;
  tv->itemNew.lParam=(LPARAM) node->tn_User;

  /* still a lot of unfilled attributes! */
  TODO();

  DebOut("[%lx] call function: %lx(IDC %d, WM_NOTIFY, %lx)\n", obj, data->func, data->src[i].idc, nm);
  data->func(data->src, WM_NOTIFY, NULL, (IPTR) nm);

TREE_SEND_NOTIFY_EXIT:
  if(nm) FreeVec(nm);
}

AROS_UFH2(void, tree_active, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  struct MUI_NListtree_TreeNode *node;
  struct Data *data = (struct Data *) hook->h_Data;

  DebOut("entered (obj %lx)\n", obj);

  node=(struct MUI_NListtree_TreeNode *) XGET((Object *) obj, MUIA_NListtree_Active);

  if(node!=(struct MUI_NListtree_TreeNode *) MUIA_NListtree_Active) {
    tree_send_notify(TVN_SELCHANGED, (Object *) obj, data, (struct ConfigStruct *) node->tn_User);
  }

  AROS_USERFUNC_EXIT
}
 
AROS_UFH2(void, tree_double, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  struct MUI_NListtree_TreeNode *node;
  struct Data *data = (struct Data *) hook->h_Data;

  DebOut("entered (obj %lx)\n", obj);

  node=(struct MUI_NListtree_TreeNode *) XGET((Object *) obj, MUIA_NListtree_Active);

  if(node!=(struct MUI_NListtree_TreeNode *) MUIA_NListtree_Active) {
    tree_send_notify(NM_DBLCLK, (Object *) obj, data, (struct ConfigStruct *) node->tn_User);
  }

  AROS_USERFUNC_EXIT
}
 
/* return the object to be inserted in the app
 * store pointer to nlisttree in &nlisttree
 */
Object *new_tree(ULONG i, void *f, struct Data *data, Object **nlisttree) {

  Object *tree=NULL;
  int *(*func) (Element *hDlg, UINT msg, ULONG wParam, IPTR lParam);
  func=(int* (*)(Element*, uint32_t, ULONG, IPTR)) f;

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
            MUIA_NListview_NList, (*nlisttree=NListtreeObject,
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

  if(!tree) {
    DebOut("ERROR: unable to create tree!\n");
    return NULL;
  }

  DebOut("new tree: %lx\n", tree);
  DebOut("new listtree: %lx\n", *nlisttree);

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

  DoMethod(tree, MUIM_Notify, MUIA_NList_DoubleClick, MUIV_EveryTime, (IPTR) tree, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_tree_double, func); 

#if 0
  Object *img1;

  ULONG QUICKSTART_WIDTH1=32;
  ULONG QUICKSTART_HEIGHT1=16;
  ULONG QUICKSTART_DEPTH1=4;

  img1= BodychunkObject,
    MUIA_FixWidth , QUICKSTART_WIDTH1 ,
    MUIA_FixHeight , QUICKSTART_HEIGHT1,
    MUIA_Bitmap_Width , QUICKSTART_WIDTH1 ,
    MUIA_Bitmap_Height , QUICKSTART_HEIGHT1,
    MUIA_Bodychunk_Depth , QUICKSTART_DEPTH1 ,

    MUIA_Bodychunk_Body , (UBYTE *) QUICKSTART,
    MUIA_Bodychunk_Compression, 0,
    //MUIA_Bodychunk_Masking , 2,
    MUIA_Bitmap_SourceColors , (ULONG *) list2_colors,
    MUIA_Bitmap_Transparent , 0,
  End;
#endif


#if 0
  img1=NBitmapObject,
        MUIA_FixWidth,       QUICKSTART_WIDTH*2,
        MUIA_FixHeight,      QUICKSTART_HEIGHT*2,
        MUIA_NBitmap_Type,   MUIV_NBitmap_Type_ARGB32,
        MUIA_NBitmap_Normal, QUICKSTART,
        MUIA_NBitmap_Width,  QUICKSTART_WIDTH,
        MUIA_NBitmap_Height, QUICKSTART_HEIGHT,
        MUIA_NBitmap_Alpha,  TRUE,
        End;



  /* setup images */
  if(DoMethod(tree, MUIM_NList_UseImage, img1, 0, 0)) {
    DebOut("ERROR: can't create image!\n");
  }
  else {
    DebOut("MUIM_NList_UseImage: SUCCESS!\n");
  }
#endif

  return tree;
}

