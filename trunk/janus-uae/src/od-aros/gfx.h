#include <intuition/intuition.h>

extern struct Window *hAmigaWnd;

extern void enumeratedisplays (int);

/* in od-win/dxwrap.h */
#define MAX_DISPLAYS 10

struct MultiDisplay {
  int primary, disabled, gdi;
//  GUID guid;
  TCHAR *name, *name2, *name3;
  struct PicassoResolution *DisplayModes;
  RECT rect;
};

extern struct MultiDisplay Displays[MAX_DISPLAYS];

