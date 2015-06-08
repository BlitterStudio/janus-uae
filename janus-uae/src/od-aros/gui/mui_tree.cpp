
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


#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "aros.h"
#include "gui.h"
#include "winnt.h"
#include "mui_data.h"

/* Deletes all items from a tree-view control. 
 * WARNING: nIDDlgItem is not in the Windows API!
 */
BOOL TreeView_DeleteAllItems(HWND hDlg, int nIDDlgItem) {
  DebOut("hDlg %lx, nIDDlgItem %d\n", hDlg, nIDDlgItem);
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

  DoMethod(elem[i].obj, MUIM_NListtree_Insert, lpis->itemex.pszText, NULL,
                        MUIV_NListtree_Insert_ListNode_Root,
                        MUIV_NListtree_Insert_PrevNode_Tail,
                        MUIV_NListtree_Insert_Flag_Active);
  
  // care for hInsertAfter = TVI_ROOT : TVI_SORT .. */
}

BOOL TreeView_DeleteItem(HWND hwndTV, int nIDDlgItem, HTREEITEM hitem) {
  TODO();
}
