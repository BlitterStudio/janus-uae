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

#define JWTRACING_ENABLED 1
#include "j.h"

/*m68k_results:
 * status (LONG)
 * position to path inside m68k_results (LONG)
 * position to filename inside m68k_results (LONG)
 * data (..)
 *
 * attention: jlaunch->args can be NULL in case there are no args,
 *            so every acces to jlaunch->args[] crashes
 */
#define NR_LONGS 4 /* status, offset of path, offset of filename and terminating 0 */

uae_u32 ld_job_get(ULONG *m68k_results) {

  char        *str_results;
  char        *results;
  char        *sep;
  GSList      *first  =NULL;
  JanusLaunch *jlaunch=NULL;
  ULONG        nr_args;
  ULONG        i;
  ULONG        target;

  JWLOG("entered\n");

  ObtainSemaphore(&sem_janus_launch_list);
  if(janus_launch) {
    first  =g_slist_nth(janus_launch, 0);
    if(first) {
      jlaunch=(JanusLaunch *) first->data;
      janus_launch=g_slist_delete_link(janus_launch, first);
    }
  }
  ReleaseSemaphore(&sem_janus_launch_list);

  if(!jlaunch) {
    JWLOG("jlaunch == NULL\n");
    /* no more commands */
    put_long_p(m68k_results, 0);
    return 0;
  }

  JWLOG("found jlaunch %lx (%s)\n", jlaunch, jlaunch->amiga_path);

  /* count nr_args */
  nr_args=0;
  if(jlaunch->args) {
    while(jlaunch->args[nr_args]) {
      nr_args++;
    }
  }

  JWLOG("nr_args: %d\n", nr_args);

  /* status/type: OK/WB/CLI */
  put_long_p(m68k_results, jlaunch->type);
  JWLOG("type is %d\n", jlaunch->type);

  /* start position of path */
  put_long_p(m68k_results+1, (nr_args+NR_LONGS)*4);  
  JWLOG("DEBOUT: m68k_results[%d]=%d (%lx)\n", 1, get_long(m68k_results+1), get_long(m68k_results+1));

  results=(char *) get_real_address((LONG) m68k_results);

  /* start of result strings */
  str_results=results + (nr_args+NR_LONGS)*4 ;

  /* TODO: check, if string fits!!!! */

  /* copy full path */
  strcpy(str_results, jlaunch->amiga_path);
  JWLOG("str_results: %s\n", str_results);

  /* seperate path and filename */
  sep=PathPart(str_results);
  sep[0]=(char) 0;
  JWLOG("path: %s\n", str_results);

  /* start of filename */
  put_long_p(m68k_results+2, (nr_args+NR_LONGS)*4 + strlen(str_results) + 1);  
  JWLOG("DEBOUT: m68k_results[%d]=%d (%lx)\n", 2, get_long(m68k_results+2), get_long(m68k_results+2));
  JWLOG("DEBOUT: --\n");
  JWLOG("filename: %s\n", results + (nr_args+NR_LONGS)*4 + strlen(str_results) + 1);

  /* copy arguments */

  /* there will be the first string of our arguments, if we have one */
  target=(nr_args+NR_LONGS)*4 + strlen(jlaunch->amiga_path) + 1;

  /* copy all args */
  for(i=0; jlaunch->args && (jlaunch->args[i]!=NULL); i++) {

    JWLOG("arg #%d target %d (%s)\n", i, target, jlaunch->args[i]);

    /* store offset */
    put_long_p(m68k_results+NR_LONGS-1+i, target); /* -1 is the count of the terminating 0 */
    /* copy string */
    strcpy(results+target, jlaunch->args[i]);

    target=target+strlen(jlaunch->args[i]+1);
    if(target > 8000) {
      JWLOG("ERROR: too many aruments!!\n");
    }

  }
  //put_long_p(m68k_results+NR_LONGS+i, 0); /* terminate*/

  for(i=0; i<50; i++) {
    JWLOG("DEBOUT: m68k_results[%d]=%d (%lx)\n", i, get_long(m68k_results+i), get_long(m68k_results+i));
  }

  JWLOG("free jlaunch..\n");
  if(jlaunch->amiga_path) {
    FreeVec(jlaunch->amiga_path);
    jlaunch->amiga_path=NULL;
  }

  if(jlaunch->args) {
    i=0;
    while(jlaunch->args[i]) {
      FreeVec(jlaunch->args[i]);
      i++;
    }
    FreeVec(jlaunch->args);
  }

  return 1;
}
    
