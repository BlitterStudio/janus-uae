/************************************************************************ 
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

#include "j.h"

/*m68k_results:
 * status (LONG)
 * position to path inside m68k_results (LONG)
 * position to filename inside m68k_results (LONG)
 * data (..)
 */
#define NR_LONGS 3

uae_u32 ld_job_get(ULONG *m68k_results) {

  char        *str_results;
  char        *sep;
  GSList      *first  =NULL;
  JanusLaunch *jlaunch=NULL;

  JWLOG("entered\n");

  ObtainSemaphore(&sem_janus_launch_list);
  if(janus_launch) {
    first  =g_slist_nth(janus_launch, 0);
    if(first) {
      jlaunch=(struct JanusLaunch *) first->data;
      janus_launch=g_slist_delete_link(janus_launch, first);
    }
  }
  ReleaseSemaphore(&sem_janus_launch_list);

  if(!jlaunch) {
    JWLOG("jlaunch == NULL !?\n");
    return;
  }

  JWLOG("found jlaunch %lx (%s)\n", jlaunch, jlaunch->amiga_path);

  /* OK */
  put_long_p(m68k_results, 1);

  /* start position of path */
  put_long_p(m68k_results+1, NR_LONGS*4);  

  /* start of result strings */
  str_results=(char *) get_real_address(m68k_results) + NR_LONGS*4 ;

  /* TODO: check, if string fits!!!! */

  /* copy full path */
  strcpy(str_results, jlaunch->amiga_path);

  /* seperate path and filename */
  sep=PathPart(str_results);
  sep[0]=(char) 0;

  /* start of filename */
  put_long_p(m68k_results+2, NR_LONGS*4 + strlen(str_results) + 1);  

  return 1;
}
    
