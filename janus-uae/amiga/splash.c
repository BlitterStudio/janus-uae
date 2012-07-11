/************************************************************************ 
 *
 * Splash Control
 *
 * Copyright 2012 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE
 *
 * Janus-UAE is free software: you can redistribute it and/or modify
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
 * $Id$
 *
 ************************************************************************/

/*
 * amnipulate splash window contents during booting..
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <clib/exec_protos.h>

#include <exec/memory.h>


#include "janus-daemon.h"

char verstag[] = "\0$VER: janus-splash 1.0";

/*
 * d0 is the function to be called (AD_*)
 * d1 is the size of the memory supplied by a0
 * a0 memory, where to put out command in and 
 *    where we store the result
 */
ULONG (*calltrap)(ULONG __asm("d0"),
                  ULONG __asm("d1"),
		  APTR  __asm("a0")) = (APTR) AROSTRAPBASE;


void usage() {
    printf("usage..\n");
}


/*
 * launch options:
 *
 * janus-splash
 *  - show options
 *
 * janus-splash close
 *  - close window immediately
 *
 * janus-splash display <time in seconds> <text>
 *
 */
  
int main (int argc, char **argv) {

  ULONG *command_mem=NULL;
  LONG time;
  ULONG len;
  char *text;

  C_ENTER

  DebOut("janus-splash: started\n");
  printf("argc: %d\n", argc);

  if(argc==1 || argc>4) {
    usage();
    goto DONE;
  }

  if(argc==2) {
    if(strcmp(argv[1], "close")) {
      usage();
      goto DONE;
    }
    /* close */
    time=0;
    printf("TODO!\n");
  }

  printf("argv[1]: %s\n", argv[1]);
  printf("argv[2]: %s\n", argv[2]);
  printf("argv[3]: %s\n", argv[3]);

  time=atoi(argv[2]);

  printf("time: %d\n",time);

  len=strlen(argv[3]);

  /* own string buffer is not really necessary here, but I had troubles getting
   * it to work without it. So I don't care about those few bytes here.
   */
  text=AllocVec(len+1, MEMF_CLEAR);

  strncpy(text, argv[3], len);
  printf("text: %s\n", text);
  printf("text: %lx\n", text);

  if(len>AD__MAXMEM-16) {
    printf("ERROR: text length is limited to %d characters only!\n", AD__MAXMEM-16);
    goto DONE;
  }

  command_mem=AllocVec(AD__MAXMEM,MEMF_CLEAR);

  if(!command_mem) {
    printf("ERROR: AllocVec failed!\n");
    goto DONE;
  }

  command_mem[ 0]=(ULONG) time;
  command_mem[ 1]=(ULONG) 0; /* reserved */
  command_mem[ 2]=(ULONG) strlen(argv[3]);
  command_mem[ 3]=(ULONG) text; /* pointer to string */

  printf ("==> send AD_GET_JOB_SPLASH\n");

  calltrap (AD_GET_JOB, AD_GET_JOB_SPLASH, command_mem);

DONE:
  if(command_mem) {
    FreeVec(command_mem);
  }

  if(text) {
    FreeVec(text);
  }

  DebOut("janus-splash: exit\n");

  C_LEAVE
}
