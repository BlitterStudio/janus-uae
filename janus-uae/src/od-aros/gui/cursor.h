#ifndef __CURSOR_H__
#define __CURSOR_H__

#include "aros.h"

#define HCURSOR IPTR

#define IDC_ARROW "IDC_ARROW"
#define IDC_WAIT "IDC_WAIT"

HCURSOR LoadCursor(HINSTANCE hInstance, LPCTSTR lpCursorName);
HCURSOR SetCursor(HCURSOR hCursor);

#endif

