#ifndef __FS_H__
#define __FS_H__

//#define LPCTSTR TCHAR *
#define PLONG LONG *
#define LPOVERLAPPED APTR

/* taken from https://source.winehq.org/source/include/winbase.h: */

/* The security attributes structure */
typedef struct _SECURITY_ATTRIBUTES {
  DWORD   nLength;
  LPVOID  lpSecurityDescriptor;
  BOOL  bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

#define FILE_BEGIN              0
#define FILE_CURRENT            1
#define FILE_END                2

#define CREATE_NEW              1
#define CREATE_ALWAYS           2
#define OPEN_EXISTING           3
#define OPEN_ALWAYS             4
#define TRUNCATE_EXISTING       5

#define INVALID_HANDLE_VALUE     ((IPTR)0)
#define INVALID_FILE_SIZE        (~0)
#define INVALID_SET_FILE_POINTER (~0)
#define INVALID_FILE_ATTRIBUTES  (~0)

/* winbase end */

#define GENERIC_READ             0x80000000L
#define GENERIC_WRITE            0x40000000L
#define GENERIC_EXECUTE          0x20000000L
#define GENERIC_ALL              0x10000000L

#define NO_ERROR                 0
#define FSCTL_SET_SPARSE         0 /* not possible on AROS */

HANDLE CreateFile( LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD  dwMoveMethod);
BOOL SetEndOfFile(HANDLE hFile);
BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
BOOL CloseHandle(HANDLE hObject);
DWORD GetLastError(void);

/* set to FSCTL_SET_SPARSE not supported.. */
#define DeviceIoControl(...)

#endif

