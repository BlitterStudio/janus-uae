#include <exec/types.h>
#include <libraries/mui.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "gui.h"

#define IDC_SETTINGSTEXT3_text "Floppy Drives"
const char IDC_DF0ENABLE_text[] = "DF0:";
const char *IDC_SETTINGSTEXT2_text = "Floppy Drive Emulation Speed";

#if 0
struct Element IDD_FLOPPY[] = {
  { 0, NULL, CONTROL,   7,  14,  34,  15, "DF0:", 0 },
  { 0, NULL, GROUPBOX,  1,   0, 393, 163, IDC_SETTINGSTEXT3_text, 0 },
  { 0, NULL, GROUPBOX,  1, 170, 393,  35, IDC_SETTINGSTEXT2_text, 0 }
};
#endif
