/************************************************************************ 
 *
 * Copyright 2012 Oliver Brunner - aros<at>oliver-brunner.de
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

#define JWTRACING_ENABLED 1
#define JW_ENTER_ENABLED 1
#include "j.h"

/**********************************************************
 * ad_job_splash_screen
 *
 * set timeout and text for splash window
 *
 **********************************************************/

void do_splash(char *text, int time);

static char text[1024];

uae_u32 ad_job_splash_screen(ULONG *m68k_results) {

  LONG time;

  ENTER

  time=get_long(m68k_results);

  kprintf("time: %d\n", time);
  kprintf("ulong 1: %d\n", get_long(m68k_results+1)); /* reserved */
  kprintf("ulong 2: %d (length)\n", get_long(m68k_results+2)); /* length */
  kprintf("text: %s\n", get_real_address(get_long(m68k_results+3)));

  /* just to be sure, limit length */
  strncpy(text, get_real_address(get_long(m68k_results+3)), 1023);

  kprintf("text: %s\n", text);

  do_splash(text, time);

  LEAVE

  return TRUE;
}

