#ifndef _REGISTRY_H_
#define _REGISTRY_H_

#include <exec/types.h>

#include "SDL_Config/SDL_config_lib.h"

#define ULONGLONG long long

typedef struct UAEREG {
    TCHAR    *inipath;
} UAEREG;

#define INI_FILE "PROGDIR:winuae.ini"

extern int getregmode (void);
extern int reginitializeinit (TCHAR **path);
extern void regstatus (void);

extern int regsetstr (UAEREG*, const TCHAR *name, const TCHAR *str);
extern int regsetint (UAEREG*, const TCHAR *name, int val);
extern int regqueryint (UAEREG*, const TCHAR *name, int *val);
extern int regquerystr (UAEREG*, const TCHAR *name, TCHAR *str, int *size);
extern int regsetlonglong (UAEREG *root, const TCHAR *name, ULONGLONG val);
extern int regquerylonglong (UAEREG *root, const TCHAR *name, ULONGLONG *val);

extern int regdelete (UAEREG*, const TCHAR *name);
extern void regdeletetree (UAEREG*, const TCHAR *name);

extern int regexists (UAEREG*, const TCHAR *name);
extern int regexiststree (UAEREG *, const TCHAR *name);

extern int regquerydatasize (UAEREG *root, const TCHAR *name, int *size);
extern int regsetdata (UAEREG*, const TCHAR *name, const void *str, int size);
extern int regquerydata (UAEREG *root, const TCHAR *name, void *str, int *size);

extern int regenumstr (UAEREG*, int idx, TCHAR *name, int *nsize, TCHAR *str, int *size);

extern UAEREG *regcreatetree (UAEREG*, const TCHAR *name);
extern void regclosetree (UAEREG *key);

extern void regflushfile (void);

#endif

