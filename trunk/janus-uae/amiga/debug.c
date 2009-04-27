/************************************************************************ 
 *
 * Janus-Daemon
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-Daemon.
 *
 * Janus-Daemon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Janus-Daemon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Janus-Daemon. If not, see <http://www.gnu.org/licenses/>.
 *
 ************************************************************************/

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <stdarg.h>

#include "janus-daemon.h"

/************************************************
 * DebOut
 *
 * write formatted string to AROS debug console
 ************************************************/
void DebOut(const char *format, ...) {
  char *command_mem;
  va_list args;

  command_mem=AllocVec(AD__MAXMEM, MEMF_CLEAR);

  va_start(args, format);
  vsnprintf(command_mem, AD__MAXMEM-1, format, args);
  va_end(args);

  calltrap (AD_GET_JOB, AD_GET_JOB_DEBUG, (ULONG *) command_mem);

  FreeVec(command_mem);
}


