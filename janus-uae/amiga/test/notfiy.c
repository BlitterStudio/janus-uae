#include <stdio.h>
#include <stdlib.h>

#include <exec/types.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <proto/intuition.h>

#define IPTR ULONG
#define JWLOG printf

do_notify(struct Screen *screen, struct MsgPort *port, ULONG class, ULONG thread) {
  IPTR notify;

  if(screen) {
    notify=StartScreenNotifyTags(SNA_MsgPort,  port,
            SNA_UserData, screen,
            SNA_SigTask,  thread,
            SNA_Notify,   class,
            TAG_END); 
  }
  else {
    notify=StartScreenNotifyTags(SNA_MsgPort,  port,
            SNA_UserData, screen,
            SNA_SigTask,  thread,
            SNA_Notify,   class,
            TAG_END); 

  }

  if(!notify) {
    printf("ERROR: StartScreenNotifyTags(%lx) failed\n", class);
  }
}

int main(void) {
  struct Screen *screen;
  struct MsgPort             *port;
  ULONG                       notify_class, notify_code, notify_object, notify_userdata;
  ULONG                       notify_signal;
  struct IntuiMessage        *intui_msg;
  UWORD                       intui_code, intui_qualifier;
  ULONG                       intui_class;
  ULONG                       signals;
  ULONG thread= (ULONG) FindTask(NULL);
  BOOL done;
  struct ScreenNotifyMessage *notify_msg;

  printf("started\n");

  screen=IntuitionBase->FirstScreen;

  printf("Screen: %lx\n", screen);

  port=CreateMsgPort();

  do_notify(screen, port, SNOTIFY_AFTER_OPENSCREEN, thread);
  do_notify(screen, port, SNOTIFY_BEFORE_CLOSESCREEN, thread);
  do_notify(screen, port, SNOTIFY_AFTER_OPENWINDOW, thread);
  do_notify(screen, port, SNOTIFY_BEFORE_CLOSEWINDOW, thread);
  do_notify(screen, port, SNOTIFY_AFTER_CLOSESCREEN, thread);
  do_notify(screen, port, SNOTIFY_AFTER_CLOSEWINDOW, thread);
  do_notify(screen, port, SNOTIFY_BEFORE_OPENSCREEN, thread);
  do_notify(screen, port, SNOTIFY_BEFORE_OPENWINDOW, thread);

  notify_signal=1L << port->mp_SigBit;

  done=FALSE;
  while(!done) {
    JWLOG("wait for signal..\n");

    signals = Wait(notify_signal | SIGBREAKF_CTRL_C);
    JWLOG("signal reveived\n");

    if(signals & SIGBREAKF_CTRL_C) {
      JWLOG("SIGBREAKF_CTRL_C received\n");
      done=TRUE;
      break;
    } else 
    if(!done && (signals & notify_signal)) {
      JWLOG("notify_signal received\n");

      while((notify_msg = (struct ScreenNotifyMessage *) GetMsg (port))) {
        notify_class     = notify_msg->snm_Class;
        notify_code      = notify_msg->snm_Code;
        notify_object    = notify_msg->snm_Object;
        notify_userdata  = notify_msg->snm_UserData;

        ReplyMsg ((struct Message*) notify_msg); 
        
        switch(notify_class) {
          case SNOTIFY_AFTER_OPENSCREEN: printf("SNOTIFY_AFTER_OPENSCREEN\n");
            break;
          case SNOTIFY_BEFORE_CLOSESCREEN: printf("SNOTIFY_BEFORE_CLOSESCREEN\n");
            break;
          case SNOTIFY_AFTER_OPENWINDOW: printf("SNOTIFY_AFTER_OPENWINDOW\n");
            break;
          case SNOTIFY_BEFORE_CLOSEWINDOW: printf("SNOTIFY_BEFORE_CLOSEWINDOW\n");
            break;
          case SNOTIFY_AFTER_CLOSESCREEN: printf("SNOTIFY_AFTER_CLOSESCREEN\n");
            break;
          case SNOTIFY_AFTER_CLOSEWINDOW: printf("SNOTIFY_AFTER_CLOSEWINDOW\n");
            break;
          case SNOTIFY_BEFORE_OPENSCREEN: printf("SNOTIFY_BEFORE_OPENSCREEN\n");
            break;
          case SNOTIFY_BEFORE_OPENWINDOW: printf("SNOTIFY_BEFORE_OPENWINDOW\n");
            break;
          default: printf("unknown(%lx)\n", notify_class);
            break;
        }
        JWLOG("    notify_object %lx screen %lx\n", notify_object, screen);

//	if(notify_object == (ULONG) screen) {
//	  JWLOG("aros_cscr_thread[%lx]: own notify_object %lx\n", thread, notify_object);
	  /* this message is for us */
	  //switch (notify_class) {
	      //case SNOTIFY_SCREENDEPTH:
	    	//handle_msg(jscreen, notify_class, notify_code, notify_object, thread);
  	      //break;
  	  //}
//	}
//	else {
//	  JWLOG("aros_cscr_thread[%lx]: foreign notify_object %lx\n", thread, notify_object);
//	}
      }
    }

  } /* while(done) */







}
