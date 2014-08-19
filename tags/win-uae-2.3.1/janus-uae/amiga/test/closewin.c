/*
 * small program to send a close message to a window
 *
 * call with no Args to get a list of all windows
 * call with window pointer to send a close message to the window
 *
 * Public Domain Source
 *
 * (c) 2009 Oliver Brunner
 */

#include <stdio.h>

#include <intuition/intuitionbase.h>
#include <proto/exec.h>

struct IntuitionBase *IntuitionBase;

void closewin(struct Window *w) {
  struct MsgPort *myport;
  struct MsgPort *winport;
  struct IntuiMessage *closemsg;
  struct Message *backmsg;


  /* use Forbid() here, if you want to be sure, your window is still around */
  if(!w) {
    printf("window is NULL\n");
    return;
  }

  /* this is only a prototype, in real life, you should check, 
   * if w is really a window!  */

  printf("try to close window %lx (%s)\n",w,w->Title);

  winport=w->UserPort;
  printf("window->UserPort: %lx\n",winport);
  if(!winport) {
    /* Cli has no UserPort ..? */
    printf("ERROR: window %lx has no UserPort !?\n",w);
    return;
  }

  myport=CreateMsgPort();
  if(!myport) {
    printf("ERROR: no port!\n");
    return;
  }

  closemsg=AllocVec(sizeof(struct IntuiMessage),MEMF_CLEAR|MEMF_PUBLIC);

  ((struct Message *)closemsg)->mn_Length=sizeof(closemsg);
  ((struct Message *)closemsg)->mn_ReplyPort=myport;
  closemsg->Class=IDCMP_CLOSEWINDOW;
  closemsg->Qualifier=0x800;

  printf("send msg %lx ..\n",closemsg);
  PutMsg(winport, (struct Msg *) closemsg);
  printf("sent msg\n");

  printf("WaitPort(%lx)\n",myport);
  backmsg=WaitPort(myport);
  printf("backmsg: %lx\n",backmsg);

  if(backmsg != (struct Msg *) closemsg) {
    printf("FATAL ERROR: Got back another message as we sent !?\n");
    /* this might crash !! */
  }
  FreeVec(closemsg);
  DeleteMsgPort(myport);
  return;
}

int main (int argc, char **argv) {

  struct Window        *w;
  struct Screen *screen;

  IntuitionBase= (struct IntuitionBase*)OpenLibrary("intuition.library",36L);

  screen=IntuitionBase->FirstScreen;

  if(!argv[1]) {
    /* no args, simply list windows */
    w=screen->FirstWindow;
    while(w) {
      printf("%lx: %s\n",w,w->Title);
      w=w->NextWindow;
    }
  }
  else {
    sscanf(argv[1],"%x",&w);
    closewin(w);
  }

  CloseLibrary((struct Library *) IntuitionBase);
}
