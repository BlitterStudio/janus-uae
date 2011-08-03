/************************************************************************ 
 *
 * AROS cfg options
 *
 * (win32 stores stuff like win32_active_priority etc with the help of
 *  those functions). TODO
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

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"

void target_fixup_options (struct uae_prefs *p) {
}

void target_default_options (struct uae_prefs *p, int type) {
}

void target_save_options (struct zfile *f, struct uae_prefs *p) {
}

int target_cfgfile_load (struct uae_prefs *p, const TCHAR *filename, int type, int isdefault)
{
  TODO();
}

int target_parse_option (struct uae_prefs *p, const TCHAR *option, const TCHAR *value) 
//int target_parse_option (struct uae_prefs *p, char *option, char *value)
{
  TODO();
}

TCHAR *target_expand_environment (const TCHAR *path)
{
	return my_strdup(path);
}

