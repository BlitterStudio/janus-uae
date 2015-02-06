/************************************************************************ 
 *
 * AROS specific file system functions
 *
 * Copyright 2011       Oliver Brunner - aros<at>oliver-brunner.de
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

    bug("[JUAE:A-FSDB] %s('%s')\n", __PRETTY_FUNCTION__, name);
    bug("[JUAE:A-FSDB] %s: statbuf @ 0x%p\n", __PRETTY_FUNCTION__, statbuf);

  lock=Lock(name, SHARED_LOCK);
  if(!lock) {
    bug("[JUAE:A-FSDB] %s: failed to lock entry\n", __PRETTY_FUNCTION__);

    goto exit;
  }

  if(!(fib=(struct FileInfoBlock *) AllocDosObject(DOS_FIB, NULL)) || !Examine(lock, fib)) {
    bug("[JUAE:A-FSDB] %s: failed to examine lock @ 0x%p [fib @ 0x%p]\n", __PRETTY_FUNCTION__, lock, fib);

    goto exit;
  }

  statbuf->size=fib->fib_Size;
  if(fib->fib_DiskKey) statbuf->mode |= FILEFLAG_DIR;
  /* fib_Protection: active-low, i.e. not set means the action is allowed! (who invented this !??) */
  if(! (fib->fib_Protection & FIBF_READ))    statbuf->mode |= FILEFLAG_WRITE;
  if(! (fib->fib_Protection & FIBF_WRITE))   statbuf->mode |= FILEFLAG_READ;
  if(! (fib->fib_Protection & FIBF_EXECUTE)) statbuf->mode |= FILEFLAG_EXECUTE;
  if(! (fib->fib_Protection & FIBF_ARCHIVE)) statbuf->mode |= FILEFLAG_ARCHIVE;
  if(! (fib->fib_Protection & FIBF_PURE))    statbuf->mode |= FILEFLAG_PURE;
  if(! (fib->fib_Protection & FIBF_SCRIPT))  statbuf->mode |= FILEFLAG_SCRIPT;

  statbuf->mtime.tv_sec = fib->fib_Date.ds_Days*24 + fib->fib_Date.ds_Minute*60 + fib->fib_Date.ds_Tick*50;
  statbuf->mtime.tv_usec= 0;

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
  struct ExAllControl *eac=NULL;
  BPTR lock=0;
  BPTR lock2=0;
  struct ExAllData *buffer=NULL;
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
    aino->dir=1;
    DebOut("=> directory\n");
    eac=(struct ExAllControl *) AllocDosObject(DOS_EXALLCONTROL, TAG_END);
    if(!eac) goto ERROR;
    buffer=(struct ExAllData *) AllocVec(4096, MEMF_CLEAR);
    lock2=Lock(aino->nname, SHARED_LOCK);
    if(ExAll(lock2, (struct ExAllData *) buffer, sizeof(buffer)-1, ED_COMMENT, eac)) {
#warning fix this, always fails on hosted
      DebOut("ExAll failed!?\n");
    }
    else {
      if(buffer->ed_Type==ST_SOFTLINK) {
        DebOut("SoftLink detected!\n");
        aino->softlink=1;
      }
    }
  }
  ret=1;

#warning TODO!!!: care for base and rest! 
  /*
   * rest:
   * aino->amigaos_mode=
   * aino->comment=
   * create_uaefsdb (aino, fsdb, mode);
   * write_uaefsdb (aino->nname, fsdb);
   */

ERROR:
  if(buffer) FreeVec(buffer);
  if(eac) FreeDosObject(DOS_EXALLCONTROL, eac);
  if(fib) FreeDosObject(DOS_FIB, fib);
  if(lock) UnLock(lock);
  if(lock2) UnLock(lock2);

  return ret;
}
