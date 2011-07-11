/************************************************************************ 
 *
 * some locking helpers
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
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

#include "od-amiga/j.h"

extern BOOL init_done;

/*****************************************************
 * obtain_W/release_W
 *
 * As W does not change any more, there most
 * likely is no need, to protect W access.
 * But I'll leave it there, just in case..
 *****************************************************/
void obtain_W(void) {
  if(!init_done) {
    return;
  }
//  ObtainSemaphore(&sem_janus_access_W);
  return;
}

void release_W(void) {
  if(!init_done) {
    return;
  }

 // ReleaseSemaphore(&sem_janus_access_W);
  return;
}