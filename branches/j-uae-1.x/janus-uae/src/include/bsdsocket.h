/************************************************************************ 
 *
 * bsdsocket.library emulation
 *
 * Copyright 1997,98 Mathias Ortmann
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
 * This file is based on a parts of cbio.c, (c) 1992 Commodore-Amiga.
 *
 * $Id$
 *
 ************************************************************************/

#ifndef __BSDSOCKET__
#define __BSDSOCKET__

#define TRACING_ENABLED

#ifdef TRACING_ENABLED
//#define TRACE(x) do { write_log x; } while(0)
#define TRACE(x) do { kprintf("%s:%d  %s(): ",__FILE__,__LINE__,__func__);kprintf(x);kprintf("\n"); } while(0)
#else
#define TRACE(x)
#endif

extern int init_socket_layer (void);
extern void deinit_socket_layer (void);

/* inital size of per-process descriptor table (currently fixed) */
#define DEFAULT_DTABLE_SIZE 64

#define SCRATCHBUFSIZE 128

#define MAXPENDINGASYNC 512

#define MAXADDRLEN 256

/* allocated and maintained on a per-task basis */
struct socketbase {
    struct socketbase *next;
    struct socketbase *nextsig;	/* queue for tasks to signal */

    int dosignal;		/* signal flag */
    uae_u32 ownertask;		/* task that opened the library */
    int signal;			/* signal allocated for that task */
    int sb_errno, sb_herrno;	/* errno and herrno variables */
    uae_u32 errnoptr, herrnoptr;	/* pointers */
    uae_u32 errnosize, herrnosize;	/* pinter sizes */
    int dtablesize;		/* current descriptor/flag etc. table size */
    int *dtable;		/* socket descriptor table */
    int *ftable;		/* socket flags */
    int resultval;
    uae_u32 hostent;		/* pointer to the current hostent structure (Amiga mem) */
    uae_u32 hostentsize;
    uae_u32 protoent;		/* pointer to the current protoent structure (Amiga mem) */
    uae_u32 protoentsize;
    uae_u32 servent;		/* pointer to the current servent structure (Amiga mem) */
    uae_u32 serventsize;
    uae_u32 sigstosend;
    uae_u32 eventsigs;		/* EVENT sigmask */
    uae_u32 eintrsigs;		/* EINTR sigmask */
    int eintr;			/* interrupted by eintrsigs? */
    int eventindex;		/* current socket looked at by GetSocketEvents() to prevent starvation */

#ifdef __AROS__
    struct Library    *bsdsocketbase;
    struct Process    *aros_task;
    struct MsgPort    *aros_port;
    struct MsgPort    *reply_port;
    void              *mempool;   /* alloc everything here */
    char              *taskname;  /* AllocPoolVec'ed */

#endif

    /* host-specific fields below */
#ifdef _WIN32
    unsigned int sockAbort;	/* for aborting WinSock2 select() (damn Microsoft) */
    unsigned int sockAsync;	/* for aborting WSBAsyncSelect() in window message handler */
    int needAbort;		/* abort flag */
    void *hAsyncTask;		/* async task handle */
    void *hEvent;		/* thread event handle */
    unsigned int *mtable;	/* window messages allocated for asynchronous event notification */
#else
    uae_sem_t sem;		/* semaphore to notify the socket thread of work */
    uae_thread_id thread;	/* socket thread */
    int  sockabort[2];		/* pipe used to tell the thread to abort a select */
    int action;
    int s;			/* for accept */
    uae_u32 name;		/* For gethostbyname */
    uae_u32 a_addr;		/* gethostbyaddr, accept */
    uae_u32 a_addrlen;		/* for gethostbyaddr, accept */
    uae_u32 flags;
    void *buf;
    uae_u32 len;
    uae_u32 to, tolen, from, fromlen;
    int nfds;
    uae_u32 sets [3];
    uae_u32 timeout;
    uae_u32 sigmp;
#endif
} *socketbases;


#define LIBRARY_SIZEOF 36

struct UAEBSDBase {
    char dummy[LIBRARY_SIZEOF];
    struct socketbase *sb;
    char scratchbuf[SCRATCHBUFSIZE];
};

/* socket flags */
/* socket events to report */
#define REP_ACCEPT	 0x01	/* there is a connection to accept() */
#define REP_CONNECT	 0x02	/* connect() completed */
#define REP_OOB		 0x04	/* socket has out-of-band data */
#define REP_READ	 0x08	/* socket is readable */
#define REP_WRITE	 0x10	/* socket is writeable */
#define REP_ERROR	 0x20	/* asynchronous error on socket */
#define REP_CLOSE	 0x40	/* connection closed (graceful or not) */
#define REP_ALL      0x7f
/* socket events that occurred */
#define SET_ACCEPT	 0x0100	/* there is a connection to accept() */
#define SET_CONNECT	 0x0200	/* connect() completed */
#define SET_OOB		 0x0400	/* socket has out-of-band data */
#define SET_READ	 0x0800	/* socket is readable */
#define SET_WRITE	 0x1000	/* socket is writeable */
#define SET_ERROR	 0x2000	/* asynchronous error on socket */
#define SET_CLOSE	 0x4000	/* connection closed (graceful or not) */
#define SET_ALL      0x7f00
/* socket properties */
#define SF_BLOCKING 0x80000000
#define SF_BLOCKINGINPROGRESS 0x40000000


extern uae_u32 addstr (uae_u32 *, const char *);
extern uae_u32 addmem (uae_u32 *, const char *, int len);

extern char *strncpyah (char *, uae_u32, int);
extern char *strcpyah (char *, uae_u32);
extern uae_u32 strcpyha (uae_u32, const char *);
extern uae_u32 strncpyha (uae_u32, const char *, int);

#define SB struct socketbase *sb

#ifndef _WIN32
typedef int SOCKET;
#define INVALID_SOCKET -1
#endif

extern void bsdsocklib_seterrno (SB, int);
extern void bsdsocklib_setherrno (SB, int);

extern void sockmsg (unsigned int, unsigned long, unsigned long);
extern void sockabort (SB);

extern void addtosigqueue (SB, int);
extern void removefromsigqueue (SB);
extern void sigsockettasks (void);
extern void locksigqueue (void);
extern void unlocksigqueue (void);

#define BOOL int
extern BOOL checksd(SB, int sd);
extern void setsd(SB, int ,int );
extern int getsd (SB, int);
extern int getsock (SB, int);
extern void releasesock (SB, int);

extern void waitsig (TrapContext *context, SB);
extern void cancelsig (TrapContext *context, SB);

extern int host_sbinit (TrapContext *, SB);
extern void host_sbcleanup (SB);
extern void host_sbreset (void);
extern void host_closesocketquick (int);

extern int host_dup2socket (SB, int, int);
extern int host_socket (SB, int, int, int);
extern uae_u32 host_bind (SB, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_listen (SB, uae_u32, uae_u32);
extern void host_accept (TrapContext *, SB, uae_u32, uae_u32, uae_u32);
extern void host_sendto (TrapContext *, SB, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32);
extern void host_recvfrom (TrapContext *, SB, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_shutdown (SB, uae_u32, uae_u32);
extern void host_setsockopt (SB, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_getsockopt (SB, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_getsockname (SB, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_getpeername (SB, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_IoctlSocket (SB, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_shutdown (SB, uae_u32, uae_u32);
extern int host_CloseSocket (SB, int);
extern void host_connect (TrapContext *, SB, uae_u32, uae_u32, uae_u32);
extern void host_WaitSelect (TrapContext *, SB, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_SetSocketSignals (void);
extern uae_u32 host_getdtablesize (void);
extern uae_u32 host_ObtainSocket (void);
extern uae_u32 host_ReleaseSocket (void);
extern uae_u32 host_ReleaseCopyOfSocket (void);
extern uae_u32 host_Inet_NtoA (TrapContext *context, SB, uae_u32);
extern uae_u32 host_inet_addr (TrapContext *context, SB, uae_u32);
extern uae_u32 host_Inet_LnaOf (void);
extern uae_u32 host_Inet_NetOf (void);
extern uae_u32 host_Inet_MakeAddr (void);
extern uae_u32 host_inet_network (void);
extern void host_gethostbynameaddr (TrapContext *, SB, uae_u32, uae_u32, long);
extern uae_u32 host_getnetbyname (void);
extern uae_u32 host_getnetbyaddr (void);
extern void host_getservbynameport (TrapContext *, SB, uae_u32, uae_u32, uae_u32);
extern void host_getprotobyname (TrapContext *, SB, uae_u32);
extern void host_getprotobynumber (TrapContext *, SB, uae_u32);
extern uae_u32 host_vsyslog (void);
extern uae_u32 host_Dup2Socket (void);
extern uae_u32 host_gethostname (struct socketbase *sb,uae_u32, uae_u32);


extern void bsdlib_install (void);
extern void bsdlib_reset (void);

#ifdef __AROS__
struct Library *getsocketbase(struct socketbase *sb);

int  aros_bsdsocket_start_thread (struct socketbase *sb);
void aros_bsdsocket_kill_thread(struct socketbase *sb);

extern void host_gethostbynameaddr_real (TrapContext *, SB, uae_u32, uae_u32, long);
extern int host_socket_real (SB, int, int, int);
extern void host_setsockopt_real (SB, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_inet_addr_real(TrapContext *context, struct socketbase *sb, uae_u32 cp);
void host_getprotobyname_real(TrapContext *context, struct socketbase *sb, uae_u32 name);
uae_u32 host_Inet_NtoA_real(TrapContext *context, struct socketbase *sb, uae_u32 in);
int host_CloseSocket_real(struct socketbase *sb, int sd);
int host_dup2socket_real(struct socketbase *sb, int fd1, int fd2);
uae_u32 host_bind_real(struct socketbase *sb, uae_u32 sd, uae_u32 name, uae_u32 namelen);
void host_connect_real(TrapContext *context, struct socketbase *sb, uae_u32 sd, uae_u32 addr68k, uae_u32 addrlen);
void host_sendto_real(TrapContext *context, struct socketbase *sb, uae_u32 sd, uae_u32 msg, uae_u32 len, uae_u32 flags, uae_u32 to, uae_u32 tolen);
void host_recvfrom_real(TrapContext *context, struct socketbase *sb, uae_u32 sd, uae_u32 msg, uae_u32 len, uae_u32 flags, uae_u32 addr, uae_u32 addrlen);
void aros_bsdsocket_kill_thread_real(struct socketbase *sb);
uae_u32 host_gethostname_real(struct socketbase *sb, uae_u32 name, uae_u32 namelen);
uae_u32 host_getsockname_real(struct socketbase *sb, uae_u32 sd, uae_u32 name, uae_u32 namelen);
uae_u32 host_getpeername_real(struct socketbase *sb, uae_u32 sd, uae_u32 name, uae_u32 namelen);


struct JUAE_bsdsocket_Message {
  struct Message  ExecMessage;
  LONG            cmd;
  LONG            block;
  ULONG           a;
  ULONG           b;
  ULONG           c;
  ULONG           d;
  ULONG           e;
  ULONG           f;
  ULONG           g;
  ULONG           h;
  ULONG           ret;
};

#define BSD_gethostbynameaddr 1
#define BSD_socket            2
#define BSD_setsockopt        3
#define BSD_connect           4
#define BSD_sendto            5
#define BSD_recvfrom          6
#define BSD_inet_addr         7
#define BSD_getprotobyname    8
#define BSD_Inet_NtoA         9
#define BSD_CloseSocket      10
#define BSD_dup2socket       11
#define BSD_bind             12
#define BSD_gethostname      13
#define BSD_getsockname      14
#define BSD_getpeername      15


#define BSD_killme           -1

#endif
#endif /* __BSDSOCKET__ */
