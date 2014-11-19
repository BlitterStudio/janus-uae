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

#define MAKELONG(a,b) ((LONG)(((WORD)(a))|(((DWORD)((WORD)(b)))<<16)))

#define LOWORD(l) ((WORD)((DWORD)(l)))
#define HIWORD(l) ((WORD)(((DWORD)(l)>>16)&0xFFFF))
#define LOBYTE(w) ((BYTE)(w))
#define HIBYTE(w) ((BYTE)(((WORD)(w)>>8)&0xFF))

#define MAKELPARAM(low,high)   ((LPARAM)(DWORD)MAKELONG(low,high))
#define MAKEWPARAM(low,high)   ((WPARAM)(DWORD)MAKELONG(low,high))
#define MAKELRESULT(low,high)  ((LRESULT)(DWORD)MAKELONG(low,high))i
#endif /* __WINNT_H__ */
