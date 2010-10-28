/*
 * small program to list all public screens with windows
 *
 * (c) 2009 Oliver Brunner
 *
 * Public Domain.
 *
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>

#include <exec/devices.h>
#include <exec/interrupts.h>
#include <exec/nodes.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <intuition/intuitionbase.h>
#include <intuition/preferences.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <devices/timer.h>
#include <hardware/intbits.h>

#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/timer_protos.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/timer.h>

struct IntuitionBase *IntuitionBase;

int main (int argc, char **argv) {

  struct List *public_screen_list;
  struct PubScreenNode *public_screen_node;
  struct Window        *w;

  IntuitionBase= (struct IntuitionBase*)OpenLibrary("intuition.library",36L);

  public_screen_list = LockPubScreenList();
  public_screen_node = (struct PubScreenNode *) public_screen_list->lh_Head;
  while (public_screen_node && public_screen_node->psn_Screen) {
    printf("public_screen_node->psn_Node.ln_Name: >%s<\n",public_screen_node->psn_Node.ln_Name);
    printf("                  ->psn_Screen: %lx\n",public_screen_node->psn_Screen);
      w=public_screen_node->psn_Screen->FirstWindow;
      printf("public_screen_node->psn_Screen->Windows:\n");
      while(w) {
	printf("  %lx (%ls)\n",w, w->Title);
	w=w->NextWindow;
      }
    printf("public_screen_node->psn_Node.ln_succ: %lx\n",(struct PubScreenNode *) public_screen_node->psn_Node.ln_Succ);
    printf("\n");
    public_screen_node = (struct PubScreenNode *) public_screen_node->psn_Node.ln_Succ;
  }

  UnlockPubScreenList();

  CloseLibrary(IntuitionBase);

}
