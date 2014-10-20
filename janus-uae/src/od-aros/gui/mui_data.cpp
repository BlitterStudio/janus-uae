#include <exec/types.h>
#include <libraries/mui.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "gui.h"


#if 0
Element IDD_FLOPPY[] = {
  { 0, NULL, GROUPBOX,     1,   0, 393, 163, "Floppy Drives", 0 },
  { 0, NULL, CONTROL,      7,  14,  34,  15, "DF0:", 0 },
  //{ 0, NULL, PUSHBUTTON, 69,  13,  78,  15, "Delete save image", 0},
  { 0, NULL, RTEXT,      221,  17,  74,  10, "Write-protected", 0},
  { 0, NULL, PUSHBUTTON, 323,  12,  17,  15, "?", 0},
  { 0, NULL, PUSHBUTTON, 345,  12,  30,  15, "Eject", 0},
  { 0, NULL, PUSHBUTTON, 379,  12,  10,  15, "...", 0},
  { 0, NULL, GROUPBOX,     1, 170, 393,  35, "Floppy Drive Emulation Speed", 0 },
  { 0, NULL, GROUPBOX,     1, 211, 393,  49, "New Floppy Disk Image", 0 },
  { 0, NULL, 0,            0,   0,   0,   0,  NULL, 0 }
};
#endif




Element IDD_FLOPPY[] = {
  { 0, NULL, GROUPBOX   ,   1,   0, 393, 163, "Floppy Drives", 0 },
  { 0, NULL, CONTROL    ,   7,  14,  34,  15, "DF0:", 0 },
  { 0, NULL, PUSHBUTTON ,  69,  13,  78,  15, "Delete save image", 0 },
  { 0, NULL, COMBOBOX   , 152,  14,  65,  15, "IDC_DF0TYPE", 0 },
  { 0, NULL, RTEXT      , 221,  17,  74,  10, "Write-protected", 0 },
  { 0, NULL, CONTROL    , 300,  13,  10,  15, "", 0 },
  { 0, NULL, PUSHBUTTON , 323,  12,  17,  15, "?", 0 },
  { 0, NULL, PUSHBUTTON , 345,  12,  30,  15, "Eject", 0 },
  { 0, NULL, PUSHBUTTON , 379,  12,  10,  15, "...", 0 },
  { 0, NULL, COMBOBOX   ,   6,  31, 384,  15, "IDC_DF0TEXT", 0 },
  { 0, NULL, CONTROL    ,   7,  51,  34,  15, "DF1:", 0 },
  { 0, NULL, PUSHBUTTON ,  69,  49,  78,  15, "Delete save image", 0 },
  { 0, NULL, COMBOBOX   , 152,  51,  65,  15, "IDC_DF1TYPE", 0 },
  { 0, NULL, RTEXT      , 221,  53,  74,  10, "Write-protected", 0 },
  { 0, NULL, CONTROL    , 300,  50,  10,  15, "", 0 },
  { 0, NULL, PUSHBUTTON , 323,  49,  17,  15, "?", 0 },
  { 0, NULL, PUSHBUTTON , 345,  49,  30,  15, "Eject", 0 },
  { 0, NULL, PUSHBUTTON , 379,  49,  10,  15, "...", 0 },
  { 0, NULL, COMBOBOX   ,   6,  68, 383,  15, "IDC_DF1TEXT", 0 },
  { 0, NULL, CONTROL    ,   7,  87,  34,  15, "DF2:", 0 },
  { 0, NULL, PUSHBUTTON ,  69,  85,  78,  15, "Delete save image", 0 },
  { 0, NULL, COMBOBOX   , 152,  87,  65,  15, "IDC_DF2TYPE", 0 },
  { 0, NULL, RTEXT      , 222,  88,  73,  10, "Write-protected", 0 },
  { 0, NULL, CONTROL    , 300,  86,   9,  15, "", 0 },
  { 0, NULL, PUSHBUTTON , 323,  85,  17,  15, "?", 0 },
  { 0, NULL, PUSHBUTTON , 345,  85,  30,  15, "Eject", 0 },
  { 0, NULL, PUSHBUTTON , 379,  85,  10,  15, "...", 0 },
  { 0, NULL, COMBOBOX   ,   6, 104, 384,  15, "IDC_DF2TEXT", 0 },
  { 0, NULL, CONTROL    ,   7, 123,  34,  15, "DF3:", 0 },
  { 0, NULL, PUSHBUTTON ,  69, 121,  78,  15, "Delete save image", 0 },
  { 0, NULL, COMBOBOX   , 152, 123,  65,  15, "IDC_DF3TYPE", 0 },
  { 0, NULL, RTEXT      , 222, 125,  73,  10, "Write-protected", 0 },
  { 0, NULL, CONTROL    , 300, 123,   9,  15, "", 0 },
  { 0, NULL, PUSHBUTTON , 323, 122,  17,  15, "?", 0 },
  { 0, NULL, PUSHBUTTON , 345, 121,  30,  15, "Eject", 0 },
  { 0, NULL, PUSHBUTTON , 379, 121,  10,  15, "...", 0 },
  { 0, NULL, COMBOBOX   ,   6, 140, 383,  15, "IDC_DF3TEXT", 0 },
  { 0, NULL, GROUPBOX   ,   1, 170, 393,  35, "Floppy Drive Emulation Speed", 0 },
  { 0, NULL, CONTROL    ,  71, 180, 116,  20, "", 0 },
  { 0, NULL, GROUPBOX   ,   1, 211, 393,  49, "New Floppy Disk Image", 0 },
  { 0, NULL, COMBOBOX   ,  58, 225,  64,  15, "IDC_FLOPPYTYPE", 0 },
  { 0, NULL, PUSHBUTTON , 130, 224,  97,  15, "Create Standard Disk [] Creates a standard 880 or 1760 KB ADF disk image.", 0 },
  { 0, NULL, PUSHBUTTON , 235, 224, 101,  15, "Create Custom Disk [] Creates a low level (MFM) ADF disk image (about 2MB). Useful for programs that use non-standard disk formats (for example some save disks or DOS-formatted floppies)", 0 },
  { 0, NULL, RTEXT      ,  60, 244,  58,  10, "Disk label:", 0 },
  { 0, NULL, CONTROL    , 235, 242,  59,  15, "Bootblock", 0 },
  { 0, NULL, CONTROL    , 300, 242,  34,  15, "FFS", 0 },
  { 0, NULL, 0,            0,   0,   0,   0,  NULL, 0 }
};
