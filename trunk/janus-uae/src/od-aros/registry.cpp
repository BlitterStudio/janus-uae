#include <string.h>
#include <proto/dos.h>
#include "SDL_Config/SDL_config_lib.h"

#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#if 0
#include <windows.h>
#include <shlwapi.h>
#include "win32.h"
#endif
#include "aros.h"
#include "registry.h"
#include "crc32.h"

static CFG_File ini;
#define _tcscpy_s strncpy
static int inimode = 1;
static TCHAR *inipath;
#define PUPPA _T("eitätäoo")

void my_init_config(UAEREG *reg) {

#if 0
  if(reg->open) {
    DebOut("Config already opened\n");
    return;
  }

  if (CFG_OK != CFG_OpenFile("PROGDIR:winuae.ini", (CFG_File *) reg )) {
    DebOut("ERROR: unable to open winuae.ini\n");
  }
  else {
    reg->open=TRUE;
    DebOut("Config now open\n");
  }
#endif
}


BOOL WritePrivateProfileString(TCHAR *lpAppName, const TCHAR *lpKeyName, TCHAR *lpString, TCHAR *lpFileName) {
  CFG_File  config;
  DebOut("lpAppName: %s\n", lpAppName);
  DebOut("lpKeyName: %s\n", lpKeyName);
  DebOut("lpString: %s\n", lpString);
  DebOut("lpFileName: %s\n", lpFileName);

#if 0
  if (CFG_OK != CFG_OpenFile(lpFileName, &config )) {
    DebOut("ERROR: unable to open %s\n", lpFileName);
    return FALSE;
  }
#endif
}

DWORD GetPrivateProfileString(TCHAR *lpAppName, const TCHAR *lpKeyName, TCHAR *lpDefault, TCHAR *lpReturnedString, DWORD nSize,TCHAR *lpFileName) {
  DebOut("lpAppName: %s\n", lpAppName);
  DebOut("lpKeyName: %s\n", lpKeyName);
  DebOut("lpDefault: %s\n", lpDefault);
}


DWORD GetPrivateProfileSection(TCHAR *lpAppName, TCHAR *lpReturnedString, DWORD nSize,TCHAR *lpFileName) {
  DebOut("lpAppName: %s\n", lpAppName);
  DebOut("lpFileName: %s\n", lpFileName);

  return 0;
}

BOOL WritePrivateProfileSection(TCHAR *lpAppName, TCHAR *lpString, TCHAR *lpFileName) {
  DebOut("lpAppName: %s\n", lpAppName);
  DebOut("lpString: %s\n", lpString);
  DebOut("lpFileName: %s\n", lpFileName);
}

DWORD GetPrivateProfileSectionNames(TCHAR *lpszReturnBuffer, DWORD nSize, TCHAR *lpFileName) {
  DebOut("lpFileName: %s\n", lpFileName);
}

#if 0
static HKEY gr (UAEREG *root)
{
	if (!root)
		return hWinUAEKey;
	return root->fkey;
}
#endif
static TCHAR *gs (UAEREG *root)
{
	if (!root)
		return _T("WinUAE");
	return root->inipath;
}
static TCHAR *gsn (UAEREG *root, const TCHAR *name)
{
	TCHAR *r, *s;
  DebOut("entered\n");
	if (!root)
		return my_strdup (name);
	r = gs (root);
	s = xmalloc (TCHAR, _tcslen (r) + 1 + _tcslen (name) + 1);
	_stprintf (s, _T("%s/%s"), r, name);
	return s;
}

int regsetstr (UAEREG *root, const TCHAR *name, const TCHAR *str)
{
  DebOut("entered\n");
	if (inimode) {
		DWORD ret;
		ret = WritePrivateProfileString (gs (root), name, (TCHAR *)str, inipath);
		return ret;
	} else {
    TODO();
#if 0
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		return RegSetValueEx (rk, name, 0, REG_SZ, (CONST BYTE *)str, (_tcslen (str) + 1) * sizeof (TCHAR)) == ERROR_SUCCESS;
#endif
	}
}

int regsetint (UAEREG *root, const TCHAR *name, int val)
{
  DebOut("entered\n");
	if (inimode) {
		DWORD ret;
		TCHAR tmp[100];
		_stprintf (tmp, _T("%d"), val);
		ret = WritePrivateProfileString (gs (root), name, tmp, inipath);
		return ret;
	} else {
    TODO();
#if 0
		DWORD v = val;
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		return RegSetValueEx(rk, name, 0, REG_DWORD, (CONST BYTE*)&v, sizeof (DWORD)) == ERROR_SUCCESS;
#endif
	}
}

int regqueryint (UAEREG *root, const TCHAR *name, int *val)
{
  DebOut("entered\n");
	if (inimode) {
		int ret = 0;
		TCHAR tmp[100];
		GetPrivateProfileString (gs (root), name, PUPPA, tmp, sizeof (tmp) / sizeof (TCHAR), inipath);
		if (_tcscmp (tmp, PUPPA)) {
			*val = _tstol (tmp);
			ret = 1;
		}
		return ret;
	} else {
    TODO();
#if 0
		DWORD dwType = REG_DWORD;
		DWORD size = sizeof (int);
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		return RegQueryValueEx (rk, name, 0, &dwType, (LPBYTE)val, &size) == ERROR_SUCCESS;
#endif
	}
}

int regsetlonglong (UAEREG *root, const TCHAR *name, ULONGLONG val)
{
  DebOut("entered\n");
	if (inimode) {
		DWORD ret;
		TCHAR tmp[100];
		_stprintf (tmp, _T("%I64d"), val);
		ret = WritePrivateProfileString (gs (root), name, tmp, inipath);
		return ret;
	} else {
    TODO();
#if 0
		ULONGLONG v = val;
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		return RegSetValueEx(rk, name, 0, REG_QWORD, (CONST BYTE*)&v, sizeof (ULONGLONG)) == ERROR_SUCCESS;
#endif
	}
}

int regquerylonglong (UAEREG *root, const TCHAR *name, ULONGLONG *val)
{
  DebOut("entered\n");
	if (inimode) {
		int ret = 0;
		TCHAR tmp[100];
		GetPrivateProfileString (gs (root), name, PUPPA, tmp, sizeof (tmp) / sizeof (TCHAR), inipath);
		if (_tcscmp (tmp, PUPPA)) {
			*val = _tstoi64 (tmp);
			ret = 1;
		}
		return ret;
	} else {
    TODO();
#if 0
		DWORD dwType = REG_QWORD;
		DWORD size = sizeof (ULONGLONG);
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		return RegQueryValueEx (rk, name, 0, &dwType, (LPBYTE)val, &size) == ERROR_SUCCESS;
#endif
	}
}


int regquerystr (UAEREG *root, const TCHAR *name, TCHAR *str, int *size)
{
  DebOut("entered\n");
	if (inimode) {
		int ret = 0;
		TCHAR *tmp = xmalloc (TCHAR, (*size) + 1);
		GetPrivateProfileString (gs (root), name, PUPPA, tmp, *size, inipath);
		if (_tcscmp (tmp, PUPPA)) {
			_tcscpy (str, tmp);
			ret = 1;
		}
		xfree (tmp);
		return ret;
	} else {
    TODO();
#if 0
		DWORD size2 = *size * sizeof (TCHAR);
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		int v = RegQueryValueEx (rk, name, 0, NULL, (LPBYTE)str, &size2) == ERROR_SUCCESS;
		*size = size2 / sizeof (TCHAR);
		return v;
#endif
	}
}

/* get the nth (idx) key/value in section gs(root)*/
int regenumstr (UAEREG *root, int idx, TCHAR *name, int *nsize, TCHAR *str, int *size) {
  int i;
  const char *res;
  int ret;
  int max;

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

  return 1;
}

int regquerydatasize (UAEREG *root, const TCHAR *name, int *size)
{
	if (inimode) {
		int ret = 0;
		int csize = 65536;
		TCHAR *tmp = xmalloc (TCHAR, csize);
		if (regquerystr (root, name, tmp, &csize)) {
			*size = _tcslen (tmp) / 2;
			ret = 1;
		}
		xfree (tmp);
		return ret;
	} else {
    TODO();
#if 0
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		DWORD size2 = *size;
		int v = RegQueryValueEx(rk, name, 0, NULL, NULL, &size2) == ERROR_SUCCESS;
		*size = size2;
		return v;
#endif
	}
}

int regsetdata (UAEREG *root, const TCHAR *name, const void *str, int size)
{
	if (inimode) {
		uae_u8 *in = (uae_u8*)str;
		DWORD ret;
		int i;
		TCHAR *tmp = xmalloc (TCHAR, size * 2 + 1);
		for (i = 0; i < size; i++)
			_stprintf (tmp + i * 2, _T("%02X"), in[i]); 
		ret = WritePrivateProfileString (gs (root), name, tmp, inipath);
		xfree (tmp);
		return ret;
	} else {
    TODO();
#if 0
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		return RegSetValueEx(rk, name, 0, REG_BINARY, (BYTE*)str, size) == ERROR_SUCCESS;
#endif
	}
}
int regquerydata (UAEREG *root, const TCHAR *name, void *str, int *size)
{
	if (inimode) {
		int csize = (*size) * 2 + 1;
		int i, j;
		int ret = 0;
		TCHAR *tmp = xmalloc (TCHAR, csize);
		uae_u8 *out = (uae_u8*)str;

		if (!regquerystr (root, name, tmp, &csize))
			goto err;
		j = 0;
		for (i = 0; i < _tcslen (tmp); i += 2) {
			TCHAR c1 = toupper(tmp[i + 0]);
			TCHAR c2 = toupper(tmp[i + 1]);
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
	} else {
    TODO();
#if 0
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		DWORD size2 = *size;
		int v = RegQueryValueEx(rk, name, 0, NULL, (LPBYTE)str, &size2) == ERROR_SUCCESS;
		*size = size2;
		return v;
#endif
	}
}

int regdelete (UAEREG *root, const TCHAR *name)
{
	if (inimode) {
		WritePrivateProfileString (gs (root), name, NULL, inipath);
		return 1;
	} else {
    TODO();
#if 0
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		return RegDeleteValue (rk, name) == ERROR_SUCCESS;
#endif
	}
}

int regexists (UAEREG *root, const TCHAR *name)
{
  CFG_File config;
  int ret = 1;
  const TCHAR *tmp;

  DebOut("entered\n");
  DebOut("inipath: %s\n", inipath);
  DebOut("name: %s\n", name);

  if (CFG_OK != CFG_OpenFile("PROGDIR:winuae.ini", &config)) {
    DebOut("winuae.ini does not exist\n");
    return 0;
  }

  DebOut("opened PROGDIR:winuae\n");

  tmp=CFG_ReadText(name, PUPPA);
  if (!_tcscmp (tmp, PUPPA)) {
    DebOut("created new file\n");
    ret = 0;
  }

  DebOut("File was already there (%s = %s)\n", name, tmp);

  CFG_CloseFile(&config);

  return ret;
}

void regdeletetree (UAEREG *root, const TCHAR *name)
{
  DebOut("entered\n");
	if (inimode) {
		TCHAR *s = gsn (root, name);
		if (!s)
			return;
		WritePrivateProfileSection (s, _T(""), inipath);
		xfree (s);
	} else {
    TODO();
#if 0
		HKEY rk = gr (root);
		if (!rk)
			return;
		SHDeleteKey (rk, name);
#endif
	}
}

int regexiststree (UAEREG *root, const TCHAR *name)
{
  int res;

  DebOut("entered\n");
  DebOut("root: %s\n", root);
  DebOut("name: %s\n", name);

  res=(int) CFG_GroupExists(name);

  DebOut("result: %d\n", res);

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

  CFG_SelectGroup(name, true);

  DebOut("created key: %lx\n", fkey);
  DebOut("selected group: %s\n", name);

	return fkey;
}

void regclosetree (UAEREG *key)
{
  DebOut("entered (key: %lx)\n", key);
	if (!key)
		return;
#if 0
	if (key->fkey)
		RegCloseKey (key->fkey);
	xfree (key->inipath);
	xfree (key);
#endif
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

  CFG_SelectGroup("Warning", true);
  CFG_WriteText("info1", "This is unsupported file. Compatibility between versions is not guaranteed.");
  CFG_WriteText("info2", "Incompatible ini-files may be re-created from scratch!");
  CFG_WriteText("info0", "This file is not for Windows. It is for AROS, really!");

  CFG_SaveFile(INI_FILE, CFG_SORT_ORIGINAL, CFG_COMPRESS_OUTPUT);
  CFG_CloseFile(NULL);

  DebOut("%s saved\n");

	return 1;
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
