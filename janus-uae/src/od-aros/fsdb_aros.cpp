/************************************************************************ 
 *
 * AROS specific file system functions
 *
 * Copyright 2015       Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE.
 *
 * Janus-UAE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Janus-UAE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id$
 *
 ************************************************************************/

#define DEBUG 1
#include <stdio.h>
#include <proto/dos.h>
#include <dos/dos.h>

#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "zfile.h"
#include "options.h"
#include "fsdb.h"
#include "winnt.h"

#define MAXFILESIZE32 (0x7fffffff)

/* why is this not defined in dos.h? See Protect source in AROS svn */
#ifndef FIBB_HOLD
#define FIBB_HOLD 7
#endif
#ifndef FIBF_HOLD
#define FIBF_HOLD (1<<FIBB_HOLD)
#endif


/******************************************************************
 * SetLastError / GetLastError
 *
 * simulate Windows Error Code handling
 ******************************************************************/
static int my_last_error=0;

int GetLastError(void) {
  return my_last_error;
}
void SetLastError(int err) {
  my_last_error=err;
  return;
}

/*
 * we use host system error codes, so just return last error code here,
 * no translation necessary as on windows hosts
 */
int dos_errno (void) {

  return GetLastError();
}

/******************************************************************
 * my_* 
 ******************************************************************/
int my_existsfile (const TCHAR *name) {

	struct FileInfoBlock fib;
	BPTR lock;
	int res=0;

    bug("[JUAE:A-FSDB] %s('%s')\n", __PRETTY_FUNCTION__, name);


	lock=Lock((CONST_STRPTR) name, SHARED_LOCK);
	if(!lock) {
		return 0;
	}

	res=0;
	if(Examine(lock, &fib)) {
		if(fib.fib_DirEntryType < 0) {
			res=1;
		}
	}

	UnLock(lock);

	return res;
}

/* should open a textfile according to its encoding. (UTF-8, UTF-16LE or ascii)
 * on AROS we do not support UTF
 */
FILE *my_opentext (const TCHAR *name) {

	FILE *f;
	uae_u8 tmp[5];
	int v;

    bug("[JUAE:A-FSDB] %s('%s')\n", __PRETTY_FUNCTION__, name);

	f = fopen (name, "rb");
	if (!f) {
                bug("[JUAE:A-FSDB] %s: failed\n", __PRETTY_FUNCTION__);

		return NULL;
	}
	v = fread (tmp, 1, 4, f);
	fclose (f);
	if (v == 4) {
		if (tmp[0] == 0xef && tmp[1] == 0xbb && tmp[2] == 0xbf) {
			/* return fopen (name, L"r, ccs=UTF-8"); */
                        bug("[JUAE:A-FSDB] %s: cant open UTF8 file\n", __PRETTY_FUNCTION__);

			fprintf(stderr, "ERROR: tried to open UTF-8 file %s!\n", name);
			return NULL;
		}
		if (tmp[0] == 0xff && tmp[1] == 0xfe) {
			/* return fopen (name, L"r, ccs=UTF-16LE"); */
			bug("[JUAE:A-FSDB] %s: cant open UTF16LE file\n", __PRETTY_FUNCTION__);

			fprintf(stderr, "ERROR: tried to open UTF-16LE file %s!\n", name);
			return NULL;
		}

	}
	return fopen (name, "r");
}


/*
istruct mystat^M
{^M
  uae_s64 size;^M
    uae_u32 mode;^M
      struct mytimeval mtime;^M
};^M
*/

bool my_stat (const TCHAR *name, struct mystat *statbuf) {
  struct FileInfoBlock *fib=NULL;
  BPTR lock=NULL;
  bool result=FALSE;

  D(bug("[JUAE:A-FSDB] %s('%s')\n", __PRETTY_FUNCTION__, name));

  lock=Lock(name, SHARED_LOCK);
  if(!lock) {
    bug("[JUAE:A-FSDB] %s: failed to lock entry %s\n", __PRETTY_FUNCTION__, name);
    goto exit;
  }

  if(!(fib=(struct FileInfoBlock *) AllocDosObject(DOS_FIB, NULL)) || !Examine(lock, fib)) {
    bug("[JUAE:A-FSDB] %s: failed to examine lock @ 0x%p [fib @ 0x%p]\n", __PRETTY_FUNCTION__, lock, fib);
    goto exit;
  }

  statbuf->size=fib->fib_Size;

  statbuf->mode=0;
  if(fib->fib_DiskKey) {
    D(bug("[JUAE:A-FSDB] %s: %s FILEFLAG_DIR\n", __PRETTY_FUNCTION__, name));
    statbuf->mode |= FILEFLAG_DIR;
  }

#ifdef DEBUG
  {
    char flags[8];
    flags[0] = fib->fib_Protection & FIBF_SCRIPT  ? 's' : '-';
    flags[1] = fib->fib_Protection & FIBF_PURE    ? 'p' : '-';
    flags[2] = fib->fib_Protection & FIBF_ARCHIVE ? 'a' : '-';

    // Active when unset!
    flags[3] = fib->fib_Protection & FIBF_READ    ? '-' : 'r';
    flags[4] = fib->fib_Protection & FIBF_WRITE   ? '-' : 'w';
    flags[5] = fib->fib_Protection & FIBF_EXECUTE ? '-' : 'e';
    flags[6] = fib->fib_Protection & FIBF_DELETE  ? '-' : 'd';
    flags[7] = 0x00;
    bug("[JUAE:A-FSDB] %s: %s protection: %s\n", __PRETTY_FUNCTION__, name, flags);
  }
#endif

  /* fib_Protection: active-low, i.e. not set means active! (who invented this !??) */
  if(!(fib->fib_Protection & FIBF_READ))    {
    statbuf->mode |= FILEFLAG_READ;
  }
  if(!(fib->fib_Protection & FIBF_WRITE))   {
    statbuf->mode |= FILEFLAG_WRITE;
  }
  if(!(fib->fib_Protection & FIBF_EXECUTE)) {
    statbuf->mode |= FILEFLAG_EXECUTE;
  }
   /* FIBF_ARCHIVE, FIBF_PURE and FIBF_SCRIPT are active when set */
  if(fib->fib_Protection & FIBF_ARCHIVE) {
    statbuf->mode |= FILEFLAG_ARCHIVE;
  }
  if(fib->fib_Protection & FIBF_PURE)    {
    statbuf->mode |= FILEFLAG_PURE;
  }
  if(fib->fib_Protection & FIBF_SCRIPT)  {
    statbuf->mode |= FILEFLAG_SCRIPT;
  }

  amiga_to_timeval(&statbuf->mtime, fib->fib_Date.ds_Days, fib->fib_Date.ds_Minute, fib->fib_Date.ds_Tick);

  result=TRUE;

exit:
  if(lock) UnLock(lock);
  if(fib) FreeDosObject(DOS_FIB, fib);

  return result;
}

/******************************************************************
 * my_getvolumeinfo (also works for directories on AROS)
 *
 * 
 * return FLAGS:
 *  -1: error
 *  MYVOLUMEINFO_READONLY
 *
 *  all other flags not supported, as they are not used anywhere
 * 
 ******************************************************************/
int my_getvolumeinfo (const TCHAR *name) {

  struct FileInfoBlock *fib;
  BPTR file=NULL;
  int ret=-1;

    bug("[JUAE:A-FSDB] %s('%s')\n", __PRETTY_FUNCTION__, name);

  fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, TAG_END);
  if(!fib) 
  {
    bug("[JUAE:A-FSDB] %s: failed to allocate FIB\n", __PRETTY_FUNCTION__);

    goto EXIT;
  }

  if(!(file=Lock(name, SHARED_LOCK))) 
  {
    bug("[JUAE:A-FSDB] %s: failed to lock entry\n", __PRETTY_FUNCTION__);

    bug("no lock..\n");
    goto EXIT;
  }

  if (!Examine(file, fib)) 
  {
    bug("[JUAE:A-FSDB] %s: failed to examine\n", __PRETTY_FUNCTION__);

    goto EXIT;
  }

  bug("[JUAE:A-FSDB] %s: fib prot flags 0x%lx\n", __PRETTY_FUNCTION__, fib->fib_Protection);

  if(fib->fib_DirEntryType >0) 
  {
    bug("[JUAE:A-FSDB] %s: DIRECTORY!\n", __PRETTY_FUNCTION__);
    ret=0;
  }

  if(!(fib->fib_Protection && FIBF_WRITE)) {
    bug("[JUAE:A-FSDB] %s: read only entry!\n", __PRETTY_FUNCTION__);

    ret=MYVOLUMEINFO_READONLY;
  }

EXIT:
  if(fib) 
  {
    FreeDosObject(DOS_FIB, fib);
  }
  if(file) 
  {
    UnLock(file);
  }

  return ret;
}

/******************************************************************
 * my_existsdir
 *
 * Return 1, if name is a valid directory. 0 otherwise.
 ******************************************************************/
int my_existsdir (const TCHAR *name) {
  int t;

    bug("[JUAE:A-FSDB] %s('%s')\n", __PRETTY_FUNCTION__, name);

  t=my_getvolumeinfo(name);

  if(t>=0) {
    return 1;
  }

  return 0;
}

/******************************************************************
 ******************************************************************/
void fullpath(TCHAR *linkfile, int size);

bool my_resolvesoftlink(TCHAR *linkfile, int size) {

  struct mystat statbuf;

  DebOut("linkfile: %s\n", linkfile);

  if(!fsdb_exists(linkfile)) {
    DebOut("%s does not exist!\n", linkfile);
    return FALSE;
  }

  fullpath(linkfile, size);

  DebOut("return : %s\n", linkfile);

  return TRUE;
}

/******************************************************************
 * int my_*dir (struct my_opendir_s *mod, TCHAR *name)
 *
 * These functions are used to open and scan a directory.
 ******************************************************************/

struct my_opendir_s {
  struct FileInfoBlock *h;
  BPTR lock;
};

struct my_openfile_s {
  BPTR lock;
};

/* return next dir entry */
int my_readdir (struct my_opendir_s *mod, TCHAR *name) {

  if(ExNext(mod->lock, mod->h)) {
    strcpy(name, (TCHAR *) mod->h->fib_FileName);
    DebOut("name: %s\n", name);

    return TRUE;
  }

  DebOut("no more files/error!\n");

  /* no more entries */
  return FALSE;
}

void my_closedir (struct my_opendir_s *mod) {

  if(mod->h) FreeDosObject(DOS_FIB, mod->h);
  if(mod->lock) UnLock(mod->lock);
  if(mod) free(mod);
}

struct my_openfile_s *my_open (const TCHAR *name, int flags) {
  struct my_openfile_s *mos;
  BPTR h;
  TCHAR path[MAX_DPATH];
  LONG access_mode=MODE_OLDFILE;


  mos=(struct my_openfile_s *) AllocVec(sizeof(struct my_openfile_s), MEMF_CLEAR);
  if (!mos) return NULL;

  DebOut("mos: %lx  name: %s, flags: 0x%lx\n",mos ,name, flags);

  /* exactly one of the first three values */
  if (flags & O_RDONLY) {
    DebOut("O_RDONLY\n");
    access_mode=MODE_OLDFILE;
  }
  else if(flags & O_WRONLY) {
    DebOut("MODE_READWRITE\n");
    access_mode=MODE_READWRITE;
  }
  else if(flags & O_RDWR) {
    DebOut("MODE_READWRITE\n");
    access_mode=MODE_READWRITE;
  }
  else {
    DebOut("ERROR: strange flags!?\n");
  }

  if(flags & O_TRUNC)  {
    DebOut("O_TRUNC => MODE_NEWFILE\n");
    access_mode=MODE_NEWFILE;
  }

  h=Open(name, access_mode);

  if(!h) {
    DebOut("Could not open %s\n", name);
    FreeVec(mos);
    return NULL;
  }

  mos->lock=h;
  return mos;
}

void my_close (struct my_openfile_s *mos) {

  DebOut("mos: %lx\n", mos);

  if(!mos) {
    bug("[JUAE:A-FSDB] %s(): WARNING: NULL pointer received !?\n", __PRETTY_FUNCTION__);
    return;
  }

  if(mos->lock) {
    Close(mos->lock);
    mos->lock=NULL;
  }

  FreeVec(mos);
  return;
}

uae_s64 int my_lseek (struct my_openfile_s *mos, uae_s64 int offset, int whence) {
  uae_s64 int ret;
  LONG old;

  DebOut("mos: %lx offset: %d whence: %d\n", mos, offset, whence);

  if(!mos->lock) {
    DebOut("ERROR: no lock!\n");
    exit(1);
  }

  switch(whence) {
    case SEEK_CUR:
      return (uae_s64 int) Seek(mos->lock, offset, OFFSET_CURRENT);
    case SEEK_SET:
      return (uae_s64 int) Seek(mos->lock, offset, OFFSET_BEGINNING);
    case SEEK_END:
      return (uae_s64 int) Seek(mos->lock, offset, OFFSET_END);
  }

  DebOut("ERROR: unknown whence %d\n", whence);
  return -1;
}

unsigned int my_read (struct my_openfile_s *mos, void *b, unsigned int size) {

  LONG r;
  DebOut("mos: %lx\n", mos);

  if(!mos->lock) {
    DebOut("ERROR: no lock!\n");
    exit(1);
  }

  return Read(mos->lock, b, size);
}

unsigned int my_write (struct my_openfile_s *mos, void *b, unsigned int size) {

  DebOut("mos: %lx, b: %lx, size: %d\n", mos, b, size);

  if(!mos->lock) {
    DebOut("ERROR: no lock!\n");
    exit(1);
  }

  return Write(mos->lock, b, size);
}


/** WARNING: mask does *not* work !! */
struct my_opendir_s *my_opendir (const TCHAR *name, const TCHAR *mask) {
  struct my_opendir_s *mod;
  TCHAR tmp[MAX_DPATH];
  unsigned int len=strlen(name);

  DebOut("name: %s, mask: %s\n", name, mask);

  tmp[0] = 0;
#if 0
  if (currprefs.win32_filesystem_mangle_reserved_names == false)
    _tcscpy (tmp, PATHPREFIX);
  _tcscat (tmp, name);
  DebOut("lastchar: %c\n", name[len-1]);
  if(! (name[len-1]=='/' || name[len-1]==':')) {
    _tcscat (tmp, _T("/"));
  }
  _tcscat (tmp, mask);
  DebOut("tmp: %s\n", tmp);
#else
  strcpy(tmp, name);
#endif

  mod = xmalloc (struct my_opendir_s, 1);
  if(!mod) return NULL;

  mod->h=(struct FileInfoBlock *) AllocDosObject(DOS_FIB, TAG_END);
  if(!mod->h) goto ERROR;

  mod->lock=Lock(tmp, ACCESS_READ); /* TODO: ACCESS_READ or ACCESS_WRITE!? */
  if(!mod->lock) {
    DebOut("unable to lock: %s\n", tmp);
    goto ERROR;
  }

  if(!Examine(mod->lock, mod->h)) {
    DebOut("Examine failed!\n");
    goto ERROR;
  }

  if(!(mod->h->fib_DirEntryType > 0 )) {
    DebOut("%s is NOT a directory!\n", tmp);
    goto ERROR;
  }

  DebOut("ok\n");

  return mod;
ERROR:

  my_closedir(mod);
  return NULL;
}

struct my_opendir_s *my_opendir (const TCHAR *name) {

  return my_opendir (name, "");
}

/* TODO: 64bit sizes !? */
uae_s64 int my_fsize (struct my_openfile_s *mos) {

  struct FileInfoBlock * fib=NULL;
  BPTR lock=NULL;
  uae_s64 size;

  if(!(fib=(struct FileInfoBlock *) AllocDosObject(DOS_FIB, NULL)) || !Examine(lock, fib)) {
    bug("[JUAE:A-FSDB] %s: failed to examine lock @ 0x%p [fib @ 0x%p]\n", __PRETTY_FUNCTION__, lock, fib);
    size=-1;
    goto EXIT;
  }

  /* test for 64 bit filesize */
  if(fib->fib_Size >= 0x7FFFFFFF) {

    bug("[JUAE:A-FSDB] %s: WARNING: filesize >2GB detected. This has never been tested!\n", __PRETTY_FUNCTION__);
    UQUAD *size_ptr=(UQUAD *)DoPkt(((struct FileLock *)lock)->fl_Task, ACTION_GET_FILE_SIZE64, (IPTR)lock, 0, 0, 0, 0);
    if (size_ptr) {
      size=(uae_s64) *size_ptr;
    }
    else {
      bug("[JUAE:A-FSDB] %s: ERROR: DoPkt return NULL!\n");
      size=-1;
    }
    goto EXIT;
  }

  /* extend 32-bit */
  size=(uae_s64) fib->fib_Size;
  
EXIT:
  DebOut("size: %d\n", size);
  FreeDosObject(DOS_FIB, fib);

  return size;
}

bool my_utime (const TCHAR *name, struct mytimeval *tv) {

  struct DateStamp stamp;

  DebOut("name: %s\n", name);

  if(!tv) {
    /* get current time */
    struct DateTime dt;
    DateStamp(&dt.dat_Stamp);
    stamp.ds_Days   = dt.dat_Stamp.ds_Days;
    stamp.ds_Minute = dt.dat_Stamp.ds_Minute;
    stamp.ds_Tick   = dt.dat_Stamp.ds_Tick;
  }
  else {
    /* use supplied time */
    /* NOT TESTED! */
    struct mytimeval tv2;
    tv2.tv_sec = tv->tv_sec;
    tv2.tv_usec = tv->tv_usec;

    timeval_to_amiga (&tv2, &stamp.ds_Days, &stamp.ds_Minute, &stamp.ds_Tick);
  }

  DebOut("stamp.ds_Days: %d\n", stamp.ds_Days);
  DebOut("stamp.ds_Minute: %d\n", stamp.ds_Minute);
  DebOut("stamp.ds_Tick: %d\n", stamp.ds_Tick);

  return SetFileDate(name, &stamp);
}

/******************************************************************
 * my_mkdir / my_rmdir
 *
 * Create/Delete directories. Return AROS host error codes for 
 * AmigaOS guest CreateDir/DeleteFile calls.
 ******************************************************************/
int my_mkdir (const TCHAR *name) {

  BPTR lock;

  DebOut("name: %s\n", name);

  lock=CreateDir(name);

  if(lock) {
    UnLock(lock); /* CreateDir returns a lock, which we don't need */
    return 0;
  }

  SetLastError(IoErr());
  DebOut("return -1 (%d)\n", IoErr());
  return -1;
}

int my_rmdir (const TCHAR *name) {

  BOOL res;

  DebOut("name: %s\n", name);

  res=DeleteFile(name);

  if(res) {
    return 0;
  }

  SetLastError(IoErr());
  DebOut("return -1 (%d)\n", IoErr());
  return -1;
}

/******************************************************************
 * my_rename
 ******************************************************************/
int my_rename (const TCHAR *oldname, const TCHAR *newname) {

  LONG res;

  DebOut("oldname: %s newname %s\n", oldname, newname);

  res=Rename(oldname, newname);

  if(res==DOSTRUE) {
    return 0;
  }

  SetLastError(IoErr());
  DebOut("return -1 (%d)\n", IoErr());
  return -1;
}

/******************************************************************
 * my_unlink: delete file 
 ******************************************************************/
int my_unlink (const TCHAR *name) {

  BOOL res;

  DebOut("name: %s\n", name);

  res=DeleteFile(name);

  if(res!=0) {
    return 0;
  }

  SetLastError(IoErr());
  DebOut("return -1 (%d)\n", IoErr());
  return -1;
}

/******************************************************************
 * my_truncate
 *
 * is used to implement SetFileSize. Would be nicer, if 
 * ACTION_SET_FILE_SIZE would call host SetFileSize directly,
 * but we would have to modify ../filesys.cpp, which I wanted
 * to avoid so far..
 *
 * WARNING: SetFileSize seems to be broken, at least for
 *          AROS/hosted filesystems !? Even the test
 *          in the AROS svn fails..
 ******************************************************************/
int my_truncate (const TCHAR *name, uae_u64 len) {

  LONG newsize;
  BPTR fh;
  int ret=0;

  DebOut("name: %s, len: %d\n", name, len);

  if(len>MAXFILESIZE32) {
    DebOut("filesize is >2GB!\n");
    return -1;
  }

  fh=Open(name, MODE_READWRITE);
  if(!fh) {
    SetLastError(IoErr());
    DebOut("return -1 (%d)\n", IoErr());

    return -1;
  }

#warning SetFileSize might be broken..
  newsize=SetFileSize(fh, (LONG) len, OFFSET_BEGINNING);

  DebOut("newsize: %d\n", newsize);

  if(newsize==-1) {
    SetLastError(IoErr());
    DebOut("return -1 (%d)\n", IoErr());
    ret=-1;
  }

  if(fh) {
    Close(fh);
  }

  return ret;
}

/******************************************************************
 * my_isfilehidden
 *
 * Hidden is *wrong* here. AmigaOS just knows "Hold"!
 ******************************************************************/
bool my_isfilehidden (const TCHAR *name) {

  struct FileInfoBlock *fib=NULL;
  BPTR lock=NULL;
  bool result=FALSE;

  D(bug("[JUAE:A-FSDB] %s('%s')\n", __PRETTY_FUNCTION__, name));

  lock=Lock(name, SHARED_LOCK);
  if(!lock) {
    bug("[JUAE:A-FSDB] %s: failed to lock entry %s\n", __PRETTY_FUNCTION__, name);
    goto exit;
  }

  if(!(fib=(struct FileInfoBlock *) AllocDosObject(DOS_FIB, NULL)) || !Examine(lock, fib)) {
    bug("[JUAE:A-FSDB] %s: failed to examine lock @ 0x%p [fib @ 0x%p]\n", __PRETTY_FUNCTION__, lock, fib);
    goto exit;
  }

  if(fib->fib_Protection & FIBF_HOLD) {
    result=TRUE;
  }

exit:
  if(lock) UnLock(lock);
  if(fib) FreeDosObject(DOS_FIB, fib);

  return result;
}

void my_setfilehidden (const TCHAR *name, bool hidden) {

  LONG mask;
  struct FileInfoBlock *fib=NULL;
  BPTR lock=NULL;

  D(bug("[JUAE:A-FSDB] %s('%s')\n", __PRETTY_FUNCTION__, name));

  lock=Lock(name, SHARED_LOCK);
  if(!lock) {
    bug("[JUAE:A-FSDB] %s: failed to lock entry %s\n", __PRETTY_FUNCTION__, name);
    goto exit;
  }

  if(!(fib=(struct FileInfoBlock *) AllocDosObject(DOS_FIB, NULL)) || !Examine(lock, fib)) {
    bug("[JUAE:A-FSDB] %s: failed to examine lock @ 0x%p [fib @ 0x%p]\n", __PRETTY_FUNCTION__, lock, fib);
    goto exit;
  }

  mask=fib->fib_Protection | FIBF_HOLD;
  if(!SetProtection(name, mask)) {
    bug("ERROR: could not set %s to FIBF_HOLD!\n", name);
  }

exit:
  if(lock) UnLock(lock);
  if(fib) FreeDosObject(DOS_FIB, fib);

  return;
}

/****************************************************************
 * my_issamevolume
 *
 * return FALSE, if one path is not inside the other:
 *
 * SYS:s and SYS:c are not the same path!
 * SYS:c/test and SYS:c *are*!
 *
 * return remaining different part in path:
 *
 * SYS:c and SYS:  return "c"
 *
 * and the number of directory levels as a return code..
 *
 * WARNING: This has never been tested! See od-win32
 ****************************************************************/
int my_issamevolume(const TCHAR *path1, const TCHAR *path2, TCHAR *path) {
  TCHAR p1[MAX_DPATH];
  TCHAR p2[MAX_DPATH];
  int len, cnt;

  DebOut("WARNING: This has never been tested!\n");
  DebOut("path1: %s, path2: %s\n", path1, path2);

  strncpy(p1, path1, MAX_DPATH);
  strncpy(p2, path2, MAX_DPATH);
  fullpath(p1, MAX_DPATH);
  fullpath(p2, MAX_DPATH);

  DebOut("p1: %s, p2: %s\n", p1, p2);

  len=_tcslen (p1);
  if (len>_tcslen (p2)) {
    len=_tcslen (p2);
  }

  /* len: MIN(len1, len2) */

  if(!same_aname(p1, p2)) {
    DebOut("not same volume: %s and %s\n", path1, path2);
    return 0;
  }

  _tcscpy (path, p2 + len);
  cnt=0;
  for (int i = 0; i < _tcslen (path); i++) {
    if(path[i] == '/') {
      cnt++;
    }
  }

  write_log (_T("'%s' (%s) matched with '%s' (%s), extra = '%s'\n"), path1, p1, path2, p2, path);
  return cnt;
}

/******************************************************************
 * my_chmod
 *
 * in win32 this can set the winnt flags:
 * FILE_ATTRIBUTE_NORMAL, FILE_ATTRIBUTE_READONLY and 
 * FILE_ATTRIBUTE_ARCHIVE
 *
 * possible input is: FILEFLAG_WRITE and FILEFLAG_ARCHIVE
 ******************************************************************/
bool my_chmod (const TCHAR *name, uae_u32 mode) {

  ULONG attr=FIBF_READ|FIBF_DELETE; /* default */

  DebOut("WARNING: This has never been tested!\n");

  if (mode & FILEFLAG_WRITE) {
    attr |= FIBF_WRITE;
  }
  if (mode & FILEFLAG_ARCHIVE) {
    attr |= FIBF_ARCHIVE;
  }

  if(!SetProtection(name, attr)) {
    return false;
  }

  return true;
}

/******************************************************************
 ******************* fsdb_* functions *****************************
 ******************************************************************/
int fsdb_exists (const TCHAR *nname) {
  BPTR lock;

  bug("[JUAE:A-FSDB] %s('%s')\n", __PRETTY_FUNCTION__, nname);

  lock=Lock(nname, SHARED_LOCK);
  if(lock) {
    UnLock(lock);
    return TRUE;
  }

  return FALSE;
}


/******************************************************************
 * name_invalid
 *
 * as both host and guest are "amigaoid", filenames should always
 * be valid.
 ******************************************************************/
int fsdb_name_invalid (const TCHAR *n) {

  return FALSE;
}

int fsdb_name_invalid_dir (const TCHAR *n) {

  return FALSE;
}


/******************************************************************
 * fsdb_fill_file_attrs
 *
 * For an a_inode we have newly created based on a filename we 
 * found on the native fs, fill in information about this 
 * file/directory.
 *
 ******************************************************************/
int fsdb_fill_file_attrs (a_inode *base, a_inode *aino) {

  struct FileInfoBlock *fib=NULL;
  BPTR lock=NULL;
  int ret=0;

  //DebOut("%lx %lx\n", base, aino);

  //DebOut("aino->dir: %s\n", aino->dir);
  DebOut("aino->nname: %s\n", aino->nname);

  fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, TAG_END);
  if(!fib) goto ERROR;

  lock=Lock(aino->nname, SHARED_LOCK);
  if(!lock) goto ERROR;

  if (!Examine(lock, fib)) goto ERROR;

  if(fib->fib_DirEntryType>0) {
    //DebOut("=> directory\n");
    aino->dir=1;
    if(fib->fib_DirEntryType==ST_SOFTLINK) {
      DebOut("WARNING: not tested: SoftLink detected!\n");
      aino->softlink=2; /* 1 hardlink, 2 softlink ? */
    }
  }

  if(fib->fib_Comment) {
    //DebOut("comment: >%s<\n", fib->fib_Comment);
    aino->comment=(TCHAR *) strndup((const char *)fib->fib_Comment, 79); /* maximum length is 79 chars,
                                                                            0 will be added automatically */
  }

  aino->amigaos_mode=fib->fib_Protection;
  //DebOut("protection: %04x\n", fib->fib_Protection);

  /* !? */
#if 0
  if (reset && (base->volflags & MYVOLUMEINFO_STREAMS)) {
    create_uaefsdb (aino, fsdb, mode);
    write_uaefsdb (aino->nname, fsdb);
  }
#endif
    
  ret=1;

#warning TODO?: care for base and host changes! 

ERROR:
  if(fib) FreeDosObject(DOS_FIB, fib);
  if(lock) UnLock(lock);

  return ret;
}

/* Return nonzero if we can represent the amigaos_mode of AINO within the
 * native FS.  Return zero if that is not possible.  */
int fsdb_mode_representable_p (const a_inode *aino, int amigaos_mode) {

  /* trivial. Our guest OS is AROS, so yes, we can ;) */
  return 1;
}

/* return supported combination */
int fsdb_mode_supported (const a_inode *aino) {

  int mask = aino->amigaos_mode;

  DebOut("aino: %lx\n", aino);

  if (0 && aino->dir)
    return 0;
  if (fsdb_mode_representable_p (aino, mask))
    return mask;
  mask &= ~(A_FIBF_SCRIPT | A_FIBF_READ | A_FIBF_EXECUTE);
  if (fsdb_mode_representable_p (aino, mask))
    return mask;
  mask &= ~A_FIBF_WRITE;
  if (fsdb_mode_representable_p (aino, mask))
    return mask;
  mask &= ~A_FIBF_DELETE;
  if (fsdb_mode_representable_p (aino, mask))
    return mask;
  return 0;
}

int fsdb_set_file_attrs (a_inode *aino) {

  BOOL res;

  DebOut("name: aino->nname %s\n", aino->nname);

  res=SetProtection(aino->nname, aino->amigaos_mode);

  if(res) {
    return 0;
  }

  return ERROR_OBJECT_NOT_AROUND;
}
