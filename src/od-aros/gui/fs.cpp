#define JUAE_DEBUG

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <exec/types.h>
 
#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "aros.h"
#include "winnt.h"
#include "fs.h"


/*
 * Creates or opens a file or I/O device. 
 */
HANDLE CreateFile( LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {

  FILE *file=NULL;

  DebOut("lpFileName: %s\n", lpFileName);

  if(dwDesiredAccess!=GENERIC_WRITE || dwCreationDisposition!=CREATE_ALWAYS || dwFlagsAndAttributes!=FILE_ATTRIBUTE_NORMAL) {
    TODO();
  }

  file=fopen(lpFileName, "w" );
  if(file==NULL) {
    DebOut("ERROR opening %s for writing\n", lpFileName);
    return 0;
  }

  return file;
}

/*
 * Writes data to the specified file or input/output (I/O) device.
 */
BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {

  LONG written;

  DebOut("write %d bytes to file handle %lx\n", nNumberOfBytesToWrite, hFile);

  written=write((IPTR) hFile, lpBuffer, nNumberOfBytesToWrite);

  if(written==-1) {
    return FALSE;
  }

  *lpNumberOfBytesWritten=written;

  return TRUE;
}

/* not needed. Just flushes file to disk */
BOOL SetEndOfFile(HANDLE hFile) {
  ;
}

/*
 * Moves the file pointer of the specified file.
 */

#define SPACE_BLOCK 1024*1024

DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD  dwMoveMethod) {

  LONG mode;
  LONG res;
  IPTR t;
  IPTR act_size;
  IPTR start_pos;
  void *ptr;
  LONG remaining;

  if(lpDistanceToMoveHigh) {
    DebOut("ERROR: no 64bit support so far!\n");
    return 0;
  }

  DebOut("hFile %lx, lDistanceToMove: %d\n", lDistanceToMove);

  switch(dwMoveMethod) {
    case FILE_BEGIN:
      break;
    default:
      DebOut("dwMoveMethod: %d not supported!\n", dwMoveMethod);
      TODO();
      return 0;
  }

  start_pos=ftell((FILE *) hFile);
  act_size=fseek((FILE *) hFile, 0, SEEK_END);
  DebOut("old size: %d\n", act_size);

  if(act_size!=lDistanceToMove) {
    ptr=calloc(1, SPACE_BLOCK);
    if(ptr) {
      remaining=lDistanceToMove;
      while(remaining > SPACE_BLOCK) {
        fwrite(ptr, SPACE_BLOCK, 1, (FILE *) hFile);
        remaining=remaining-SPACE_BLOCK;
      }
      fwrite(ptr, remaining, 1, (FILE *) hFile);
    }
    free(ptr);
  }

  rewind((FILE *)hFile);
  act_size=fseek((FILE *) hFile, 0, SEEK_END);
  DebOut("new size: %d\n", act_size);

  return 0;
}

BOOL CloseHandle(HANDLE hFile) {

  fclose((FILE *) hFile);

  return TRUE;
}
