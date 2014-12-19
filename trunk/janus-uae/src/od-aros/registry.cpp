/************************************************************************
 * 
 * INI file create/read/write operations for AROS hosts
 *
 * Copyright 2014 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of WinUAE4AROS
 *
 * WinUAE4AROS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * WinUAE4AROS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WinUAE4AROS. If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id$
 *
 ************************************************************************/

#include <string.h>
#include <proto/dos.h>

#include "SDL_Config/SDL_config_lib.h"

#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "aros.h"
#include "registry.h"
#include "crc32.h"

static CFG_File ini;
#define _tcscpy_s strncpy
static int inimode = 1;
static TCHAR *inipath;
#define PUPPA _T("eitätäoo")

static TCHAR *gs (UAEREG *root) {

	if (!root) {
		return _T("WinUAE");
  }
	return root->inipath;
}


int regsetstr (UAEREG *root, const TCHAR *name, const TCHAR *str) {

  DebOut("entered\n");
  DebOut("group: %s\n", gs(root));
  DebOut("name: %s\n", name);

  //reginitializeinit(NULL);

  if(CFG_SelectGroup(gs(root), TRUE)==CFG_ERROR) {
    DebOut("could not create group %s \n", gs(root));
    return 0;
  }

  if(CFG_WriteText(name, str)==CFG_ERROR) {
    DebOut("ERROR: wrong type for key %s\n", name);
    return 0;
  }

  //regflushfile();

  DebOut("now: %s = %s\n", name, str);
  return 1;
}

int regsetint (UAEREG *root, const TCHAR *name, int val) {

  DebOut("entered\n");
  DebOut("group: %s\n", gs(root));
  DebOut("name: %s\n", name);

  //reginitializeinit(NULL);

  if(CFG_SelectGroup(gs(root), TRUE)==CFG_ERROR) {
    DebOut("could not create group %s \n", gs(root));
    //regflushfile();
    return 0;
  }

  if(CFG_WriteInt(name, val)==CFG_ERROR) {
    DebOut("ERROR: wrong type for key %s\n", name);
    //regflushfile();
    return 0;
  }

  DebOut("now: %s = %d\n", name, val);
  //regflushfile();
  return 1;
}

int regqueryint (UAEREG *root, const TCHAR *name, int *val) {
  signed int res;

  DebOut("group: %s\n", gs(root));

  //reginitializeinit(NULL);

  if(CFG_SelectGroup(gs(root), FALSE)==CFG_ERROR) {
    DebOut("group %s not found\n", gs(root));
    //regflushfile();
    return 0;
  }
  DebOut("name: %s\n", name);

  res=CFG_ReadInt(name, 6666);
  if (res == 6666) {
    DebOut("key %s not found\n", name);
    *val=0;
    //regflushfile();
    return 0;
  }

  DebOut("found %s = %d\n", name, res);
  *val=res;
  //regflushfile();
  return 1;
}

/* warning: never tested */
int regsetlonglong (UAEREG *root, const TCHAR *name, ULONGLONG val) {

  TCHAR str[32];

  DebOut("entered\n");
  DebOut("group: %s\n", gs(root));
  DebOut("name: %s\n", name);

  //reginitializeinit(NULL);

  if(CFG_SelectGroup(gs(root), TRUE)==CFG_ERROR) {
    DebOut("could not create group %s \n", gs(root));
    //regflushfile();
    return 0;
  }

  /* no 64bit support in SDL_config..*/
	_stprintf (str, _T("%I64d"), val);

  if(CFG_WriteText(name, str)==CFG_ERROR) {
    DebOut("ERROR: wrong type for key %s\n", name);
    //regflushfile();
    return 0;
  }

  DebOut("now: %s = %s\n", name, str);
  //regflushfile();
  return 1;
}

/* warning: never tested */
int regquerylonglong (UAEREG *root, const TCHAR *name, ULONGLONG *val) {

  const char *res;

  DebOut("entered\n");
  DebOut("group: %s\n", gs(root));
  DebOut("name: %s\n", name);

  //reginitializeinit(NULL);

  if(CFG_SelectGroup(gs(root), TRUE)==CFG_ERROR) {
    DebOut("could not create group %s \n", gs(root));
    return 0;
  }

  res=CFG_ReadText(name, PUPPA);
  if (!strcmp (res, PUPPA)) {
    DebOut("key %s not found\n", name);
    //regflushfile();
    return 0;
  }

  /* no 64bit support in SDL_config..*/
  *val = _tstoi64 (res);

  DebOut("return: %s = %I64d\n", name, *val);
  //regflushfile();
  return 1;
}

int regquerystr (UAEREG *root, const TCHAR *name, TCHAR *str, int *size)
{
  const char *res;
  DebOut("entered\n");

  //reginitializeinit(NULL);

  if(CFG_SelectGroup(gs(root), FALSE)==CFG_ERROR) {
    DebOut("group %s not found\n", gs(root));
    //regflushfile();
    return 0;
  }

  res=CFG_ReadText(name, PUPPA);
  if (!strcmp (res, PUPPA)) {
    DebOut("key %s not found\n", name);
    //regflushfile();
    return 0;
  }

  strncpy(str, res, *size);
  DebOut("found %s = %s\n", name, str);
  //regflushfile();
  return strlen(str);
}

/* get the nth (idx) key/value in section gs(root)*/
int regenumstr (UAEREG *root, int idx, TCHAR *name, int *nsize, TCHAR *str, int *size) {
  int i;
  const char *res;
  int ret;
  int max;

  //reginitializeinit(NULL);

	name[0] = 0;
	str[0]  = 0;

  DebOut("idx: %d\n", idx);
  DebOut("root: %lx\n", root);

  DebOut("group: %s\n", gs(root));
  CFG_SelectGroup(gs(root), TRUE);

  max=CFG_GetEntriesInSelectedGroup(0);
  DebOut("number of entries: %d\n", CFG_GetEntriesInSelectedGroup(0));

  if(idx == max) {
    DebOut("no more entries\n");
    //regflushfile();
    return 0;
  }

  CFG_StartEntryIteration();

  i=0;
  while(i<idx) {
    CFG_SelectNextEntry();
    i++;
  }

  res=CFG_GetSelectedEntryName();
  DebOut("name: %s\n", res);
  strncpy(name, res, *nsize);

  res=CFG_ReadText(name, "");
  DebOut("value: %s\n", res);
  strncpy(str, res, *size);

  //regflushfile();

  return 1;
}

/* never used anywhere..*/
int regquerydatasize (UAEREG *root, const TCHAR *name, int *size)
{
  int ret = 0;
  int csize = 65536;
  //reginitializeinit(NULL);
  TCHAR *tmp = xmalloc (TCHAR, csize);
  TODO();
  if (regquerystr (root, name, tmp, &csize)) {
    *size = _tcslen (tmp) / 2;
    ret = 1;
  }
  xfree (tmp);
  //regflushfile();
  return ret;
}

/* write binary data !? */
/* warning: never tested */
int regsetdata (UAEREG *root, const TCHAR *name, const void *str, int size) {

  uae_u8 *in = (uae_u8*)str;
  int i;
  TCHAR *tmp = xmalloc (TCHAR, size * 2 + 1);
  int ret=1;

  //reginitializeinit(NULL);

  if(CFG_SelectGroup(gs(root), FALSE)==CFG_ERROR) {
    DebOut("group %s not found\n", gs(root));
    //regflushfile();
    return 0;
  }
  for (i = 0; i < size; i++) {
    _stprintf (tmp + i * 2, _T("%02X"), in[i]); 
  }
  if(CFG_WriteText(name, tmp)==CFG_ERROR) {
    DebOut("Error writing to key %s\n", name);
    ret=0;
  }

  xfree (tmp);
  //regflushfile();
  return ret;
}

/* write binary data !? */
/* warning: never tested */
int regquerydata (UAEREG *root, const TCHAR *name, void *str, int *size)
{
  int csize = (*size) * 2 + 1;
  int i, j;
  int ret = 0;
  TCHAR *tmp = xmalloc (TCHAR, csize);
  uae_u8 *out = (uae_u8*)str;
  TCHAR c1, c2;

  //reginitializeinit(NULL);

  if (!regquerystr (root, name, tmp, &csize))
    goto err;
  j = 0;
  for (i = 0; i < _tcslen (tmp); i += 2) {
    c1 = toupper(tmp[i + 0]);
    c2 = toupper(tmp[i + 1]);
    if (c1 >= 'A')
      c1 -= 'A' - 10;
    else if (c1 >= '0')
      c1 -= '0';
    if (c1 > 15)
      goto err;
    if (c2 >= 'A')
      c2 -= 'A' - 10;
    else if (c2 >= '0')
      c2 -= '0';
    if (c2 > 15)
      goto err;
    out[j++] = c1 * 16 + c2;
  }
  ret = 1;
err:
  //regflushfile();
  xfree (tmp);
  return ret;
}

/* warning: never tested */
int regdelete (UAEREG *root, const TCHAR *name) {

  DebOut("group: %s\n", gs(root));
  DebOut("name: %s\n", name);

  //reginitializeinit(NULL);

  if(CFG_SelectGroup(gs(root), FALSE)==CFG_ERROR) {
    DebOut("group %s not found, ok\n", gs(root));
    //regflushfile();
    return 1;
  }

  /* as we don't know the type, delete any type */
  CFG_RemoveBoolEntry(name);
  CFG_RemoveIntEntry(name);
  CFG_RemoveFloatEntry(name);
  CFG_RemoveTextEntry(name);
  //regflushfile();
  return 1;
}

int regexists (UAEREG *root, const TCHAR *name)
{
  CFG_File config;
  int ret = 1;
  const TCHAR *tmp;

  DebOut("entered\n");
  DebOut("inipath: %s\n", inipath);
  DebOut("name: %s\n", name);

#if 0
  if (CFG_OK != CFG_OpenFile("PROGDIR:winuae.ini", &config)) {
    DebOut("winuae.ini does not exist\n");
    return 0;
  }

  DebOut("opened PROGDIR:winuae\n");
#endif

  tmp=CFG_ReadText(name, PUPPA);
  if (!_tcscmp (tmp, PUPPA)) {
    DebOut("created new file\n");
    ret = 0;
  }

  DebOut("File was already there (%s = %s)\n", name, tmp);
#if 0

  CFG_CloseFile(&config);
#endif

  return ret;
}

/* delete complete group */
/* warning: never tested */
void regdeletetree (UAEREG *root, const TCHAR *name) {

  int ret;

  DebOut("name: %s\n", name);
  DebOut("entered\n");
  DebOut("WARNING: never tested!!\n");

  if(root!=NULL) {
    DebOut("ERROR: root!=NULL not done!!\n");
    return;
  }

  //reginitializeinit(NULL);

  if(CFG_RemoveGroup(name)==CFG_ERROR) {
    DebOut("group %s did not exists before\n", name);
    //regflushfile();
    return;
  }

  //regflushfile();
  DebOut("group %s deleted\n", name);
}

int regexiststree (UAEREG *root, const TCHAR *name)
{
  int res;

  //reginitializeinit(NULL);

  DebOut("entered\n");
  DebOut("root: %s\n", root);
  DebOut("name: %s\n", name);

  res=(int) CFG_GroupExists(name);

  DebOut("result: %d\n", res);

  //regflushfile();

  return res;
}


UAEREG *regcreatetree (UAEREG *root, const TCHAR *name) {
	UAEREG *fkey;
  TCHAR *ininame;

  DebOut("entered\n");
  DebOut("name: %s\n", name);
  DebOut("root: %lx\n", root);

  if (!root) {
    if (!name)
      ininame = my_strdup (gs (NULL));
    else
      ininame = my_strdup (name);
  } else {
    ininame = xmalloc (TCHAR, _tcslen (root->inipath) + 1 + _tcslen (name) + 1);
    _stprintf (ininame, _T("%s/%s"), root->inipath, name);
  }
  DebOut("ininame: %s\n", ininame);
  fkey = xcalloc (UAEREG, 1);
  fkey->inipath = ininame;

  //reginitializeinit(NULL);
  CFG_SelectGroup(name, true);
  //regflushfile();

  DebOut("created key: %lx\n", fkey);
  DebOut("selected group: %s\n", name);

	return fkey;
}

/* free allocated memory */
void regclosetree (UAEREG *key)
{
  DebOut("entered (key: %lx)\n", key);
	if (!key)
		return;
#if 0
	if (key->fkey)
		RegCloseKey (key->fkey);
#endif
	xfree (key->inipath);
	xfree (key);
}

#ifdef SHA1_CHECK
static uae_u8 crcok[20] = { 0xaf,0xb7,0x36,0x15,0x05,0xca,0xe6,0x9d,0x23,0x17,0x4d,0x50,0x2b,0x5c,0xc3,0x64,0x38,0xb8,0x4e,0xfc };
#endif

int reginitializeinit (TCHAR **pppath)
{
  const char *res;
#if SHA1_CHECK
	TCHAR tmp1[1000];
	uae_u8 crc[20];
	int s, v1, v2, v3;
#endif

  /* AROS never uses the registry (we have none..), so we use INI-files */
	inimode = 1;

  if (CFG_OK != CFG_OpenFile(INI_FILE, &ini )) {
    DebOut("%s does not exist\n", INI_FILE);
    goto FAIL2;
  }

  if(CFG_SelectGroup("WinUAE", false) != CFG_OK) {
    DebOut("%s has dubious contents\n", INI_FILE);
    goto FAIL1;
  }

  res=CFG_ReadText("Version", PUPPA);
  if(!strcmp(res, PUPPA)) {
    DebOut("%s has dubious contents\n", INI_FILE);
    goto FAIL1;
  }

  if(CFG_SelectGroup("Warning", true)==CFG_ERROR) {
    DebOut("%s: could not create new group\n");
    goto FAIL1;
  }

#if SHA1_CHECK
  /* o1i: this is not really useful IMHO. why not just make a string comare!? */
  /* anyhow, I'll skip this test.. */
	memset (tmp1, 0, sizeof tmp1);
	s = 200;
  res=CFG_ReadText("info1", PUPPA);
  DebOut("info1: %s\n", res);
  if(!strcmp(res, PUPPA)) goto FAIL1;
  strcpy(tmp1, res);
  res=CFG_ReadText("info2", PUPPA);
  DebOut("info2: %s\n", res);
  if(!strcmp(res, PUPPA)) goto FAIL1;
  strcpy(tmp1+200, res);
	get_sha1 (tmp1, sizeof tmp1, crc);
	if (memcmp (crc, crcok, sizeof crcok)) {
    DebOut("sha1 check failed!\n");
		goto FAIL1;
  }
#endif

  /* INI file is good enough for me! */
  DebOut("%s checks done. File seems ok.\n", INI_FILE);
  return 1;
FAIL1:
  CFG_CloseFile(NULL);
  DebOut("%s deleted\n", INI_FILE);
  DeleteFile(INI_FILE);

FAIL2:
  if (CFG_OK == !CFG_OpenFile(NULL, &ini )) {

    DebOut("FAILED TO CREATE ANY INI FILE!?\n");
    write_log("Failed to create any ini file..!?\n");
    return 0;
  }
  DebOut("%s initialized\n", INI_FILE);

  CFG_SelectGroup("WinUAE", true);
  CFG_SelectGroup("Warning", true);
  CFG_WriteText("info1", "This is unsupported file. Compatibility between versions is not guaranteed.");
  CFG_WriteText("info2", "Incompatible ini-files may be re-created from scratch!");
  CFG_WriteText("info0", "This file is *not* for Windows. It is for AROS, really!");

  DebOut("%s saved\n");

	return 1;
}

void regflushfile (void) {

  DebOut("entered\n");

  CFG_SaveFile(INI_FILE, CFG_SORT_ORIGINAL, CFG_COMPRESS_OUTPUT);
  CFG_CloseFile(NULL);
}



void regstatus (void)
{
  DebOut("entered\n");
	if (inimode)
		write_log (_T("WARNING: Unsupported '%s' enabled\n"), inipath);
}

int getregmode (void)
{
  DebOut("entered\n");
	return inimode;
}
