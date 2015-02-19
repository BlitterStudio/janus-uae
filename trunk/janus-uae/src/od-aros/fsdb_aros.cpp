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

#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "zfile.h"
#include "options.h"
#include "fsdb.h"
#include "winnt.h"

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

/*
istruct mystat^M
{^M
  uae_s64 size;^M
    uae_u32 mode;^M
      struct mytimeval mtime;^M
};^M
*/

bool my_stat (const TCHAR *name, struct mystat *statbuf) {
  struct FileInfoBlock *fib;
  BPTR lock;
  bool result=FALSE;

  D(bug("[JUAE:A-FSDB] %s('%s')\n", __PRETTY_FUNCTION__, name));
  D(bug("[JUAE:A-FSDB] %s: statbuf @ 0x%p\n", __PRETTY_FUNCTION__, statbuf));

  lock=Lock(name, SHARED_LOCK);
  if(!lock) {
    bug("[JUAE:A-FSDB] %s: failed to lock entry %s\n", __PRETTY_FUNCTION__, name);
    goto exit;
  }

  if(!(fib=(struct FileInfoBlock *) AllocDosObject(DOS_FIB, NULL)) || !Examine(lock, fib)) {
    bug("[JUAE:A-FSDB] %s: failed to examine lock @ 0x%p [fib @ 0x%p]\n", __PRETTY_FUNCTION__, lock, fib);
    goto exit;
  }

  D(bug("[JUAE:A-FSDB] %s: fib_FileName %s\n", __PRETTY_FUNCTION__, fib->fib_FileName));
  D(bug("[JUAE:A-FSDB] %s: fib_Protection : 0x%04x\n", __PRETTY_FUNCTION__, fib->fib_Protection));
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

  statbuf->mtime.tv_sec = fib->fib_Date.ds_Days*24 + fib->fib_Date.ds_Minute*60 + fib->fib_Date.ds_Tick*50;
  statbuf->mtime.tv_usec= 0;

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
 * name_invalid
 *
 * as both host and guest are "amigaoid", filenames should always
 * be valid.
 ******************************************************************/
int fsdb_name_invalid (const TCHAR *n) {
  DebOut("name: %s is ok\n", n);
  return FALSE;
}

int fsdb_name_invalid_dir (const TCHAR *n) {
  DebOut("name: %s is ok\n", n);
  return FALSE;
}

/******************************************************************
 * int my_*dir (struct my_opendir_s *mod, TCHAR *name)
 *
 * These functions are used to open and scan a directory.
 ******************************************************************/

struct my_opendir_s {
  struct FileInfoBlock *h;
  BPTR lock;
  int first;
};

struct my_openfile_s {
  BPTR lock;
};

/* return next dir entry */
int my_readdir (struct my_opendir_s *mod, TCHAR *name) {

  strcpy(name, (TCHAR *) mod->h->fib_FileName);
  DebOut("name: %s\n", name);

  if(ExNext(mod->lock, mod->h)) {
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

  DebOut("mos: %lx  name: %s, flags: %lx\n",mos ,name, flags);

  if (flags & O_RDONLY) {
    access_mode=MODE_OLDFILE;
  }
  else if(flags & O_TRUNC)  {
    access_mode=MODE_NEWFILE;
  }
  else if(flags & O_WRONLY) {
    access_mode=MODE_READWRITE;
  }
  else if(flags & O_RDWR) {
    access_mode=MODE_READWRITE;
  }
  else {
    DebOut("ERROR: strange flags!?\n");
  }

  h=Open(name, access_mode);

  if(!h) {
    FreeVec(mos);
    return NULL;
  }

  mos->lock=h;
  return mos;
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

  if(!ExNext(mod->lock, mod->h)) {
    DebOut("ExNext failed!\n");
    goto ERROR;
  }

  mod->first = 1;

  DebOut("ok\n");

  return mod;
ERROR:

  my_closedir(mod);
  return NULL;
}

struct my_opendir_s *my_opendir (const TCHAR *name) {

  return my_opendir (name, "");
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
  BPTR lock=0;
  int ret=0;

  DebOut("%lx %lx\n", base, aino);

  DebOut("aino->dir: %s\n", aino->dir);
  DebOut("aino->nname: %s\n", aino->nname);

  fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, TAG_END);
  if(!fib) return 0;

  lock=Lock(aino->nname, SHARED_LOCK);
  if(!lock) goto ERROR;

  if (!Examine(lock, fib)) goto ERROR;

  if(fib->fib_DirEntryType>0) {
    DebOut("=> directory\n");
    aino->dir=1;
    if(fib->fib_DirEntryType==ST_SOFTLINK) {
      DebOut("SoftLink detected!\n");
      aino->softlink=2; /* 1 hardlink, 2 softlink ? */
    }
  }

  if(fib->fib_Comment) {
    DebOut("comment: >%s<\n", fib->fib_Comment);
    aino->comment=(TCHAR *) strdup((const char *)fib->fib_Comment);
  }

  aino->amigaos_mode=fib->fib_Protection;
  DebOut("protection: %04x\n", fib->fib_Protection);

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
