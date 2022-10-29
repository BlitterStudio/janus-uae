#ifndef UAE_STRING_H
#define UAE_STRING_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "uae/types.h"
#include <string.h>

#ifdef _WIN32
/* Make sure the real _tcs* functions are already declared before we
 * re-define them below. */
#include <tchar.h>
#include <wchar.h>
#include <stdlib.h>
#endif

#ifdef _WIN32
/* Using the real _tcs* functions */
#else
#define _istdigit isdigit
#define _istspace isspace
#define _istupper isupper
#ifndef _sntprintf
#define	_sntprintf	_snprintf
#endif
#define _stprintf sprintf
#ifndef _strtoui64
#if defined (__AROS__)
#define _strtoui64 strtoull
#else
#define _strtoui64 strtoll
#endif
#endif
#define _tcscat strcat
#define _tcschr strchr
#define _tcscmp strcmp
#define _tcscpy strcpy
#define _tcscspn strcspn
#ifndef _tcsdup
#define	_tcsdup		_strdup
#endif
#define _tcsftime strftime
#define _tcsftime strftime
#ifndef _tcsicmp
#define	_tcsicmp	_stricmp
#endif
#define _tcslen strlen
#define _tcsncat strncat
#define _tcsncmp strncmp
#define _tcsncpy strncpy
#ifndef _tcsnicmp
#define	_tcsnicmp	_strnicmp
#endif
#define _tcsrchr strrchr
#define _tcsspn strspn
#define _tcsstr strstr
#define _tcstod strtod
#define _tcstok strtok
#define _tcstol strtol
#define _totlower tolower
#define _totupper toupper
#define _tprintf printf
#define _tstof atof
#if defined(__AROS__)
#define _tstoi64 atoi
#else
#define _tstoi64 atoll
#endif
#define _tstoi atoi
#define _tstol atol
#define _vsnprintf vsnprintf
#define _vsntprintf vsnprintf
#endif

static inline size_t uae_tcslcpy(TCHAR *dst, const TCHAR *src, size_t size)
{
	if (size == 0) {
		return 0;
	}
	size_t src_len = _tcslen(src);
	size_t cpy_len = src_len;
	if (cpy_len >= size) {
		cpy_len = size - 1;
	}
	memcpy(dst, src, cpy_len * sizeof(TCHAR));
	dst[cpy_len] = _T('\0');
	return src_len;
}

static inline size_t uae_strlcpy(char *dst, const char *src, size_t size)
{
	if (size == 0) {
		return 0;
	}
	size_t src_len = strlen(src);
	size_t cpy_len = src_len;
	if (cpy_len >= size) {
		cpy_len = size - 1;
	}
	memcpy(dst, src, cpy_len);
	dst[cpy_len] = '\0';
	return src_len;
}

#endif /* UAE_STRING_H */
