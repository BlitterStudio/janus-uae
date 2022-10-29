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

#include "sysconfig.h"
#include "sysdeps.h"

#include "aros.h"
#include "registry.h"
#include "crc32.h"

static CFG_File ini;
#define _tcscpy_s strncpy
static int inimode = 1;
TCHAR *inipath;
#define PUPPA _T("eitätäoo")

static TCHAR *gs (UAEREG *root)
{
    if (!root) {
        return _T("WinUAE");
    }
    return root->inipath;
}

int regsetstr (UAEREG *root, const TCHAR *name, const TCHAR *str)
{
    D(bug("[JUAE:Reg] %s(0x%p, '%s')\n", __PRETTY_FUNCTION__, root, name));
    D(bug("[JUAE:Reg] %s: group: %s\n", __PRETTY_FUNCTION__, gs(root)));

    if (CFG_SelectGroup(gs(root), TRUE) == CFG_ERROR) {
        bug("[JUAE:Reg] %s: could not create group %s \n", __PRETTY_FUNCTION__, gs(root));
        return 0;
    }

    /* see regsetint */
    CFG_ReadText(name, PUPPA);

    if (CFG_WriteText(name, str)==CFG_ERROR) {
        bug("[JUAE:Reg] %s: ERROR: wrong type for key %s\n", __PRETTY_FUNCTION__, name);
        return 0;
    }

    D(bug("[JUAE:Reg] %s: now: %s = %s\n", __PRETTY_FUNCTION__, name, str));
    return 1;
}

int regsetint (UAEREG *root, const TCHAR *name, int val)
{
    D(bug("[JUAE:Reg] %s(0x%p, '%s')\n", __PRETTY_FUNCTION__, root, name));
    D(bug("[JUAE:Reg] %s: group: %s\n", __PRETTY_FUNCTION__, gs(root)));

    if (CFG_SelectGroup(gs(root), TRUE) == CFG_ERROR) {
        bug("[JUAE:Reg] %s: could not create group %s \n", __PRETTY_FUNCTION__, gs(root));
        //regflushfile();
        return 0;
    }

    /* SDL throws a warning, if you try to write something to a key, that does not exist.
     * So we try to read the key (which creates it, if necessary with the supplied 
     * default value). Of course we are not really interested in the old value..
     * Yes,. this is strange..
     */
    if(CFG_ReadInt(name, val)==val) {
      D(bug("[JUAE:Reg] %s: key %s was already set to %d\n", __PRETTY_FUNCTION__, name, val));
      return TRUE;
    }

    if(CFG_WriteInt(name, val)==CFG_ERROR) {
        bug("[JUAE:Reg] %s: ERROR: wrong type for key %s\n", __PRETTY_FUNCTION__, name);
        //regflushfile();
        return 0;
    }

    D(bug("[JUAE:Reg] %s: now: %s = %d\n", __PRETTY_FUNCTION__, name, val));
    return 1;
}

int regqueryint (UAEREG *root, const TCHAR *name, int *val)
{
    signed int res;

    D(bug("[JUAE:Reg] %s(0x%p, '%s')\n", __PRETTY_FUNCTION__, root, name));
    D(bug("[JUAE:Reg] %s: group: %s\n", __PRETTY_FUNCTION__, gs(root)));

    if (CFG_SelectGroup(gs(root), FALSE) == CFG_ERROR) {
        bug("[JUAE:Reg] %s: group %s not found\n", __PRETTY_FUNCTION__, gs(root));
        return 0;
    }
    D(bug("[JUAE:Reg] name: %s\n", __PRETTY_FUNCTION__, name));

    res = CFG_ReadInt(name, 6666);
    if (res == 6666) {
        bug("[JUAE:Reg] %s: key %s not found\n", __PRETTY_FUNCTION__, name);
        *val=0;
        return 0;
    }

    D(bug("[JUAE:Reg] %s: found %s = %d\n", __PRETTY_FUNCTION__, name, res));
    *val = res;
    return 1;
}

/* warning: never tested */
int regsetlonglong (UAEREG *root, const TCHAR *name, ULONGLONG val)
{
    TCHAR str[32];

    D(bug("[JUAE:Reg] %s(0x%p, '%s')\n", __PRETTY_FUNCTION__, root, name));
    D(bug("[JUAE:Reg] %s: group: %s\n", __PRETTY_FUNCTION__, gs(root)));

    if (CFG_SelectGroup(gs(root), TRUE) == CFG_ERROR) {
        bug("[JUAE:Reg] %s: could not create group %s \n", __PRETTY_FUNCTION__, gs(root));
        //regflushfile();
        return 0;
    }

    /* no 64bit support in SDL_config..*/
    _stprintf (str, _T("%lld"), val);

    /* see regsetint */
    CFG_ReadText(name, str);

    if (CFG_WriteText(name, str) == CFG_ERROR) {
        bug("[JUAE:Reg] %s: ERROR: wrong type for key %s\n", __PRETTY_FUNCTION__, name);
        //regflushfile();
        return 0;
    }

    D(bug("[JUAE:Reg] %s: now: %s = %s\n", __PRETTY_FUNCTION__, name, str));
    return 1;
}

/* warning: never tested */
int regquerylonglong (UAEREG *root, const TCHAR *name, ULONGLONG *val)
{
    const char *res;

    D(bug("[JUAE:Reg] %s(0x%p, '%s')\n", __PRETTY_FUNCTION__, root, name));
    D(bug("[JUAE:Reg] %s: group: %s\n", __PRETTY_FUNCTION__, gs(root)));

    if (CFG_SelectGroup(gs(root), TRUE) == CFG_ERROR) {
        bug("[JUAE:Reg] %s: could not create group %s \n", __PRETTY_FUNCTION__, gs(root));
        return 0;
    }

    res = CFG_ReadText(name, PUPPA);
    if (!strcmp (res, PUPPA)) {
        bug("[JUAE:Reg] %s: key %s not found\n", __PRETTY_FUNCTION__, name);
        //regflushfile();
        return 0;
    }

    /* no 64bit support in SDL_config..*/
    *val = _tstoi64 (res);

    D(bug("[JUAE:Reg] %s: return: %s = %I64d\n", __PRETTY_FUNCTION__, name, *val));
    return 1;
}

int regquerystr (UAEREG *root, const TCHAR *name, TCHAR *str, int *size)
{
    CFG_String_Arg res;

    D(bug("[JUAE:Reg] %s(0x%p, '%s', %d)\n", __PRETTY_FUNCTION__, root, name, *size));

    if (CFG_SelectGroup(gs(root), FALSE) == CFG_ERROR) {
        bug("[JUAE:Reg] %s: group %s not found\n", __PRETTY_FUNCTION__, gs(root));
        return 0;
    }

    res = CFG_ReadText(name, PUPPA);
    if (!strcmp (res, PUPPA)) {
        bug("[JUAE:Reg] %s: key %s not found\n", __PRETTY_FUNCTION__, name);
        return 0;
    }

    if(strlen(res)>*size-1) {
      /* WARNING: this seems to destroy the *size value !? */
      D(bug("[JUAE:Reg] %s: Warning: truncated %s\n", __PRETTY_FUNCTION__, res));
      strncpy(str, res, *size);
      str[*size-1]=0;
    }
    else {
      strcpy(str, res);
    }
    D(bug("[JUAE:Reg] %s: found %s = %s\n", __PRETTY_FUNCTION__, name, str));

    return strlen(str);
}

/* get the nth (idx) key/value in section gs(root)*/
int regenumstr (UAEREG *root, int idx, TCHAR *name, int *nsize, TCHAR *str, int *size)
{
    int i;
    const char *res;
    int ret;
    int max;

    name[0] = 0;
    str[0]  = 0;

    D(bug("[JUAE:Reg] %s(0x%p)\n", __PRETTY_FUNCTION__, root));
    D(bug("[JUAE:Reg] %s: idx: %d\n", __PRETTY_FUNCTION__, idx));
    D(bug("[JUAE:Reg] %s: group: %s\n", __PRETTY_FUNCTION__, gs(root)));

    CFG_SelectGroup(gs(root), TRUE);

    max = CFG_GetEntriesInSelectedGroup(0);
    D(bug("[JUAE:Reg] %s: number of entries: %d\n", __PRETTY_FUNCTION__, CFG_GetEntriesInSelectedGroup(0)));

    if (idx == max) {
        D(bug("[JUAE:Reg] %s: no more entries\n", __PRETTY_FUNCTION__));
        return 0;
    }

    CFG_StartEntryIteration();

    i = 0;
    while(i<idx) {
        CFG_SelectNextEntry();
        i++;
    }

    res = CFG_GetSelectedEntryName();
    D(bug("[JUAE:Reg] %s: name: %s\n", __PRETTY_FUNCTION__, res));
    strncpy(name, res, *nsize);

    res = CFG_ReadText(name, "");
    D(bug("[JUAE:Reg] %s: value: %s\n", __PRETTY_FUNCTION__, res));
    strncpy(str, res, *size);

    return 1;
}

/* never used anywhere..*/
int regquerydatasize (UAEREG *root, const TCHAR *name, int *size)
{
    int ret = 0;
    int csize = 65536;
    TCHAR *tmp = xmalloc (TCHAR, csize);

    TODO();

    if (regquerystr (root, name, tmp, &csize)) {
        *size = _tcslen (tmp) / 2;
        ret = 1;
    }
    xfree (tmp);

    return ret;
}

/* write binary data !? */
/* warning: never tested */
int regsetdata (UAEREG *root, const TCHAR *name, const void *str, int size)
{
    uae_u8 *in = (uae_u8*)str;
    int i;
    TCHAR *tmp = xmalloc (TCHAR, size * 2 + 1);
    int ret=1;

    if (CFG_SelectGroup(gs(root), FALSE)==CFG_ERROR) {
        bug("[JUAE:Reg] %s: group %s not found\n", __PRETTY_FUNCTION__, gs(root));
        return 0;
    }
    for (i = 0; i < size; i++) {
        _stprintf (tmp + i * 2, _T("%02X"), in[i]); 
    }

    /* see regsetint */
    CFG_ReadText(name, tmp);

    if (CFG_WriteText(name, tmp)==CFG_ERROR) {
        bug("[JUAE:Reg] %s: Error writing to key %s\n", __PRETTY_FUNCTION__, name);
        ret=0;
    }

    xfree (tmp);
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
    xfree (tmp);
    return ret;
}

/* warning: never tested */
int regdelete (UAEREG *root, const TCHAR *name)
{
    D(bug("[JUAE:Reg] %s(0x%p, '%s')\n", __PRETTY_FUNCTION__, root, name));
    D(bug("[JUAE:Reg] %s: group: %s\n", __PRETTY_FUNCTION__, gs(root)));

    if (CFG_SelectGroup(gs(root), FALSE)==CFG_ERROR) {
        bug("[JUAE:Reg] %s: group %s not found, ok\n", __PRETTY_FUNCTION__, gs(root));
        return 1;
    }

    /* as we don't know the type, delete any type */
    CFG_RemoveBoolEntry(name);
    CFG_RemoveIntEntry(name);
    CFG_RemoveFloatEntry(name);
    CFG_RemoveTextEntry(name);

    return 1;
}

int regexists (UAEREG *root, const TCHAR *name)
{
    CFG_File config;
    int ret = 1;
    const TCHAR *tmp;

    D(bug("[JUAE:Reg] %s(0x%p, '%s')\n", __PRETTY_FUNCTION__, root, name));

    if (CFG_SelectGroup(gs(root), FALSE) == CFG_ERROR) {
        D(bug("[JUAE:Reg] %s: group %s does not exists\n", __PRETTY_FUNCTION__, gs(root)));
        return FALSE;
    }

    /* 0 when no entry was found */
    if(!CFG_GetEntryType(name)) {
      D(bug("[JUAE:Reg] %s: entry %s does not exists\n", __PRETTY_FUNCTION__, name));
      return FALSE;
    }

    D(bug("[JUAE:Reg] %s: entry %s exists\n", __PRETTY_FUNCTION__, name));
    return TRUE;
}

/* delete complete group */
/* warning: never tested */
void regdeletetree (UAEREG *root, const TCHAR *name)
{
    int ret;

    D(bug("[JUAE:Reg] %s(0x%p, '%s')\n", __PRETTY_FUNCTION__, root, name));
    D(bug("[JUAE:Reg] %s: WARNING: never tested!!\n", __PRETTY_FUNCTION__));

    if(root != NULL) {
        bug("[JUAE:Reg] %s: ERROR: root != NULL, skipping!!\n", __PRETTY_FUNCTION__);
        return;
    }

    if(CFG_RemoveGroup(name)==CFG_ERROR) {
        bug("[JUAE:Reg] %s: group %s did not exists before\n", __PRETTY_FUNCTION__, name);
        return;
    }

    bug("[JUAE:Reg] %s: group %s deleted\n", __PRETTY_FUNCTION__, name);
}

int regexiststree (UAEREG *root, const TCHAR *name)
{
    int res;

    D(bug("[JUAE:Reg] %s(0x%p, '%s')\n", __PRETTY_FUNCTION__, root, name));
    D(bug("[JUAE:Reg] %s: root: %s\n", __PRETTY_FUNCTION__, root));
    D(bug("[JUAE:Reg] %s: name: %s\n", __PRETTY_FUNCTION__, name));

    res = (int) CFG_GroupExists(name);

    D(bug("[JUAE:Reg] %s: result: %d\n", __PRETTY_FUNCTION__, res));

    return res;
}


UAEREG *regcreatetree (UAEREG *root, const TCHAR *name)
{
    UAEREG *fkey;
    TCHAR *ininame;

    D(bug("[JUAE:Reg] %s(0x%p, '%s')\n", __PRETTY_FUNCTION__, root, name));
    D(bug("[JUAE:Reg] %s: name: %s\n", __PRETTY_FUNCTION__, name));
    D(bug("[JUAE:Reg] %s: root: %lx\n", __PRETTY_FUNCTION__, root));

    if (!root) {
        if (!name)
            ininame = my_strdup (gs (NULL));
        else
            ininame = my_strdup (name);
    } else {
        ininame = xmalloc (TCHAR, _tcslen (root->inipath) + 1 + _tcslen (name) + 1);
        _stprintf (ininame, _T("%s/%s"), root->inipath, name);
    }
    D(bug("[JUAE:Reg] %s: ininame: %s\n", __PRETTY_FUNCTION__, ininame));
    fkey = xcalloc (UAEREG, 1);
    fkey->inipath = ininame;

    CFG_SelectGroup(name, true);

    D(bug("[JUAE:Reg] %s: created key: %lx\n", __PRETTY_FUNCTION__, fkey));
    D(bug("[JUAE:Reg] %s: selected group: %s\n", __PRETTY_FUNCTION__, name));

    return fkey;
}

/* free allocated memory */
void regclosetree (UAEREG *key)
{
    D(bug("[JUAE:Reg] %s(key: %lx)\n", __PRETTY_FUNCTION__, key));

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

int my_existsfile (const TCHAR *name);

int reginitializeinit (TCHAR **pppath)
{
    const char *res;
#if SHA1_CHECK
    TCHAR tmp1[1000];
    uae_u8 crc[20];
    int s, v1, v2, v3;
#endif

    D(bug("[JUAE:Reg] %s()\n", __PRETTY_FUNCTION__));

    /* AROS never uses the registry (we have none..), so we use INI-files */
    inimode = 1;

    if (!my_existsfile(INI_FILE) || (CFG_OK != CFG_OpenFile(INI_FILE, &ini ))) {
        bug("[JUAE:Reg] %s: %s does not exist\n", __PRETTY_FUNCTION__, INI_FILE);
        goto FAIL2;
    }

    if (CFG_SelectGroup("WinUAE", false) != CFG_OK) {
        bug("[JUAE:Reg] %s: %s has dubious contents\n", __PRETTY_FUNCTION__, INI_FILE);
        goto FAIL1;
    }

    res = CFG_ReadText("Version", PUPPA);
    if(!strcmp(res, PUPPA)) {
        bug("[JUAE:Reg] %s: %s has dubious contents\n", __PRETTY_FUNCTION__, INI_FILE);
        goto FAIL1;
    }

    if (CFG_SelectGroup("Warning", true)==CFG_ERROR) {
        bug("[JUAE:Reg] %s: %s: could not create new group\n", __PRETTY_FUNCTION__);
        goto FAIL1;
    }

#if SHA1_CHECK
    /* o1i: this is not really useful IMHO. why not just make a string comare!? */
    /* anyhow, I'll skip this test.. */
    memset (tmp1, 0, sizeof tmp1);
    s = 200;
    res = CFG_ReadText("info1", PUPPA);
    D(bug("[JUAE:Reg] %s: info1: %s\n", __PRETTY_FUNCTION__, res));

    if(!strcmp(res, PUPPA))
        goto FAIL1;
    strcpy(tmp1, res);
    res=CFG_ReadText("info2", PUPPA);
    D(bug("[JUAE:Reg] %s: info2: %s\n", __PRETTY_FUNCTION__, res));

    if(!strcmp(res, PUPPA)) goto FAIL1;
    strcpy(tmp1+200, res);
    get_sha1 (tmp1, sizeof tmp1, crc);
    if (memcmp (crc, crcok, sizeof crcok)) {
        bug("[JUAE:Reg] %s: sha1 check failed!\n", __PRETTY_FUNCTION__);
        goto FAIL1;
    }
#endif

    /* INI file is good enough for me! */
    D(bug("[JUAE:Reg] %s: %s checks done. File seems ok.\n", __PRETTY_FUNCTION__, INI_FILE));
    return 1;
FAIL1:
    CFG_CloseFile(NULL);
    D(bug("[JUAE:Reg] %s: %s deleted\n", __PRETTY_FUNCTION__, INI_FILE));
    DeleteFile(INI_FILE);

FAIL2:
    if (CFG_OK == !CFG_OpenFile(NULL, &ini )) {
        bug("[JUAE:Reg] %s: FAILED TO CREATE ANY INI FILE!?\n", __PRETTY_FUNCTION__);
        write_log("Failed to create any ini file..!?\n");
        return 0;
    }
    D(bug("[JUAE:Reg] %s: %s initialized\n", __PRETTY_FUNCTION__, INI_FILE));

    CFG_SelectGroup("WinUAE", true);
    CFG_SelectGroup("Warning", true);
    CFG_WriteText("info1", "This is unsupported file. Compatibility between versions is not guaranteed.");
    CFG_WriteText("info2", "Incompatible ini-files may be re-created from scratch!");
    CFG_WriteText("info0", "This file is *not* for Windows. It is for AROS, really!");

    D(bug("[JUAE:Reg] %s: saved\n", __PRETTY_FUNCTION__));

    return 1;
}

void regflushfile (void)
{
    D(bug("[JUAE:Reg] %s()\n", __PRETTY_FUNCTION__));

    CFG_SaveFile(INI_FILE, CFG_SORT_ORIGINAL, CFG_COMPRESS_OUTPUT);
    CFG_CloseFile(NULL);
}

void regstatus (void)
{
    D(bug("[JUAE:Reg] %s()\n", __PRETTY_FUNCTION__));

    if (inimode)
        write_log (_T("WARNING: Unsupported '%s' enabled\n"), inipath);
}

int getregmode (void)
{
    D(bug("[JUAE:Reg] %s()\n", __PRETTY_FUNCTION__));

    return inimode;
}

bool switchreginimode(void) {
  return false;
}
