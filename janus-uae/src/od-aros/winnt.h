/*
 * winNT status flags
 *
 * not sure, if it is a good idea to use them..
 */

#ifndef __WINNT_H__
#define __WINNT_H__

#define FILE_ATTRIBUTE_READONLY            0x00000001
#define FILE_ATTRIBUTE_HIDDEN              0x00000002
#define FILE_ATTRIBUTE_SYSTEM              0x00000004
#define FILE_ATTRIBUTE_DIRECTORY           0x00000010
#define FILE_ATTRIBUTE_ARCHIVE             0x00000020
#define FILE_ATTRIBUTE_DEVICE              0x00000040
#define FILE_ATTRIBUTE_NORMAL              0x00000080
#define FILE_ATTRIBUTE_TEMPORARY           0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE         0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT       0x00000400
#define FILE_ATTRIBUTE_COMPRESSED          0x00000800
#define FILE_ATTRIBUTE_OFFLINE             0x00001000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED 0x00002000
#define FILE_ATTRIBUTE_ENCRYPTED           0x00004000
#define FILE_ATTRIBUTE_VIRTUAL             0x00010000
#define FILE_ATTRIBUTE_VALID_FLAGS         0x00017fb7
#define FILE_ATTRIBUTE_VALID_SET_FLAGS     0x000031a7 

/* usually, this macro seems to be defined as: */
/* even on 64bit, MAKELONG returns a 32bit value, see:
  https://msdn.microsoft.com/en-us/library/windows/desktop/ms632660.aspx and
  https://msdn.microsoft.com/en-us/library/windows/desktop/aa383751.aspx
 */

#define MAKELONG(a,b) ((LONG)(((WORD)(a))|(((DWORD)((WORD)(b)))<<16)))

/* but for -9, -1 it is *worng*, at least on AROS 32bit? This one works correct: */
//#define MAKELONG(a,b) (a&0xFFFF)|(b&0xFFFF)<<16

#define LOWORD(l) ((WORD)((DWORD)(l)))
#define HIWORD(l) ((WORD)(((DWORD)(l)>>16)&0xFFFF))
#define LOBYTE(w) ((BYTE)(w))
#define HIBYTE(w) ((BYTE)(((WORD)(w)>>8)&0xFF))

#define MAKELPARAM(low,high)   ((LPARAM)(DWORD)MAKELONG(low,high))
#define MAKEWPARAM(low,high)   ((WPARAM)(DWORD)MAKELONG(low,high))
#define MAKELRESULT(low,high)  ((LRESULT)(DWORD)MAKELONG(low,high))i

#define TVIF_TEXT             0x0001
#define TVIF_IMAGE            0x0002
#define TVIF_PARAM            0x0004
#define TVIF_STATE            0x0008
#define TVIF_HANDLE           0x0010
#define TVIF_SELECTEDIMAGE    0x0020
#define TVIF_CHILDREN         0x0040
#define TVIF_INTEGRAL         0x0080
#define TVIF_DI_SETITEM     0x1000

#define TVI_ROOT              ((HTREEITEM)0xffff0000)     /* -65536 */
#define TVI_FIRST             ((HTREEITEM)0xffff0001)     /* -65535 */
#define TVI_LAST              ((HTREEITEM)0xffff0002)     /* -65534 */
#define TVI_SORT              ((HTREEITEM)0xffff0003)     /* -65533 */

#define TVIS_FOCUSED          0x0001
#define TVIS_SELECTED         0x0002
#define TVIS_CUT              0x0004
#define TVIS_DROPHILITED      0x0008
#define TVIS_BOLD             0x0010
#define TVIS_EXPANDED         0x0020
#define TVIS_EXPANDEDONCE     0x0040
#define TVIS_EXPANDPARTIAL    0x0080
#define TVIS_OVERLAYMASK      0x0f00
#define TVIS_STATEIMAGEMASK   0xf000
#define TVIS_USERMASK         0xf000

#endif /* __WINNT_H__ */
