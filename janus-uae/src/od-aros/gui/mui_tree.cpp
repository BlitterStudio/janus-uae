
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

HTREEITEM TreeView_InsertItem(HWND hwnd, int nIDDlgItem, LPTVINSERTSTRUCT lpis) {
  TODO();
}

BOOL TreeView_DeleteItem(HWND hwndTV, int nIDDlgItem, HTREEITEM hitem) {
  TODO();
}
