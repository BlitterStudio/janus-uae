/************************************************************************ 
 *
 * bsdsocket.library emulation - AROS-dependent part
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

#define BSDSOCKET 1
#ifdef BSDSOCKET

#include <proto/bsdsocket.h>
#include <proto/exec.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include "sysconfig.h"
#include "sysdeps.h"

#if 0
#include <winsock.h>
#endif
#include <stddef.h>
#include <process.h>

#include "options.h"
#include "include/memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h" 
#include "autoconf.h"
#include "traps.h"
#include "td-amigaos/thread.h"
#include "bsdsocket.h"

#include "threaddep/thread.h"
#include "native2amiga.h"

#define BSDLOG_ENABLED 1
#if BSDLOG_ENABLED
#define BSDLOG(...)   do { kprintf("[%lx]%s:%d %s(): ",FindTask(NULL),__FILE__,__LINE__,__func__);kprintf(__VA_ARGS__); } while(0)
#else
#define BSDLOG(...)     do { ; } while(0)
#endif

struct Library *SocketBase;
#if 0

static HWND hSockWnd;
static long FAR PASCAL SocketWindowProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );

extern HWND hAmigaWnd;
int hWndSelector = 0; /* Set this to zero to get hSockWnd */
CRITICAL_SECTION csSigQueueLock;

DWORD threadid;
#ifdef __GNUC__
#define THREAD(func,arg) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)func,(LPVOID)arg,0,&threadid)
#else
#define THREAD(func,arg) _beginthreadex( NULL, 0, func, (void *)arg, 0, (unsigned *)&threadid )
#endif

#define SETERRNO seterrno(sb,WSAGetLastError()-WSABASEERR)
#define SETHERRNO setherrno(sb,WSAGetLastError()-WSABASEERR)
#define WAITSIGNAL waitsig(sb)

#define SETSIGNAL addtosigqueue(sb,0)
#define CANCELSIGNAL cancelsig(sb)

#define FIOSETOWN _IOW('f', 124, long)    /* set owner (struct Task *) */
#define FIOGETOWN _IOR('f', 123, long)    /* get owner (struct Task *) */

#define BEGINBLOCKING if (sb->ftable[sd-1] & SF_BLOCKING) sb->ftable[sd-1] |= SF_BLOCKINGINPROGRESS
#define ENDBLOCKING sb->ftable[sd-1] &= ~SF_BLOCKINGINPROGRESS

static WSADATA wsbData;

int PASCAL WSAEventSelect(SOCKET,HANDLE,long);

#define MAX_SELECT_THREADS 64
static HANDLE hThreads[MAX_SELECT_THREADS];
uae_u32 *threadargs[MAX_SELECT_THREADS];
static HANDLE hEvents[MAX_SELECT_THREADS];

#define MAX_GET_THREADS 64
static HANDLE hGetThreads[MAX_GET_THREADS];
uae_u32 *threadGetargs[MAX_GET_THREADS];
static HANDLE hGetEvents[MAX_GET_THREADS];


static HANDLE hSockThread;
static HANDLE hSockReq, hSockReqHandled;
static unsigned int __stdcall sock_thread(void *);

CRITICAL_SECTION SockThreadCS;
#define PREPARE_THREAD EnterCriticalSection( &SockThreadCS )
#define TRIGGER_THREAD { SetEvent( hSockReq ); WaitForSingleObject( hSockReqHandled, INFINITE ); LeaveCriticalSection( &SockThreadCS ); }

#define SOCKVER_MAJOR 2
#define SOCKVER_MINOR 2

#define SF_RAW_UDP 0x10000000
#define SF_RAW_RAW 0x20000000
#define SF_RAW_RUDP 0x08000000
#define SF_RAW_RICMP 0x04000000

typedef struct ip_option_information {
    u_char Ttl;		/* Time To Live (used for traceroute) */
    u_char Tos; 	/* Type Of Service (usually 0) */
    u_char Flags; 	/* IP header flags (usually 0) */
    u_char OptionsSize; /* Size of options data (usually 0, max 40) */
    u_char FAR *OptionsData;   /* Options data buffer */
} IPINFO, *PIPINFO, FAR *LPIPINFO;

static void bsdsetpriority (HANDLE thread)
{
//    int pri = os_winnt ? THREAD_PRIORITY_NORMAL : priorities[currprefs.win32_active_priority].value;
    int pri = THREAD_PRIORITY_NORMAL;
    SetThreadPriority(thread, pri);
}

static int mySockStartup( void )
{
    int result = 0;
    SOCKET dummy;
    DWORD lasterror;

    if (WSAStartup(MAKEWORD( SOCKVER_MAJOR, SOCKVER_MINOR ), &wsbData))
    {
	    lasterror = WSAGetLastError();
	    
	    if( lasterror == WSAVERNOTSUPPORTED )
	    {
//		char szMessage[ MAX_DPATH ];
//		WIN32GUI_LoadUIString( IDS_WSOCK2NEEDED, szMessage, MAX_DPATH );
//              gui_message( szMessage );
	    }
	    else
                write_log( "BSDSOCK: ERROR - Unable to initialize Windows socket layer! Error code: %d\n", lasterror );
	    return 0;
    }

    if (LOBYTE (wsbData.wVersion) != SOCKVER_MAJOR || HIBYTE (wsbData.wVersion) != SOCKVER_MINOR )
    {
//	char szMessage[ MAX_DPATH ];
//	WIN32GUI_LoadUIString( IDS_WSOCK2NEEDED, szMessage, MAX_DPATH );
//        gui_message( szMessage );

        return 0;
    }
    else
    {
        write_log( "BSDSOCK: using %s\n", wsbData.szDescription );
	// make sure WSP/NSPStartup gets called from within the regular stack
	// (Windows 95/98 need this)
	if( ( dummy = socket( AF_INET,SOCK_STREAM,IPPROTO_TCP ) ) != INVALID_SOCKET ) 
	{
	    closesocket( dummy );
	    result = 1;
	}
	else
	{
	    write_log( "BSDSOCK: ERROR - WSPStartup/NSPStartup failed! Error code: %d\n",WSAGetLastError() );
	    result = 0;
	}
    }

    return result;
}

#endif

static void seterrno (struct socketbase *sb, int err) {
  sb->sb_errno=err;
};

struct Library *getsocketbase(void) {

  struct Task *mytask;

  mytask=FindTask(NULL);

  BSDLOG("task: %lx\n", mytask);

  if(SocketBase) {
    BSDLOG("return already opened SocketBase: %lx\n", SocketBase);
    return SocketBase;
  }

  BSDLOG("OpenLibrary(\"bsdsocket.library\", 0) ..\n");;
  SocketBase = OpenLibrary("bsdsocket.library", 0);
  if(!SocketBase) {
    kprintf("Unable to open bsdsocket.library!!\n");
  }

  BSDLOG("return SocketBase %lx\n", SocketBase);

  return SocketBase;
}

static int socket_layer_initialized = 0;

int init_socket_layer(void) {

  
  int result = 0;

#if 0
#ifndef CAN_DO_STACK_MAGIC
  currprefs.socket_emu = 0;
#endif

  if( currprefs.socket_emu ) {
    SocketBase = OpenLibrary("bsdsocket.library", 0);
    if(!SocketBase) {
    kprintf("Unable to open bsdsocket.library\n");
    BSDLOG("Unable to open bsdsocket.library\n");
    result=0;
  }
  else {
    BSDLOG("bsdsocket.library opened.\n");
    result=1;
  }
#endif

#if 0

    if( ( result = mySockStartup() ) ) {
      InitializeCriticalSection(&csSigQueueLock);

      if( hSockThread == NULL ) {

        WNDCLASS wc;    // Set up an invisible window and dummy wndproc
    
        InitializeCriticalSection( &SockThreadCS );
        hSockReq = CreateEvent( NULL, FALSE, FALSE, NULL );
        hSockReqHandled = CreateEvent( NULL, FALSE, FALSE, NULL );
        
        wc.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
        wc.lpfnWndProc = SocketWindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = 0;
//      wc.hIcon = LoadIcon (GetModuleHandle (NULL), MAKEINTRESOURCE (IDI_APPICON));
        wc.hCursor = LoadCursor (NULL, IDC_ARROW);
        wc.hbrBackground = GetStockObject (BLACK_BRUSH);
        wc.lpszMenuName = 0;
        wc.lpszClassName = "SocketFun";
        if( RegisterClass (&wc) ) {
          hSockWnd = CreateWindowEx ( 0,
            "SocketFun", "WinUAE Socket Window",
            WS_POPUP,
            0, 0,
            1, 1, 
            NULL, NULL, 0, NULL);
          hSockThread = (void *)THREAD(sock_thread,NULL);
        }
      }
    }
  }
#endif

/* DEBUG! */
  BSDLOG("return faked socket_layer_initialized=TRUE!\n");
  result = 1;

  socket_layer_initialized = result;

  return result;
}

#if 0
void deinit_socket_layer(void)
{
    int i;
    if( currprefs.socket_emu )
    {
	WSACleanup();
	if( socket_layer_initialized )
	{
	    DeleteCriticalSection( &csSigQueueLock );
	    if( hSockThread )
	    {
		DeleteCriticalSection( &SockThreadCS );
		CloseHandle( hSockReq );
		hSockReq = NULL;
		CloseHandle( hSockReqHandled );
		WaitForSingleObject( hSockThread, INFINITE );
		CloseHandle( hSockThread );
	    }
	    for (i = 0; i < MAX_SELECT_THREADS; i++)
	    {
		if (hThreads[i])
		{
		    CloseHandle( hThreads[i] );
		}
	    }
	}
    }
}
#endif

//#ifdef BSDSOCKET

void locksigqueue(void)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
    EnterCriticalSection(&csSigQueueLock);
#endif
}

void unlocksigqueue(void)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
    LeaveCriticalSection(&csSigQueueLock);
#endif
}

#if 0

// Asynchronous completion notification

// We use window messages posted to hAmigaWnd in the range from 0xb000 to 0xb000+MAXPENDINGASYNC*2
// Socket events cause even-numbered messages, task events odd-numbered messages
// Message IDs are allocated on a round-robin basis and deallocated by the main thread.

// WinSock tends to choke on WSAAsyncCancelMessage(s,w,m,0,0) called too often with an event pending

// @@@ Enabling all socket event messages for every socket by default and basing things on that would
// be cleaner (and allow us to write a select() emulation that doesn't need to be kludge-aborted).
// However, the latency of the message queue is too high for that at the moment (setting up a dummy
// window from a separate thread would fix that).

// Blocking sockets with asynchronous event notification are currently not safe to use.

struct socketbase *asyncsb[MAXPENDINGASYNC];
SOCKET asyncsock[MAXPENDINGASYNC];
uae_u32 asyncsd[MAXPENDINGASYNC];
int asyncindex;
//#endif
#endif

int host_sbinit(TrapContext *context, struct socketbase *sb) {

  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
	sb->sockAbort = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	
	if (sb->sockAbort == INVALID_SOCKET) return 0;
	if ((sb->hEvent = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL) return 0;

	sb->mtable = calloc(sb->dtablesize,sizeof(*sb->mtable));
	
	return 1;
#endif
}

void host_closesocketquick(int s)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
	BOOL true = 1;

	if( s )
	{
	    setsockopt((SOCKET)s,SOL_SOCKET,SO_DONTLINGER,(char *)&true,sizeof(true));
	    shutdown(s,1);
	    closesocket((SOCKET)s);
	}
#endif
}

void host_sbcleanup(struct socketbase *sb)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
	int i;

	for (i = 0; i < MAXPENDINGASYNC; i++) if (asyncsb[i] == sb) asyncsb[i] = NULL;

	if (sb->hEvent != NULL) CloseHandle(sb->hEvent);
	
	for (i = sb->dtablesize; i--; )
	{
		if (sb->dtable[i] != (int)INVALID_SOCKET) host_closesocketquick(sb->dtable[i]);
		
		if (sb->mtable[i]) asyncsb[(sb->mtable[i]-0xb000)/2] = NULL;
	}

        shutdown(sb->sockAbort,1);
	closesocket(sb->sockAbort);

	free(sb->mtable);
#endif
}

void host_sbreset(void)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
	memset(asyncsb,0,sizeof asyncsb);
	memset(asyncsock,0,sizeof asyncsock);
	memset(asyncsd,0,sizeof asyncsd);
	memset(threadargs,0,sizeof threadargs);
#endif
}

#if 0
void sockmsg(unsigned int msg, unsigned long wParam, unsigned long lParam)
{
	struct socketbase *sb;
	unsigned int index;
	int sdi;

	index = (msg-0xb000)/2;
	sb = asyncsb[index];
    
	if (!(msg & 1))
	{
		// is this one really for us?
		if ((SOCKET)wParam != asyncsock[index])
		{
			// cancel socket event
			WSAAsyncSelect((SOCKET)wParam,hWndSelector ? hAmigaWnd : hSockWnd,0,0);
			return;
		}

		sdi = asyncsd[index]-1;

		// asynchronous socket event?
		if (sb && !(sb->ftable[sdi] & SF_BLOCKINGINPROGRESS) && sb->mtable[sdi])
		{
			long wsbevents = WSAGETSELECTEVENT(lParam);
			int fmask = 0;

			// regular socket event?
			if (wsbevents & FD_READ) fmask = REP_READ;
			else if (wsbevents & FD_WRITE) fmask = REP_WRITE;
			else if (wsbevents & FD_OOB) fmask = REP_OOB;
			else if (wsbevents & FD_ACCEPT) fmask = REP_ACCEPT;
			else if (wsbevents & FD_CONNECT) fmask = REP_CONNECT;
			else if (wsbevents & FD_CLOSE) fmask = REP_CLOSE;

			// error?
			if (WSAGETSELECTERROR(lParam)) fmask |= REP_ERROR;

			// notify
			if (sb->ftable[sdi] & fmask) sb->ftable[sdi] |= fmask<<8;

			addtosigqueue(sb,1);
			return;
		}
	}

	locksigqueue();

	if (sb != NULL)
	{


		asyncsb[index] = NULL;

		if (WSAGETASYNCERROR(lParam))
		{
			seterrno(sb,WSAGETASYNCERROR(lParam)-WSABASEERR);
			if (sb->sb_errno >= 1001 && sb->sb_errno <= 1005) setherrno(sb,sb->sb_errno-1000);
			else if (sb->sb_errno == 55)	// ENOBUFS
				write_log("BSDSOCK: ERROR - Buffer overflow - %d bytes requested\n",WSAGETASYNCBUFLEN(lParam));
		}
		else seterrno(sb,0);


		SETSIGNAL;
	}
	
	unlocksigqueue();
}

static unsigned int allocasyncmsg(struct socketbase *sb,uae_u32 sd,SOCKET s)
{
	int i;
	locksigqueue();
	
	for (i = asyncindex+1; i != asyncindex; i++)
	{
		if (i == MAXPENDINGASYNC) i = 0;
		
		if (!asyncsb[i])
		{
			asyncsb[i] = sb;
			if (++asyncindex == MAXPENDINGASYNC) asyncindex = 0;
			unlocksigqueue();
			
			if (s == INVALID_SOCKET)
			{
				return i*2+0xb001;
			}
			else
			{
				asyncsd[i] = sd;
				asyncsock[i] = s;
				return i*2+0xb000;
			}
		}
	}
	
	unlocksigqueue();

	seterrno(sb,12); // ENOMEM
	write_log("BSDSOCK: ERROR - Async operation completion table overflow\n");
	
	return 0;
}

static void cancelasyncmsg(unsigned int wMsg)
{
	struct socketbase *sb;
	
	wMsg = (wMsg-0xb000)/2;

	sb = asyncsb[wMsg];

	if (sb != NULL)
	{
		asyncsb[wMsg] = NULL;
		CANCELSIGNAL;
	}
}
#endif

void sockabort(struct socketbase *sb)
{	
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
    locksigqueue();

    unlocksigqueue();
#endif
}

#if 0
void setWSAAsyncSelect(struct socketbase *sb, uae_u32 sd, SOCKET s, long lEvent )
	{
	if (sb->mtable[sd-1])
		{
		long wsbevents = 0;
		long eventflags;
		int i;
		locksigqueue();
	

		eventflags = sb->ftable[sd-1]  & REP_ALL;

		if (eventflags & REP_ACCEPT) wsbevents |= FD_ACCEPT;
		if (eventflags & REP_CONNECT) wsbevents |= FD_CONNECT;
		if (eventflags & REP_OOB) wsbevents |= FD_OOB;
		if (eventflags & REP_READ) wsbevents |= FD_READ;
		if (eventflags & REP_WRITE) wsbevents |= FD_WRITE;
		if (eventflags & REP_CLOSE) wsbevents |= FD_CLOSE;
		wsbevents |= lEvent;
		i = (sb->mtable[sd-1]-0xb000)/2;
		asyncsb[i] = sb;
		asyncsd[i] = sd;
		asyncsock[i] = s;
		WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,sb->mtable[sd-1],wsbevents);

		unlocksigqueue();
		}
	}

#endif

// address cleaning
static void prephostaddr(struct sockaddr_in *addr)
{
    addr->sin_family = AF_INET;
}

#if 0
static void prepamigaaddr(struct sockaddr *realpt, int len)
{
    // little endian address family value to the byte sin_family member
    ((char *)realpt)[1] = *((char *)realpt);
    
    // set size of address
    *((char *)realpt) = len;
}
#endif


int host_dup2socket(struct socketbase *sb, int fd1, int fd2)
	{

  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
    SOCKET s1,s2;

    TRACE(("dup2socket(%d,%d) -> ",fd1,fd2));
	fd1++;

    s1 = getsock(sb, fd1);
    if (s1 != INVALID_SOCKET)
		{
		if (fd2 != -1)
			{
		    if ((unsigned int) (fd2) >= (unsigned int) sb->dtablesize) 
				{
				TRACE (("Bad file descriptor (%d)\n", fd2));
				seterrno (sb, 9);	/* EBADF */
				}
			fd2++;
			s2 = getsock(sb,fd2);
			if (s2 != INVALID_SOCKET)
				{
	            shutdown(s2,1);
			    closesocket(s2);
				}
			setsd(sb,fd2,s1);
		    TRACE(("0\n"));
			return 0;
			}
		else
			{
			fd2 = getsd(sb, 1);
			setsd(sb,fd2,s1);
		    TRACE(("%d\n",fd2));
			return (fd2 - 1);
			}
		}
    TRACE(("-1\n"));
	return -1;
#endif
	}

int host_socket_real(struct socketbase *sb, int af, int type, int protocol) {
  int s;
  int sd;
  int flags;

  BSDLOG("======== %s -> %s ========\n", __FILE__,__func__);

  BSDLOG("socket(%s,%s,%d) -> \n", af == AF_INET ? "AF_INET" : "AF_other", type == SOCK_STREAM ? "SOCK_STREAM" : type == SOCK_DGRAM ? "SOCK_DGRAM " : "SOCK_RAW", protocol);

  if(!getsocketbase()) {
    return -1;
  }

  if ((s = socket(af,type,protocol)) == INVALID_SOCKET) {
    seterrno(sb, INVALID_SOCKET);
    BSDLOG("INVALID_SOCKET\n");
    return -1;
  }

  BSDLOG("s: %lx\n", s);

  sd = getsd(sb, s);
  BSDLOG("sb: %lx\n",sb);
  BSDLOG("s:  %d\n",s);
  BSDLOG("sd: %d\n",sd);

  //ioctlsocket(s, FIONBIO, &nonblocking);
  flags=fcntl(s, F_GETFL, 0);
  fcntl(s, F_SETFL, flags | O_NONBLOCK);
 
	if (type == SOCK_RAW) {

		if (protocol==IPPROTO_UDP) {
      BSDLOG("=> IPPROTO_UDP\n");
	 		//sb->ftable[sd-1] |= SF_RAW_UDP;
    }
		if (protocol==IPPROTO_ICMP) {
			struct sockaddr_in sin;

      BSDLOG("=> IPPROTO_ICMP\n");

			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = INADDR_ANY;
			bind(s,(struct sockaddr *)&sin,sizeof(sin)) ;
    }
 		if (protocol==IPPROTO_RAW) {
      BSDLOG("==> IPPROTO_RAW\n");
	 		//sb->ftable[sd-1] |= SF_RAW_RAW;
    }
  }

  BSDLOG("return: %lx\n", sd-1);
	return sd-1;
}

int host_socket(struct socketbase *sb, int af, int type, int protocol) {
  struct JUAE_bsdsocket_Message msg;

  BSDLOG("======== %s ========\n", __func__);

  sb->reply_port=CreateMsgPort();
  msg.ExecMessage.mn_Node.ln_Type=NT_MESSAGE;
  msg.ExecMessage.mn_Length = sizeof(struct JUAE_bsdsocket_Message); 
  msg.ExecMessage.mn_ReplyPort = sb->reply_port; 
  BSDLOG("sb->reply_port: %lx\n", sb->reply_port);

  msg.cmd=(ULONG) BSD_socket;
  msg.a=(ULONG) sb;
  msg.b=(ULONG) af;
  msg.c=(ULONG) type;
  msg.d=(ULONG) protocol;

  BSDLOG("PutMsg..\n");
  PutMsg(sb->aros_port, &msg);
  BSDLOG("WaitPort..\n");
  WaitPort(sb->reply_port);
  BSDLOG("done!\n");

  DeleteMsgPort(sb->reply_port); 

  return msg.ret;
}

#if 0

int host_socket(struct socketbase *sb, int af, int type, int protocol)
{
    int sd;
    SOCKET s;
    unsigned long nonblocking = 1;

    TRACE(("socket(%s,%s,%d) -> ",af == AF_INET ? "AF_INET" : "AF_other",type == SOCK_STREAM ? "SOCK_STREAM" : type == SOCK_DGRAM ? "SOCK_DGRAM " : "SOCK_RAW",protocol));

    if ((s = socket(af,type,protocol)) == INVALID_SOCKET)
    {
	SETERRNO;
	TRACE(("failed (%d)\n",sb->sb_errno));
	return -1;
    }
    else
        sd = getsd(sb,(int)s);

  	sb->ftable[sd-1] = SF_BLOCKING;
    ioctlsocket(s,FIONBIO,&nonblocking);
    TRACE(("%d\n",sd));

	if (type == SOCK_RAW)
		{
		if (protocol==IPPROTO_UDP)
			{
	 		sb->ftable[sd-1] |= SF_RAW_UDP;
			}
		if (protocol==IPPROTO_ICMP)
			{
			struct sockaddr_in sin;

			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = INADDR_ANY;
			bind(s,(struct sockaddr *)&sin,sizeof(sin)) ;
			}
 		if (protocol==IPPROTO_RAW)
			{
	 		sb->ftable[sd-1] |= SF_RAW_RAW;
			}
		}
	return sd-1;
}
#endif

uae_u32 host_bind(struct socketbase *sb, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
    char buf[MAXADDRLEN];
    uae_u32 success = 0;
    SOCKET s;

	sd++;
    TRACE(("bind(%d,0x%lx,%d) -> ",sd,name,namelen));
    s = getsock(sb, sd);

    if (s != INVALID_SOCKET)
    {
	if (namelen <= sizeof buf)
	{
	    memcpy(buf,get_real_address(name),namelen);
	    
	    // some Amiga programs set this field to bogus values
	    prephostaddr((SOCKADDR_IN *)buf);

	    if ((success = bind(s,(struct sockaddr *)buf,namelen)) != 0)
	    {
		SETERRNO;
		TRACE(("failed (%d)\n",sb->sb_errno));
	    }
	    else
                TRACE(("OK\n"));
	}
	else
            write_log("BSDSOCK: ERROR - Excessive namelen (%d) in bind()!\n",namelen);
    }

    return success;
#endif
}

uae_u32 host_listen(struct socketbase *sb, uae_u32 sd, uae_u32 backlog)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
    SOCKET s;
    uae_u32 success = -1;
	
	sd++;
    TRACE(("listen(%d,%d) -> ",sd,backlog));
    s = getsock(sb, sd);

    if (s != INVALID_SOCKET)
    {
	if ((success = listen(s,backlog)) != 0)
	{
	    SETERRNO;
	    TRACE(("failed (%d)\n",sb->sb_errno));
	}
	else 
            TRACE(("OK\n"));
    }
		    
    return success;
#endif
}

void host_accept(TrapContext *context, struct socketbase *sb, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
    struct sockaddr *rp_name,*rp_nameuae;
	struct sockaddr sockaddr;
    int hlen,hlenuae=0;
    SOCKET s, s2;
    int success = 0;
    unsigned int wMsg;
    
    sd++;
	if (name != 0 )
		{
		rp_nameuae = rp_name = (struct sockaddr *)get_real_address(name);
	    hlenuae = hlen = get_long(namelen);
		if (hlenuae < sizeof(sockaddr))
			{ // Fix for CNET BBS Windows must have 16 Bytes (sizeof(sockaddr)) otherwise Error WSAEFAULT
			rp_name = &sockaddr; 
			hlen = sizeof(sockaddr);
			}
		}
	else
		{
		rp_name = &sockaddr; 
		hlen = sizeof(sockaddr);
		}
    TRACE(("accept(%d,%d,%d) -> ",sd,name,hlenuae));

    s = (SOCKET)getsock(sb,(int)sd);
    
    if (s != INVALID_SOCKET)
    {
	BEGINBLOCKING;
	
	s2 = accept(s,rp_name,&hlen);

	if (s2 == INVALID_SOCKET)
	{
	    SETERRNO;

	    if (sb->ftable[sd-1] & SF_BLOCKING && sb->sb_errno == WSAEWOULDBLOCK-WSABASEERR)
	    {
		if (sb->mtable[sd-1] || (wMsg = allocasyncmsg(sb,sd,s)) != 0)
		{
			if (sb->mtable[sd-1] == 0)
				{
			    WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,wMsg,FD_ACCEPT);
				}
			else
				{
				setWSAAsyncSelect(sb,sd,s,FD_ACCEPT);
				}

		    WAITSIGNAL;

			if (sb->mtable[sd-1] == 0)
				{
				cancelasyncmsg(wMsg);
				}
			else
				{
				setWSAAsyncSelect(sb,sd,s,0);
				}
		    
		    if (sb->eintr)
		    {
			TRACE(("[interrupted]\n"));
			ENDBLOCKING;
			return;
		    }

		    s2 = accept(s,rp_name,&hlen);

		    if (s2 == INVALID_SOCKET)
		    {
			SETERRNO;

			if (sb->sb_errno == WSAEWOULDBLOCK-WSABASEERR) write_log("BSDSOCK: ERRRO - accept() would block despite FD_ACCEPT message\n");
		    }
		}
	    }
	}
			
	if (s2 == INVALID_SOCKET)
	{
	    sb->resultval = -1;
	    TRACE(("failed (%d)\n",sb->sb_errno));
	}
	else
	{
	    sb->resultval = getsd(sb, s2);
	    sb->ftable[sb->resultval-1] = sb->ftable[sd-1];	// new socket inherits the old socket's properties
		sb->resultval--;
	    if (rp_name != 0)
			{ // 1.11.2002 XXX
			if (hlen <= hlenuae)
				{ // Fix for CNET BBS Part 2
				prepamigaaddr(rp_name,hlen);
				if (namelen != 0)
					{
					put_long(namelen,hlen);
					}
				}
			else
				{ // Copy only the number of bytes requested
				if (hlenuae != 0)
					{	
					prepamigaaddr(rp_name,hlenuae);
					memcpy(rp_nameuae,rp_name,hlenuae);
					put_long(namelen,hlenuae);
					}
				}
			}
	    TRACE(("%d/%d\n",sb->resultval,hlen));
	}

	ENDBLOCKING;
    }

#endif
	}

#if 0
typedef enum
{
    connect_req,
    recvfrom_req,
    sendto_req,
    abort_req,
    last_req
} threadsock_e;

struct threadsock_packet
{
    threadsock_e packet_type;
    union
    {
        struct sendto_params
        {
            char *buf;
            char *realpt;
            uae_u32 sd;
            uae_u32 msg;
            uae_u32 len;
            uae_u32 flags;
            uae_u32 to;
            uae_u32 tolen;
        } sendto_s;
        struct recvfrom_params
        {
            char *realpt;
            uae_u32 addr;
            uae_u32 len;
            uae_u32 flags;
            struct sockaddr *rp_addr;
            int *hlen;
        } recvfrom_s;
        struct connect_params
        {
            char *buf;
            uae_u32 namelen;
        } connect_s;
        struct abort_params
        {
            SOCKET *newsock;
        } abort_s;
    } params;
    SOCKET s;
    struct socketbase *sb;
} sockreq;

BOOL HandleStuff( void )
{
    BOOL quit = FALSE;
    struct socketbase *sb = NULL;
    BOOL handled = TRUE;
    if( hSockReq )
    {
	// 100ms sleepiness might need some tuning...
	//if(WaitForSingleObject( hSockReq, 100 ) == WAIT_OBJECT_0 )
	{
	    switch( sockreq.packet_type )
	    {
		case connect_req:
    		    sockreq.sb->resultval = connect(sockreq.s,(struct sockaddr *)(sockreq.params.connect_s.buf),sockreq.params.connect_s.namelen);
		break;
		case sendto_req:
		    if( sockreq.params.sendto_s.to )
		    {
			sockreq.sb->resultval = sendto(sockreq.s,sockreq.params.sendto_s.realpt,sockreq.params.sendto_s.len,sockreq.params.sendto_s.flags,(struct sockaddr *)(sockreq.params.sendto_s.buf),sockreq.params.sendto_s.tolen);
		    }
		    else
		    {
			sockreq.sb->resultval = send(sockreq.s,sockreq.params.sendto_s.realpt,sockreq.params.sendto_s.len,sockreq.params.sendto_s.flags);
		    }
		break;
		case recvfrom_req:
		    if( sockreq.params.recvfrom_s.addr )
		    {
			sockreq.sb->resultval = recvfrom( sockreq.s, sockreq.params.recvfrom_s.realpt, sockreq.params.recvfrom_s.len,
							  sockreq.params.recvfrom_s.flags, sockreq.params.recvfrom_s.rp_addr,
							  sockreq.params.recvfrom_s.hlen );

		    }
		    else
		    {
			sockreq.sb->resultval = recv( sockreq.s, sockreq.params.recvfrom_s.realpt, sockreq.params.recvfrom_s.len,
						      sockreq.params.recvfrom_s.flags );
		    }
		break;
		case abort_req:
		    *(sockreq.params.abort_s.newsock) = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
			if (*(sockreq.params.abort_s.newsock) != sb->sockAbort)
				{
				shutdown( sb->sockAbort, 1 );
				closesocket(sb->sockAbort);
				}
		    handled = FALSE; /* Don't bother the SETERRNO section after the switch() */
		break;
		case last_req:
		default:
		    write_log( "BSDSOCK: Invalid sock-thread request!\n" );
		    handled = FALSE;
		break;
	    }
	    if( handled )
	    {
		if( sockreq.sb->resultval == SOCKET_ERROR )
		{
		    sb = sockreq.sb;
			
		    SETERRNO;
		}
	    }
	    SetEvent( hSockReqHandled );
	}
    }
    else
    {
	quit = TRUE;
    }
    return quit;
}

static long FAR PASCAL SocketWindowProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if( message >= 0xB000 && message < 0xB000+MAXPENDINGASYNC*2 )
    {
#if DEBUG_SOCKETS
        write_log( "sockmsg(0x%x, 0x%x, 0x%x)\n", message, wParam, lParam );
#endif
        sockmsg( message, wParam, lParam );
        return 0;
    }
    return DefWindowProc( hwnd, message, wParam, lParam );
}



static unsigned int __stdcall sock_thread(void *blah)
{
    unsigned int result = 0;
	HANDLE WaitHandle;
    MSG msg;

	if( hSockWnd )
	{
	    // Make sure we're outrunning the wolves
	    int pri = THREAD_PRIORITY_ABOVE_NORMAL;
#if 0	   
	    if (!os_winnt) {
		pri = priorities[currprefs.win32_active_priority].value;
		if (pri == THREAD_PRIORITY_HIGHEST)
	    	    pri = THREAD_PRIORITY_TIME_CRITICAL;
		else
		    pri++;
	    }
#endif	   
	    SetThreadPriority( GetCurrentThread(), pri );
	    
	    while( TRUE )
	    {
	    if( hSockReq )
			{
			DWORD wait;
			WaitHandle = hSockReq;
			wait = MsgWaitForMultipleObjects (1, &WaitHandle, FALSE,INFINITE, QS_POSTMESSAGE);
			if (wait == WAIT_OBJECT_0)
				{
				if( HandleStuff() ) // See if its time to quit...
					break;
				}
			if (wait == WAIT_OBJECT_0 +1)
				{
				Sleep(10);
				while( PeekMessage( &msg, NULL, WM_USER, 0xB000+MAXPENDINGASYNC*2, PM_REMOVE ) > 0 )
					{
					TranslateMessage( &msg );
					DispatchMessage( &msg );
					}
				}
			}
	    }
	}
    write_log( "BSDSOCK: We have exited our sock_thread()\n" );
#ifndef __GNUC__
    _endthreadex( result );
#endif
    return result;
}
#endif


void host_connect_real(TrapContext *context, struct socketbase *sb, uae_u32 sd, uae_u32 addr68k, uae_u32 addrlen) {
  struct Socket *s;
  struct sockaddr_in sin;
  int *addr;
  unsigned int reorder;


  BSDLOG("======== %s -> %s ========\n", __FILE__,__func__);
  if(!getsocketbase()) {
		seterrno(sb,1); /*???*/
    return;
  }

  BSDLOG("addr68k: %lx\n", addr68k);
  BSDLOG("namelen: %d\n",  addrlen);

	sd++;
  s = getsock(sb,(int)sd);

  if (s == INVALID_SOCKET) {
    BSDLOG("s is NULL!\n");
    /* TODO: error handling !? */
    return;
  }

	if (addrlen > MAXADDRLEN) {
    write_log("BSDSOCK: WARNING - Excessive addrlen (%d) in connect()!\n",addrlen);
    BSDLOG("BSDSOCK: WARNING - Excessive addrlen (%d) in connect()!\n",addrlen);
    return;
  }

  addr=get_real_address(addr68k);

  sin.sin_len   =sizeof(struct sockaddr);
  sin.sin_family=AF_INET;
  BSDLOG("port:         %d\n", get_word(addr68k+2));
  sin.sin_port  =htons(get_word(addr68k+2)); /* byte??*/
  BSDLOG("sin.sin_port: %d\n", sin.sin_port);
  sin.sin_addr.s_addr =  addr[1]; /* endianess ?? */
#if 0
  BSDLOG("addr: %lx\n", sin.sin_addr.s_addr);
  sin.sin_addr.s_addr =  htonl(addr[1]);
  BSDLOG("addr: %lx\n", sin.sin_addr.s_addr);
  reorder=addr[1];
  reorder = (reorder >> 24) | ((reorder<<8) & 0x00FF0000) | ((reorder>>8) & 0x0000FF00) | (reorder << 24);
  BSDLOG("reorder: %lx\n", reorder);
#endif
  BSDLOG("port0: %lx\n", addr[0]);
  BSDLOG("port1: %lx\n", addr[1]);
  BSDLOG("port2: %lx\n", addr[2]);


  BSDLOG("connecting..\n");

  sb->resultval=connect(s, (struct sockaddr *) &sin, sin.sin_len);

  if(sb->resultval != 0) {
    sb->sb_errno=errno; /* TODO! */
    BSDLOG("connect error: %ld\n", sb->sb_errno);
    return;
  }


  BSDLOG("connected!\n");
#if 0

    SOCKET s;
    int success = 0;
    unsigned int wMsg;
    char buf[MAXADDRLEN];


	sd++;
    TRACE(("connect(%d,0x%lx,%d) -> ",sd,name,namelen));

    s = (SOCKET)getsock(sb,(int)sd);
    
    if (s != INVALID_SOCKET)
    {
	if (namelen <= MAXADDRLEN)
	{
	    if (sb->mtable[sd-1] || (wMsg = allocasyncmsg(sb,sd,s)) != 0)
	    {
		if (sb->mtable[sd-1] == 0)
			{
			WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,wMsg,FD_CONNECT);
			}
		else
			{
			setWSAAsyncSelect(sb,sd,s,FD_CONNECT);
			}

	
		BEGINBLOCKING;
                PREPARE_THREAD;

		memcpy(buf,get_real_address(name),namelen);
		prephostaddr((SOCKADDR_IN *)buf);
		
                sockreq.packet_type = connect_req;
                sockreq.s = s;
                sockreq.sb = sb;
                sockreq.params.connect_s.buf = buf;
                sockreq.params.connect_s.namelen = namelen;

                TRIGGER_THREAD;


		if (sb->resultval)
		{
		    if (sb->sb_errno == WSAEWOULDBLOCK-WSABASEERR)
		    {
			if (sb->ftable[sd-1] & SF_BLOCKING)
			{
			    seterrno(sb,0);
			
	
				WAITSIGNAL;

			    if (sb->eintr)
			    {
				// Destroy socket to cancel abort, replace it with fake socket to enable proper closing.
				// This is in accordance with BSD behaviour.
                                shutdown(s,1);
				closesocket(s);
				sb->dtable[sd-1] = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
			    }
			}
		        else
					{
                    seterrno(sb,36);	// EINPROGRESS

					}
		    }
		else
			{
			CANCELSIGNAL; // Cancel pending signal

			}
			}

			ENDBLOCKING;
			if (sb->mtable[sd-1] == 0)
				{
				cancelasyncmsg(wMsg);
				}
			else
				{
				setWSAAsyncSelect(sb,sd,s,0);
				}
			}

        }
        else
            write_log("BSDSOCK: WARNING - Excessive namelen (%d) in connect()!\n",namelen);
    }
    TRACE(("%d\n",sb->sb_errno));
#endif
}

void host_connect(TrapContext *context, struct socketbase *sb, uae_u32 sd, uae_u32 addr68k, uae_u32 addrlen) {

  struct JUAE_bsdsocket_Message msg;

  BSDLOG("======== %s ========\n",__func__);

  sb->reply_port=CreateMsgPort();
  msg.ExecMessage.mn_Node.ln_Type=NT_MESSAGE;
  msg.ExecMessage.mn_Length = sizeof(struct JUAE_bsdsocket_Message); 
  msg.ExecMessage.mn_ReplyPort = sb->reply_port; 
  BSDLOG("sb->reply_port: %lx\n", sb->reply_port);

  msg.cmd=(ULONG) BSD_connect;
  msg.a=(ULONG) context;
  msg.b=(ULONG) sb;
  msg.c=(ULONG) sd;
  msg.d=(ULONG) addr68k;
  msg.e=(ULONG) addrlen;

  BSDLOG("PutMsg..\n");
  PutMsg(sb->aros_port, &msg);
  BSDLOG("WaitPort..\n");
  WaitPort(sb->reply_port);
  BSDLOG("done!\n");
  DeleteMsgPort(sb->reply_port);
}



void host_sendto_real(TrapContext *context, struct socketbase *sb, uae_u32 sd, uae_u32 msg, uae_u32 len, uae_u32 flags, uae_u32 to, uae_u32 tolen) {
  int s; 
  void *realmsg;
  char buf[MAXADDRLEN];

  BSDLOG("======== %s -> %s ========\n", __FILE__,__func__);

  if(!getsocketbase()) {
    seterrno(sb, 1);
    return; /* TODO!*/
  }

  if (to)
    BSDLOG("sendto(%d,msg 0x%lx, len %d, flags 0x%lx, to 0x%lx, tolen %d)\n",sd,msg,len,flags,to,tolen);
  else
    BSDLOG("send(%d,msg 0x%lx, len %d, flags %d)\n",sd,msg,len,flags);

	sd++;
  s = getsock(sb,sd);

  BSDLOG("s: %lx\n", s);

  if(s==INVALID_SOCKET) {
    BSDLOG("s==INVALID_SOCKET!!\n");
    sb->resultval=INVALID_SOCKET;
    return;
  }

  realmsg = get_real_address(msg);
  BSDLOG("realmsg: %lx\n", realmsg);
  if (to) {
    if (tolen > sizeof buf) {
      write_log("BSDSOCK: WARNING - Target address in sendto() too large (%d)!\n",tolen);
      BSDLOG("WARNING - Target address in sendto() too large (%d)!\n",tolen);
    }
    else {
      memcpy(buf,get_real_address(to),tolen);
      // some Amiga software sets this field to bogus values
      prephostaddr((struct sockaddr_in *)buf);
    }
  }


  BSDLOG("calling sendto..\n");
  sb->resultval=sendto(s, realmsg, len, flags, buf, tolen);

  if(sb->resultval == -1) {
    BSDLOG("SOCKET_ERROR!\n");
  }
  else {
    BSDLOG("sendto ok!\n");
  }

  BSDLOG("left: characters sent: %d\n", sb->resultval);

  return;

#if 0


//	if (sb->ftable[sd-1]&SF_RAW_RAW)
//		{

  if (*(realpt+9) == 0x1) { // ICMP
    struct sockaddr_in sin;
    BSDLOG("RAW 0x1\n");
    shutdown(s,1);
    //closesocket(s);
    s = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = (unsigned short) (*(realpt+21)&0xff)*256 + (unsigned short) (*(realpt+20)&0xff);
    bind(s,(struct sockaddr *)&sin,sizeof(sin)) ;

    sb->dtable[sd-1] = s;
    //sb->ftable[sd-1]&= ~SF_RAW_RAW;
    //sb->ftable[sd-1]|= SF_RAW_RICMP;
    }

  if (*(realpt+9) == 0x11) { // UDP
    struct sockaddr_in sin;
    BSDLOG("RAW 0x11\n");
    shutdown(s,1);
    //closesocket(s);
    s = socket(AF_INET,SOCK_RAW,IPPROTO_UDP);

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = (unsigned short) (*(realpt+21)&0xff)*256 + (unsigned short) (*(realpt+20)&0xff);
    bind(s,(struct sockaddr *)&sin,sizeof(sin)) ;

    sb->dtable[sd-1] = s;
    //sb->ftable[sd-1]&= ~SF_RAW_RAW;
    //sb->ftable[sd-1]|= SF_RAW_RUDP;
    }
//		}
#endif
	
#if 0
    SOCKET s;
    char *realpt;
    unsigned int wMsg;
    char buf[MAXADDRLEN];
	int iCut;

#ifdef TRACING_ENABLED
    if (to)
	TRACE(("sendto(%d,0x%lx,%d,0x%lx,0x%lx,%d) -> ",sd,msg,len,flags,to,tolen));
    else
	TRACE(("send(%d,0x%lx,%d,%d) -> ",sd,msg,len,flags));
#endif
	sd++;
    s = getsock(sb,sd);

    if (s != INVALID_SOCKET)
    {
        realpt = get_real_address(msg);
		
	if (to)
	{
	    if (tolen > sizeof buf) write_log("BSDSOCK: WARNING - Target address in sendto() too large (%d)!\n",tolen);
	    else
	    {
		memcpy(buf,get_real_address(to),tolen);
		// some Amiga software sets this field to bogus values
		prephostaddr((SOCKADDR_IN *)buf);
	    }
	}
	if (sb->ftable[sd-1]&SF_RAW_RAW)
		{
		if (*(realpt+9) == 0x1)
			{ // ICMP
			struct sockaddr_in sin;
			shutdown(s,1);
			closesocket(s);
			s = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);

			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = INADDR_ANY;
			sin.sin_port = (unsigned short) (*(realpt+21)&0xff)*256 + (unsigned short) (*(realpt+20)&0xff);
			bind(s,(struct sockaddr *)&sin,sizeof(sin)) ;

			sb->dtable[sd-1] = s;
			sb->ftable[sd-1]&= ~SF_RAW_RAW;
			sb->ftable[sd-1]|= SF_RAW_RICMP;
			}
		if (*(realpt+9) == 0x11)
			{ // UDP
			struct sockaddr_in sin;
			shutdown(s,1);
			closesocket(s);
			s = socket(AF_INET,SOCK_RAW,IPPROTO_UDP);

			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = INADDR_ANY;
			sin.sin_port = (unsigned short) (*(realpt+21)&0xff)*256 + (unsigned short) (*(realpt+20)&0xff);
			bind(s,(struct sockaddr *)&sin,sizeof(sin)) ;

			sb->dtable[sd-1] = s;
			sb->ftable[sd-1]&= ~SF_RAW_RAW;
			sb->ftable[sd-1]|= SF_RAW_RUDP;
			}
		}
		
	BEGINBLOCKING;

	for (;;)
	{
            PREPARE_THREAD;

            sockreq.packet_type = sendto_req;
            sockreq.s = s;
            sockreq.sb = sb;
            sockreq.params.sendto_s.realpt = realpt;
            sockreq.params.sendto_s.buf = buf;
            sockreq.params.sendto_s.sd = sd;
            sockreq.params.sendto_s.msg = msg;
            sockreq.params.sendto_s.len = len;
            sockreq.params.sendto_s.flags = flags;
            sockreq.params.sendto_s.to = to;
            sockreq.params.sendto_s.tolen = tolen;

			if (sb->ftable[sd-1]&SF_RAW_UDP)
				{
				*(buf+2) = *(realpt+2);
 				*(buf+3) = *(realpt+3);
				// Copy DST-Port
				iCut = 8;
				sockreq.params.sendto_s.realpt += iCut;
				sockreq.params.sendto_s.len -= iCut;
				}
			if (sb->ftable[sd-1]&SF_RAW_RUDP)
				{
				int iTTL;
				iTTL = (int) *(realpt+8)&0xff;
				setsockopt(s,IPPROTO_IP,4,(char*) &iTTL,sizeof(iTTL));
				*(buf+2) = *(realpt+22);
 				*(buf+3) = *(realpt+23);
				// Copy DST-Port
				iCut = 28;
				sockreq.params.sendto_s.realpt += iCut;
				sockreq.params.sendto_s.len -= iCut;
				}
			if (sb->ftable[sd-1]&SF_RAW_RICMP)
				{
				int iTTL;
				iTTL = (int) *(realpt+8)&0xff;
				setsockopt(s,IPPROTO_IP,4,(char*) &iTTL,sizeof(iTTL));
				iCut = 20;
				sockreq.params.sendto_s.realpt += iCut;
				sockreq.params.sendto_s.len -= iCut;
				}

			

            TRIGGER_THREAD;
		if (sb->ftable[sd-1]&SF_RAW_UDP||sb->ftable[sd-1]&SF_RAW_RUDP||sb->ftable[sd-1]&SF_RAW_RICMP)
			{
			sb->resultval += iCut;
			}
	    if (sb->resultval == -1)
	    {
		if (sb->sb_errno != WSAEWOULDBLOCK-WSABASEERR || !(sb->ftable[sd-1] & SF_BLOCKING)) break;
	    }
	    else
	    {
		realpt += sb->resultval;
		len -= sb->resultval;
		
		if (len <= 0) break;
		else continue;
	    }

	    if (sb->mtable[sd-1] || (wMsg = allocasyncmsg(sb,sd,s)) != 0)
	    {
		if (sb->mtable[sd-1] == 0)
			{
			WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,wMsg,FD_WRITE);
			}
		else
			{
			setWSAAsyncSelect(sb,sd,s,FD_WRITE);
			}
			
		WAITSIGNAL;
		
		if (sb->mtable[sd-1] == 0)
			{
			cancelasyncmsg(wMsg);
			}
		else
			{
			setWSAAsyncSelect(sb,sd,s,0);
			}
		
		if (sb->eintr)
		{
		    TRACE(("[interrupted]\n"));
		    return;
		}
	    }
	    else break;
	}

	ENDBLOCKING;
    }
    else sb->resultval = -1;

#ifdef TRACING_ENABLED
    if (sb->resultval == -1)
	TRACE(("failed (%d)\n",sb->sb_errno));
    else
	TRACE(("%d\n",sb->resultval));
#endif

#endif
}

void host_sendto(TrapContext *context, struct socketbase *sb, uae_u32 sd, uae_u32 msg_in, uae_u32 len, uae_u32 flags, uae_u32 to, uae_u32 tolen) {

  struct JUAE_bsdsocket_Message msg;

  BSDLOG("======== %s ========\n",__func__);

  sb->reply_port=CreateMsgPort();
  msg.ExecMessage.mn_Node.ln_Type=NT_MESSAGE;
  msg.ExecMessage.mn_Length = sizeof(struct JUAE_bsdsocket_Message); 
  msg.ExecMessage.mn_ReplyPort = sb->reply_port; 
  BSDLOG("sb->reply_port: %lx\n", sb->reply_port);

  msg.cmd=(ULONG) BSD_sendto;
  msg.a=(ULONG) context;
  msg.b=(ULONG) sb;
  msg.c=(ULONG) sd;
  msg.d=(ULONG) msg_in;
  msg.e=(ULONG) len;
  msg.f=(ULONG) flags;
  msg.g=(ULONG) to;
  msg.h=(ULONG) tolen;

  BSDLOG("PutMsg..\n");
  PutMsg(sb->aros_port, &msg);
  BSDLOG("WaitPort..\n");
  WaitPort(sb->reply_port);
  BSDLOG("done!\n");
  DeleteMsgPort(sb->reply_port);
}



void host_recvfrom_real(TrapContext *context, struct socketbase *sb, uae_u32 sd, uae_u32 msg, uae_u32 len, uae_u32 flags, uae_u32 addr, uae_u32 addrlen) {
  int s;
  char *realmsg;
  struct sockaddr *rp_addr = NULL;
  int hlen;
  int res;
  int i;
  int *addr_got;

  struct sockaddr_in sin;

  BSDLOG("======== %s -> %s ========\n", __FILE__,__func__);

  if(!getsocketbase()) {
    seterrno(sb, 1);
    return; /* TODO!*/
  }

  if (addr)
    BSDLOG("recvfrom(%d,0x%lx,%d,0x%lx,0x%lx,%d) ->\n",sd,msg,len,flags,addr,get_long(addrlen));
  else
    BSDLOG("recv(%d,0x%lx,%d,0x%lx) ->\n",sd,msg,len,flags);

  sd++;
  s = getsock(sb,sd);

  BSDLOG("s: %lx\n", s);

  if (s == INVALID_SOCKET) {
    BSDLOG("INVALID_SOCKET!\n");
    sb->sb_errno=-1; /* TODO */
    return;
  }

  if(addr) {
    addr_got=get_real_address(addr);
    BSDLOG("addr0: %lx\n", addr_got[0]);
    BSDLOG("addr1: %lx\n", addr_got[1]);
    BSDLOG("addr2: %lx\n", addr_got[2]);
    BSDLOG("addr3: %lx\n", addr_got[3]);


    sin.sin_len   =sizeof(struct sockaddr);
    sin.sin_family=AF_INET;
    sin.sin_port  =htons(get_word(addr+2)); /* byte??*/
    BSDLOG("port: %d\n", sin.sin_port);
    sin.sin_addr.s_addr =  addr_got[1]; /* endianess ?? */
    BSDLOG("addr: %lx\n", sin.sin_addr.s_addr);
  }

#if 0
  sin.sin_family=AF_INET;
  sin.sin_port  =htons(get_word(addr68k+2)); /* byte??*/
  BSDLOG("port: %d\n", sin.sin_port);
  sin.sin_addr.s_addr =  addr[1]; /* endianess ?? */
#
#endif
  realmsg = get_real_address(msg);

#if 0
    if (addr) {
      hlen = get_long(addrlen);
      rp_addr = (struct sockaddr *)get_real_address(addr);
    }

    BSDLOG("rp_addr->sa_family: %d\n", rp_addr->sa_family);
    for(i=0;i<14;i++) {
      BSDLOG("rp_addr->sa_data[%d]: %lx\n", i, rp_addr->sa_data[i]);
    }
#endif

  if(addr) {
    //res=recvfrom(s, realmsg, len, flags, &sin, sizeof(struct sockaddr_in));
    BSDLOG("recvfrom with addr: NOT YET IMPLEMENTED!\n");
  }
  else {
    /* same as recv */
    res=recvfrom(s, realmsg, len, flags, NULL, 0);
  }

  BSDLOG("byts received with recvfrom: %d\n", res);

  sb->resultval = res;
#if 0
	if (addr) {
	    prepamigaaddr(realmsg, hlen);
	    put_long(addrlen, res);
  }
    }
    else sb->resultval = -1;
#endif




#if 0
    SOCKET s;
    char *realpt;
    struct sockaddr *rp_addr = NULL;
    int hlen;
    unsigned int wMsg;

#ifdef TRACING_ENABLED
    if (addr)
	TRACE(("recvfrom(%d,0x%lx,%d,0x%lx,0x%lx,%d) -> ",sd,msg,len,flags,addr,get_long(addrlen)));
    else
	TRACE(("recv(%d,0x%lx,%d,0x%lx) -> ",sd,msg,len,flags));
#endif
    sd++;
    s = getsock(sb,sd);

    if (s != INVALID_SOCKET)
    {
	realpt = get_real_address(msg);

	if (addr)
	{
	    hlen = get_long(addrlen);
	    rp_addr = (struct sockaddr *)get_real_address(addr);
	}

	BEGINBLOCKING;

	for (;;)
	{
            PREPARE_THREAD;

            sockreq.packet_type = recvfrom_req;
            sockreq.s = s;
            sockreq.sb = sb;
            sockreq.params.recvfrom_s.addr = addr;
            sockreq.params.recvfrom_s.flags = flags;
            sockreq.params.recvfrom_s.hlen = &hlen;
            sockreq.params.recvfrom_s.len = len;
            sockreq.params.recvfrom_s.realpt = realpt;
            sockreq.params.recvfrom_s.rp_addr = rp_addr;

            TRIGGER_THREAD;
	    if (sb->resultval == -1)
	    {
		if (sb->sb_errno == WSAEWOULDBLOCK-WSABASEERR && sb->ftable[sd-1] & SF_BLOCKING)
		{
		    if (sb->mtable[sd-1] || (wMsg = allocasyncmsg(sb,sd,s)) != 0)
		    {
			if (sb->mtable[sd-1] == 0)
			{
			    WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,wMsg,FD_READ|FD_CLOSE);
			}
			else
			{
			    setWSAAsyncSelect(sb,sd,s,FD_READ|FD_CLOSE);
			}

			WAITSIGNAL;
		
			if (sb->mtable[sd-1] == 0)
			{
			    cancelasyncmsg(wMsg);
			}
			else
			{
			    setWSAAsyncSelect(sb,sd,s,0);
			}

		
			if (sb->eintr)
			{
			    TRACE(("[interrupted]\n"));
			    return;
			}
		    }
		    else break;
		}
		else break;
	    }
	    else break;
	}
	
	ENDBLOCKING;

	if (addr)
	{
	    prepamigaaddr(rp_addr,hlen);
	    put_long(addrlen,hlen);
	}
    }
    else sb->resultval = -1;

#ifdef TRACING_ENABLED
    if (sb->resultval == -1)
	TRACE(("failed (%d)\n",sb->sb_errno));
    else
	TRACE(("%d\n",sb->resultval));
#endif

#endif
}

void host_recvfrom(TrapContext *context, struct socketbase *sb, uae_u32 sd, uae_u32 msg_in, uae_u32 len, uae_u32 flags, uae_u32 addr, uae_u32 addrlen) {

  struct JUAE_bsdsocket_Message msg;

  BSDLOG("======== %s ========\n",__func__);

  sb->reply_port=CreateMsgPort();
  msg.ExecMessage.mn_Node.ln_Type=NT_MESSAGE;
  msg.ExecMessage.mn_Length = sizeof(struct JUAE_bsdsocket_Message); 
  msg.ExecMessage.mn_ReplyPort = sb->reply_port; 
  BSDLOG("sb->reply_port: %lx\n", sb->reply_port);

  msg.cmd=(ULONG) BSD_recvfrom;
  msg.a=(ULONG) context;
  msg.b=(ULONG) sb;
  msg.c=(ULONG) sd;
  msg.d=(ULONG) msg_in;
  msg.e=(ULONG) len;
  msg.f=(ULONG) flags;
  msg.g=(ULONG) addr;
  msg.h=(ULONG) addrlen;

  BSDLOG("PutMsg..\n");
  PutMsg(sb->aros_port, &msg);
  BSDLOG("WaitPort..\n");
  WaitPort(sb->reply_port);
  BSDLOG("done!\n");
  DeleteMsgPort(sb->reply_port);
}



uae_u32 host_shutdown(struct socketbase *sb, uae_u32 sd, uae_u32 how)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
    SOCKET s;
    
    TRACE(("shutdown(%d,%d) -> ",sd,how));
    sd++;
    s = getsock(sb,sd);

    if (s != INVALID_SOCKET)
    {
	if (shutdown(s,how))
	{
	    SETERRNO;
	    TRACE(("failed (%d)\n",sb->sb_errno));
	}
	else
	{
	    TRACE(("OK\n"));
	    return 0;
	}
    }
	
    return -1;
#endif
}

void host_setsockopt_real(struct socketbase *sb, uae_u32 sd, uae_u32 level, uae_u32 optname, uae_u32 optval, uae_u32 len) {

  struct Socket *s;
  char buf[MAXADDRLEN];
  BSDLOG("======== %s -> %s ========\n", __FILE__,__func__);

  BSDLOG("host_setsockopt(0x%lx,%d,%lx,0x%lx,0x%lx,%d)\n", sb, sd, level,optname,optval,len);
  if(level != SOL_SOCKET) {
    BSDLOG("Warning: strange level %lx != SOL_SOCKET !?\n", level);
  }
  else {
    BSDLOG("level: %lx (SOL_SOCKET)\n", level);
  }

  if(!getsocketbase()) {
    seterrno(sb, 1);
    return; /* TODO!*/
  }

	sd++;
  s = getsock(sb,sd);

  BSDLOG("s: %lx\n", s);

  if (s == INVALID_SOCKET) {
    BSDLOG("INVALID_SOCKET (EBADF returned)!\n");
    seterrno(sb, EBADF); /* EBADF => The argument sockfd is not a valid descriptor. */
    return;
  }

	if (len > sizeof buf) {
	    write_log("BSDSOCK: WARNING - Excessive optlen in setsockopt() (%d)\n",len);
	    BSDLOG("BSDSOCK: WARNING - Excessive optlen in setsockopt() (%d)\n",len);
	    len = sizeof buf;
	}	
	
	if (level == IPPROTO_IP && optname == IP_HDRINCL) {
    /* The IPv4 layer generates an IP header when sending a packet unless 
     * the IP_HDRINCL socket option is enabled on the socket. 
     * When it is enabled, the packet must contain an IP header. 
     * For receiving the IP header is always included in the packet. 
     */
    BSDLOG("Warning: IP_HDRINCL might NOT be implemented!\n");
  }

	if (level == SOL_SOCKET && optname == SO_LINGER) {
    BSDLOG("Warning: SOL_SOCKET && SO_LINGER is TODO!\n");
	    //((LINGER *)buf)->l_onoff = get_long(optval);
	    //((LINGER *)buf)->l_linger = get_long(optval+4);
	}
	else {
    if (len == 4) {
      *(long *)buf = get_long(optval);
    }
    else {
      if (len == 2) {
        *(short *)buf = get_word(optval);
      }
      else {
        write_log("BSDSOCK: ERROR - Unknown optlen (%d) in setsockopt(%d,%d)\n",level,optname);
        BSDLOG("BSDSOCK: ERROR - Unknown optlen (%d) in setsockopt(%d,%d)\n",level,optname);
      }
    }
	}

  /* handle SO_EVENTMASK */
	if (level == 0xffff && optname == 0x2001) {
    BSDLOG("Warning: SO_EVENTMASK is TODO!\n");
  }
 
  BSDLOG("call setsockopt(s %d, level %lx, optname %lx, buf 0x%lx, len %d)\n", s, level, optname, buf,len);
	sb->resultval = setsockopt(s, level, optname, buf, len);

  if(sb->resultval) {
    BSDLOG("ERROR!\n");
  }

  BSDLOG("result: %d\n", sb->resultval);

#if 0
    SOCKET s;
    char buf[MAXADDRLEN];

    TRACE(("setsockopt(%d,%d,0x%lx,0x%lx,%d) -> ",sd,(short)level,optname,optval,len));
	sd++;
    s = getsock(sb,sd);

    if (s != INVALID_SOCKET)
    {
	if (len > sizeof buf)
	{
	    write_log("BSDSOCK: WARNING - Excessive optlen in setsockopt() (%d)\n",len);
	    len = sizeof buf;
	}	
	if (level == IPPROTO_IP && optname == 2)
		{ // IP_HDRINCL emulated by icmp.dll
		sb->resultval = 0;
		return;
		}
	if (level == SOL_SOCKET && optname == SO_LINGER)
	{
	    ((LINGER *)buf)->l_onoff = get_long(optval);
	    ((LINGER *)buf)->l_linger = get_long(optval+4);
	}
	else
	{
	    if (len == 4) *(long *)buf = get_long(optval);
	    else if (len == 2) *(short *)buf = get_word(optval);
	    else write_log("BSDSOCK: ERROR - Unknown optlen (%d) in setsockopt(%d,%d)\n",level,optname);
	}

	// handle SO_EVENTMASK
	if (level == 0xffff && optname == 0x2001)
	{
	    long wsbevents = 0;
	    uae_u32 eventflags = get_long(optval);

	    sb->ftable[sd-1] = (sb->ftable[sd-1] & ~REP_ALL) | (eventflags & REP_ALL);

	    if (eventflags & REP_ACCEPT) wsbevents |= FD_ACCEPT;
	    if (eventflags & REP_CONNECT) wsbevents |= FD_CONNECT;
	    if (eventflags & REP_OOB) wsbevents |= FD_OOB;
	    if (eventflags & REP_READ) wsbevents |= FD_READ;
	    if (eventflags & REP_WRITE) wsbevents |= FD_WRITE;
	    if (eventflags & REP_CLOSE) wsbevents |= FD_CLOSE;
	    
	    if (sb->mtable[sd-1] || (sb->mtable[sd-1] = allocasyncmsg(sb,sd,s)))
	    {
		    WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,sb->mtable[sd-1],wsbevents);
		    sb->resultval = 0;
	    }
	    else sb->resultval = -1;
	}
	else sb->resultval = setsockopt(s,level,optname,buf,len);
		
	if (!sb->resultval)
	{
	    TRACE(("OK\n"));
	    return;
	}
	else SETERRNO;
		
	TRACE(("failed (%d)\n",sb->sb_errno));
    }
#endif
}

void host_setsockopt(struct socketbase *sb, uae_u32 sd, uae_u32 level, uae_u32 optname, uae_u32 optval, uae_u32 len) {

  struct JUAE_bsdsocket_Message msg;

  BSDLOG("======== %s ========\n",__func__);

  sb->reply_port=CreateMsgPort();
  msg.ExecMessage.mn_Node.ln_Type=NT_MESSAGE;
  msg.ExecMessage.mn_Length = sizeof(struct JUAE_bsdsocket_Message); 
  msg.ExecMessage.mn_ReplyPort = sb->reply_port; 
  BSDLOG("sb->reply_port: %lx\n", sb->reply_port);

  msg.cmd=(ULONG) BSD_setsockopt;
  msg.a=(ULONG) sb;
  msg.b=(ULONG) sd;
  msg.c=(ULONG) level;
  msg.d=(ULONG) optname;
  msg.e=(ULONG) optval;
  msg.f=(ULONG) len;

  BSDLOG("PutMsg..\n");
  PutMsg(sb->aros_port, &msg);
  BSDLOG("WaitPort..\n");
  WaitPort(sb->reply_port);
  BSDLOG("done!\n");
  DeleteMsgPort(sb->reply_port);
}



uae_u32 host_getsockopt(struct socketbase *sb, uae_u32 sd, uae_u32 level, uae_u32 optname, uae_u32 optval, uae_u32 optlen)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0

	SOCKET s;
	char buf[MAXADDRLEN];
	int len = sizeof(buf);

	TRACE(("getsockopt(%d,%d,0x%lx,0x%lx,0x%lx) -> ",sd,(short)level,optname,optval,optlen));
	sd++;
	s = getsock(sb,sd);
	
	if (s != INVALID_SOCKET)
	{
		if (!getsockopt(s,level,optname,buf,&len))
		{
			if (level == SOL_SOCKET && optname == SO_LINGER)
			{
				put_long(optval,((LINGER *)buf)->l_onoff);
				put_long(optval+4,((LINGER *)buf)->l_linger);
			}
			else
			{
				if (len == 4) put_long(optval,*(long *)buf);
				else if (len == 2) put_word(optval,*(short *)buf);
				else write_log("BSDSOCK: ERROR - Unknown optlen (%d) in setsockopt(%d,%d)\n",level,optname);
			}

//			put_long(optlen,len); // some programs pass the actual ength instead of a pointer to the length, so...
			TRACE(("OK (%d,%d)\n",len,*(long *)buf));
			return 0;
		}
		else
		{
			SETERRNO;
			TRACE(("failed (%d)\n",sb->sb_errno));
		}
	}

	return -1;
#endif
}

uae_u32 host_getsockname(struct socketbase *sb, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0

	SOCKET s;
	int len;
	struct sockaddr *rp_name;

	sd++;
	len = get_long(namelen);
	
	TRACE(("getsockname(%d,0x%lx,%d) -> ",sd,name,len));
	
	s = getsock(sb,sd);
	
	if (s != INVALID_SOCKET)
	{
		rp_name = (struct sockaddr *)get_real_address(name);
		
		if (getsockname(s,rp_name,&len))
		{
			SETERRNO;
			TRACE(("failed (%d)\n",sb->sb_errno));
		}
		else
		{
			TRACE(("%d\n",len));
			prepamigaaddr(rp_name,len);
			put_long(namelen,len);
			return 0;
		}
	}	

	return -1;
#endif
}

uae_u32 host_getpeername(struct socketbase *sb, uae_u32 sd, uae_u32 name, uae_u32 namelen)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
	SOCKET s;
	int len;
	struct sockaddr *rp_name;
	
	sd++;
	len = get_long(namelen);
	
	TRACE(("getpeername(%d,0x%lx,%d) -> ",sd,name,len));
	
	s = getsock(sb,sd);
	
	if (s != INVALID_SOCKET)
	{
		rp_name = (struct sockaddr *)get_real_address(name);
		
		if (getpeername(s,rp_name,&len))
		{
			SETERRNO;
			TRACE(("failed (%d)\n",sb->sb_errno));
		}
		else
		{
			TRACE(("%d\n",len));
			prepamigaaddr(rp_name,len);
			put_long(namelen,len);
			return 0;
		}
	}	

	return -1;
#endif
}

uae_u32 host_IoctlSocket(struct socketbase *sb, uae_u32 sd, uae_u32 request, uae_u32 arg)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
	SOCKET s;
	uae_u32 data;
	int success = SOCKET_ERROR;

	TRACE(("IoctlSocket(%d,0x%lx,0x%lx) ",sd,request,arg));
	sd++;
	s = getsock(sb,sd);

	if (s != INVALID_SOCKET)
	{
		switch (request)
		{
	                case FIOSETOWN:
			    sb->ownertask = get_long(arg);
			    success = 0;
			break;
			case FIOGETOWN:
			    put_long(arg,sb->ownertask);
			    success = 0;
			break;
			case FIONBIO:
				TRACE(("[FIONBIO] -> "));
				if (get_long(arg))
				{
					TRACE(("nonblocking\n"));
					sb->ftable[sd-1] &= ~SF_BLOCKING;
				}
				else
				{
					TRACE(("blocking\n"));
					sb->ftable[sd-1] |= SF_BLOCKING;
				}
				success = 0;
				break;
			case FIONREAD:
				ioctlsocket(s,request,(u_long *)&data);
				TRACE(("[FIONREAD] -> %d\n",data));
				put_long(arg,data);
				success = 0;
				break;
			case FIOASYNC:
				if (get_long(arg))
				{
					sb->ftable[sd-1] |= REP_ALL;

					TRACE(("[FIOASYNC] -> enabled\n"));
					if (sb->mtable[sd-1] || (sb->mtable[sd-1] = allocasyncmsg(sb,sd,s)))
					{
						WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,sb->mtable[sd-1],FD_ACCEPT | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE | FD_CLOSE);
						success = 0;
						break;
					}
				}
				else write_log(("BSDSOCK: WARNING - FIOASYNC disabling unsupported.\n"));

				success = -1;
				break;
			default:
				write_log("BSDSOCK: WARNING - Unknown IoctlSocket request: 0x%08lx\n",request);
				seterrno(sb,22);	// EINVAL
		}
	}
	
	return success;
#endif
}

int host_CloseSocket(struct socketbase *sb, int sd)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
    unsigned int wMsg;
    SOCKET s;
	
    TRACE(("CloseSocket(%d) -> ",sd));
	sd++;

    s = getsock(sb,sd);
    if (s != INVALID_SOCKET)
    {
	if (sb->mtable[sd-1])
	{
	    asyncsb[(sb->mtable[sd-1]-0xb000)/2] = NULL;
	    sb->mtable[sd-1] = 0;
	}

	if (checksd(sb ,sd) == TRUE)
		return 0;



	BEGINBLOCKING;

	for (;;)
	{
            shutdown(s,1);
	    if (!closesocket(s))
	    {
		releasesock(sb,sd);
		TRACE(("OK\n"));
		return 0;
	    }

	    SETERRNO;

	    if (sb->sb_errno != WSAEWOULDBLOCK-WSABASEERR || !(sb->ftable[sd-1] & SF_BLOCKING)) break;

	    if ((wMsg = allocasyncmsg(sb,sd,s)) != 0)
	    {
		WSAAsyncSelect(s,hWndSelector ? hAmigaWnd : hSockWnd,wMsg,FD_CLOSE);

	
		WAITSIGNAL;
		

		cancelasyncmsg(wMsg);
	
		if (sb->eintr)
		{
		    TRACE(("[interrupted]\n"));
		    break;
		}
	    }
	    else break;
	}

        ENDBLOCKING;
    }
	
    TRACE(("failed (%d)\n",sb->sb_errno));
  
    return -1;
#endif
}

// For the sake of efficiency, we do not malloc() the fd_sets here.
// 64 sockets should be enough for everyone.
#if 0
static void makesocktable(struct socketbase *sb, uae_u32 fd_set_amiga, struct fd_set *fd_set_win, int nfds, SOCKET addthis)
{
	int i, j;
	uae_u32 currlong, mask;
	SOCKET s;

	if (addthis != INVALID_SOCKET)
	{
		*fd_set_win->fd_array = addthis;
		fd_set_win->fd_count = 1;
	}
	else fd_set_win->fd_count = 0;

	if (!fd_set_amiga)
	{
		fd_set_win->fd_array[fd_set_win->fd_count] = INVALID_SOCKET;
		return;
	}

	if (nfds > sb->dtablesize)
	{
		write_log("BSDSOCK: ERROR - select()ing more sockets (%d) than socket descriptors available (%d)!\n",nfds,sb->dtablesize);
		nfds = sb->dtablesize;
	}

	for (j = 0; ; j += 32, fd_set_amiga += 4)
	{
		currlong = get_long(fd_set_amiga);

		mask = 1;
		
		for (i = 0; i < 32; i++, mask <<= 1)
		{
			if (i+j > nfds)
			{
				fd_set_win->fd_array[fd_set_win->fd_count] = INVALID_SOCKET;
				return;
			}
			
			if (currlong & mask)
			{
				s = getsock(sb,j+i+1);
				
				if (s != INVALID_SOCKET)
				{
					fd_set_win->fd_array[fd_set_win->fd_count++] = s;

					if (fd_set_win->fd_count >= FD_SETSIZE)
					{
						write_log("BSDSOCK: ERROR - select()ing more sockets (%d) than the hard-coded fd_set limit (%d) - please report\n",nfds,FD_SETSIZE);
						return;
					}
				}
			}
		}
	}

	fd_set_win->fd_array[fd_set_win->fd_count] = INVALID_SOCKET;
}

static void makesockbitfield(struct socketbase *sb, uae_u32 fd_set_amiga, struct fd_set *fd_set_win, int nfds)
{
	int n, i, j, val, mask;
	SOCKET currsock;

	for (n = 0; n < nfds; n += 32)
	{
		val = 0;
		mask = 1;
		
		for (i = 0; i < 32; i++, mask <<= 1)
		{
			if ((currsock = getsock(sb, n+i+1)) != INVALID_SOCKET)
			{ // Do not use sb->dtable directly because of Newsrog
				for (j = fd_set_win->fd_count; j--; )
				{
					if (fd_set_win->fd_array[j] == currsock)
					{
						val |= mask;
						break;
					}
				}
			}
		}
		put_long(fd_set_amiga,val);
		fd_set_amiga += 4;
	}
}

static void fd_zero(uae_u32 fdset, uae_u32 nfds)
{
	unsigned int i;
	
	for (i = 0; i < nfds; i += 32, fdset += 4) put_long(fdset,0);
}

// This seems to be the only way of implementing a cancelable WinSock2 select() call... sigh.
static unsigned int __stdcall thread_WaitSelect(void *index2)
{
    uae_u32 index = (uae_u32)index2;
    unsigned int result = 0;
    long nfds;
    uae_u32 readfds, writefds, exceptfds;
    uae_u32 timeout;
    struct fd_set readsocks, writesocks, exceptsocks;
    struct timeval tv;
    uae_u32 *args;

    struct socketbase *sb;

    for (;;)
    {
	    WaitForSingleObject(hEvents[index],INFINITE);

	    if ((args = threadargs[index]) != NULL)
	    {
		    sb = (struct socketbase *)*args;
		    nfds = args[1];
		    readfds = args[2];
		    writefds = args[3];
		    exceptfds = args[4];
		    timeout = args[5];
	    
		    // construct descriptor tables
		    makesocktable(sb,readfds,&readsocks,nfds,sb->sockAbort);
		    if (writefds) makesocktable(sb,writefds,&writesocks,nfds,INVALID_SOCKET);
		    if (exceptfds) makesocktable(sb,exceptfds,&exceptsocks,nfds,INVALID_SOCKET);
	    
		    if (timeout)
		    {
			    tv.tv_sec = get_long(timeout);
			    tv.tv_usec = get_long(timeout+4);
			    TRACE(("(timeout: %d.%06d) ",tv.tv_sec,tv.tv_usec));
		    }
	    
		    TRACE(("-> "));
	    
		    sb->resultval = select(nfds+1,&readsocks,writefds ? &writesocks : NULL,exceptfds ? &exceptsocks : NULL,timeout ? &tv : 0);
			if (sb->resultval == SOCKET_ERROR)
		    { 
			// select was stopped by sb->sockAbort
			if (readsocks.fd_count > 1)
				{
				makesocktable(sb,readfds,&readsocks,nfds,INVALID_SOCKET);
				tv.tv_sec = 0;
				tv.tv_usec = 10000;
				// Check for 10ms if data is available
				sb->resultval = select(nfds+1,&readsocks,writefds ? &writesocks : NULL,exceptfds ? &exceptsocks : NULL,&tv);
				if (sb->resultval == 0)
					{ // Now timeout -> really no data available
					if (GetLastError() != 0)
						{
						sb->resultval = SOCKET_ERROR;
						// Set old resultval
						}
					}   
				}
		    }
			if (FD_ISSET(sb->sockAbort,&readsocks))
				{
				if (sb->resultval != SOCKET_ERROR)
					{
					sb->resultval--;
					}
				}
			else
				{
	       		sb->needAbort = 0;
				}
		    if (sb->resultval == SOCKET_ERROR)
		    {
			    SETERRNO;
			    TRACE(("failed (%d) - ",sb->sb_errno));
			    if (readfds) fd_zero(readfds,nfds);
			    if (writefds) fd_zero(writefds,nfds);
			    if (exceptfds) fd_zero(exceptfds,nfds);
		    }
		    else
		    {
			    if (readfds) makesockbitfield(sb,readfds,&readsocks,nfds);
			    if (writefds) makesockbitfield(sb,writefds,&writesocks,nfds);
			    if (exceptfds) makesockbitfield(sb,exceptfds,&exceptsocks,nfds);
		    }
	    
		    SETSIGNAL;

		    threadargs[index] = NULL;
		    SetEvent(sb->hEvent);
	    }
    }
#ifndef __GNUC__
    _endthreadex( result );
#endif
    return result;
}
#endif

void host_WaitSelect(TrapContext *context, struct socketbase *sb, uae_u32 nfds, uae_u32 readfds, uae_u32 writefds, uae_u32 exceptfds, uae_u32 timeout, uae_u32 sigmp)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
	uae_u32 sigs, wssigs;
	int i;

	wssigs = sigmp ? get_long(sigmp) : 0;

	TRACE(("WaitSelect(%d,0x%lx,0x%lx,0x%lx,0x%lx,0x%lx) ",nfds,readfds,writefds,exceptfds,timeout,wssigs));

	if (!readfds && !writefds && !exceptfds && !timeout && !wssigs)
	{
		sb->resultval = 0;
		TRACE(("-> [ignored]\n"));
		return;
	}
	if (wssigs)
	{
		m68k_dreg (&regs, 0) = 0;
		m68k_dreg (&regs, 1) = wssigs;
		sigs = CallLib(get_long(4),-0x132) & wssigs;	// SetSignal()
		
		if (sigs)
		{
			TRACE(("-> [preempted by signals 0x%08lx]\n",sigs & wssigs));
			put_long(sigmp,sigs & wssigs);
			// Check for zero address -> otherwise WinUAE crashes
			if (readfds) fd_zero(readfds,nfds);
			if (writefds) fd_zero(writefds,nfds);
			if (exceptfds) fd_zero(exceptfds,nfds);
 			sb->resultval = 0;
			seterrno(sb,0);
			return;
		}
	}
	if (nfds == 0)
		{ // No sockets to check, only wait for signals
		m68k_dreg (&regs, 0) = wssigs;
		sigs = CallLib(get_long(4),-0x13e);	// Wait()

		put_long(sigmp,sigs & wssigs);

		if (readfds) fd_zero(readfds,nfds);
		if (writefds) fd_zero(writefds,nfds);
		if (exceptfds) fd_zero(exceptfds,nfds);
		sb->resultval = 0;
		return;
		}

	ResetEvent(sb->hEvent);

	sb->needAbort = 1;

	for (i = 0; i < MAX_SELECT_THREADS; i++) if (hThreads[i] && !threadargs[i]) break;

	if (i >= MAX_SELECT_THREADS)
	{
	    for (i = 0; i < MAX_SELECT_THREADS; i++)
	    {
		if (!hThreads[i])
		{
		    if ((hEvents[i] = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL || (hThreads[i] = (void *)THREAD(thread_WaitSelect,i)) == NULL)
		    {
			hThreads[i] = 0;
			write_log("BSDSOCK: ERROR - Thread/Event creation failed - error code: %d\n",GetLastError());
			seterrno(sb,12);	// ENOMEM
			sb->resultval = -1;
			return;
		    }
		    
		    // this should improve responsiveness
		    SetThreadPriority(hThreads[i],THREAD_PRIORITY_TIME_CRITICAL);
		    break;
		}
	    }
	}
	
	if (i >= MAX_SELECT_THREADS) write_log("BSDSOCK: ERROR - Too many select()s\n");
	else
	{
		SOCKET newsock = INVALID_SOCKET;

		threadargs[i] = (uae_u32 *)&sb;

		SetEvent(hEvents[i]);

		m68k_dreg (&regs, 0) = (((uae_u32)1)<<sb->signal)|sb->eintrsigs|wssigs;
		sigs = CallLib(get_long(4),-0x13e);	// Wait()
/*
		if ((1<<sb->signal) & sigs)
		{ // 2.3.2002/SR Fix for AmiFTP -> Thread is ready, no need to Abort
		    sb->needAbort = 0;
		}
*/
		if (sb->needAbort)
		{
			if ((newsock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) == INVALID_SOCKET)
				write_log("BSDSOCK: ERROR - Cannot create socket: %d\n",WSAGetLastError());
                        shutdown(sb->sockAbort,1);
			if (newsock != sb->sockAbort)
				{
				shutdown(sb->sockAbort,1);
				closesocket(sb->sockAbort);
				}
		}

		WaitForSingleObject(sb->hEvent,INFINITE);

		CANCELSIGNAL;

		if (newsock != INVALID_SOCKET) sb->sockAbort = newsock;

        if( sigmp )
        {
		    put_long(sigmp,sigs & wssigs);

		    if (sigs & sb->eintrsigs)
		    {
			    TRACE(("[interrupted]\n"));
 			    sb->resultval = -1;
			    seterrno(sb,4);	// EINTR
		    }
		    else if (sigs & wssigs)
		    {
			    TRACE(("[interrupted by signals 0x%08lx]\n",sigs & wssigs));
				if (readfds) fd_zero(readfds,nfds);
				if (writefds) fd_zero(writefds,nfds);
				if (exceptfds) fd_zero(exceptfds,nfds);
			    seterrno(sb,0);
 			    sb->resultval = 0;
		    }
		if (sb->resultval >= 0)
			{
			TRACE(("%d\n",sb->resultval));
			}
		else
			{
			TRACE(("%d errno %d\n",sb->resultval,sb->sb_errno));
			}
		
        }
		else TRACE(("%d\n",sb->resultval));
	}
#endif
}

uae_u32 host_Inet_NtoA(TrapContext *context, struct socketbase *sb, uae_u32 in) {

  char *addr;
	uae_u32 scratchbuf;
  BSDLOG("======== %s -> %s ========\n", __FILE__,__func__);
	BSDLOG("Inet_NtoA(%lx)\n",in);

  if(!getsocketbase()) {
    seterrno(sb, 1);
    return 0;
  }

	if ((addr = Inet_NtoA(in)) != 0) {
		scratchbuf = m68k_areg (&regs, 6)+offsetof(struct UAEBSDBase, scratchbuf);
		BSDLOG("addr: %s\n",addr);
		strncpyha(scratchbuf, addr, SCRATCHBUFSIZE);
		return scratchbuf;
  }
	seterrno(sb, 1);
  return 0;
	
#if 0
	char *addr;
	struct in_addr ina;

	*(uae_u32 *)&ina = htonl(in);


	{
		scratchbuf = m68k_areg (&regs, 6)+offsetof(struct UAEBSDBase,scratchbuf);
		strncpyha(scratchbuf,addr,SCRATCHBUFSIZE);
		TRACE(("%s\n",addr));
		return scratchbuf;
	}
	else SETERRNO;

	TRACE(("failed (%d)\n",sb->sb_errno));

	return 0;
#endif
}

uae_u32 host_inet_addr(uae_u32 cp)
{
	uae_u32 addr;
	char *cp_rp;

  BSDLOG("======== %s -> %s ========\n", __FILE__,__func__);
  if(!getsocketbase()) {
    return NULL;
  }

  BSDLOG("cp: %lx\n", cp);
	cp_rp = get_real_address(cp);

  BSDLOG("cp_rp: %s\n", cp_rp);

	//addr = htonl(inet_addr(cp_rp));
	addr = htonl(inet_addr(cp_rp));

	BSDLOG("inet_addr(%s) -> 0x%08lx\n",cp_rp,addr);

	return addr;
}

#if 0
int isfullscreen (void);
BOOL CheckOnline(struct socketbase *sb)
	{
	DWORD dwFlags;
	BOOL bReturn = TRUE;
	if (InternetGetConnectedState(&dwFlags,0) == FALSE)
		{ // Internet is offline
		if (InternetAttemptConnect(0) != ERROR_SUCCESS)
			{ // Show Dialer window
			sb->sb_errno = 10001;
			sb->sb_herrno = 1;
			bReturn = FALSE;
			// No success or aborted
			}
		if (isfullscreen())
			{
			ShowWindow (hAmigaWnd, SW_RESTORE);
			SetActiveWindow(hAmigaWnd);
			}
		}
	return(bReturn);
	}

static unsigned int __stdcall thread_get(void *index2)
{
    uae_u32 index = (uae_u32)index2;
    unsigned int result = 0;
    uae_u32 *args;
	uae_u32 name;
	uae_u32 namelen;
	long addrtype;
	char *name_rp;
	char *buf;
	
	
	struct socketbase *sb;

    for (;;)
    {
	    WaitForSingleObject(hGetEvents[index],INFINITE);
		if (threadGetargs[index] == -1)
			{
			threadGetargs[index] = NULL;
			}
	    if ((args = threadGetargs[index]) != NULL )
	    {
			sb = (struct socketbase *)*args;
			if (args[1] == 0)
				{ // gethostbyname or gethostbyaddr
				struct hostent *host;
				name = args[2];
				namelen = args[3];
				addrtype = args[4];
				buf = (char*) args[5];
				name_rp = get_real_address(name);

				if (strchr(name_rp,'.') == 0 || CheckOnline(sb) == TRUE)
					{ // Local Address or Internet Online ?
					if (addrtype == -1)
						{
						host = gethostbyname(name_rp);
						}
					else
						{
						host = gethostbyaddr(name_rp,namelen,addrtype);
						}
					if (threadGetargs[index] != -1)
						{ // No CTRL-C Signal
						if (host == 0)
							{
							// Error occured 
							SETERRNO;
							TRACE(("failed (%d) - ",sb->sb_errno));
							}
						else
							{
							seterrno(sb,0);
							memcpy(buf,host,sizeof(HOSTENT));
							}
						}
					}
				}
			if (args[1] == 1)
				{ // getprotobyname
				struct protoent  *proto;

				name = args[2];
				buf = (char*) args[5];
				name_rp = get_real_address(name);
				proto = getprotobyname (name_rp);
				if (threadGetargs[index] != -1)
					{ // No CTRL-C Signal
					if (proto == 0)
						{
						// Error occured 
						SETERRNO;
						TRACE(("failed (%d) - ",sb->sb_errno));
						}
					else
						{
						seterrno(sb,0);
						memcpy(buf,proto,sizeof(struct protoent));
						}
					}
				}
			if (args[1] == 2)
				{ // getservbyport and getservbyname
				uae_u32 nameport;
				uae_u32 proto;
				uae_u32 type;
				char *proto_rp = 0;
				struct servent *serv;

				nameport = args[2];
				proto = args[3];
				type = args[4];
				buf = (char*) args[5];
	
				if (proto) proto_rp = get_real_address(proto);

				if (type)
					{
					serv = getservbyport(nameport,proto_rp);
					}
				else
					{
					name_rp = get_real_address(nameport);
					serv = getservbyname(name_rp,proto_rp);
					}
				if (threadGetargs[index] != -1)
					{ // No CTRL-C Signal
					if (serv == 0)
						{
						// Error occured 
						SETERRNO;
						TRACE(("failed (%d) - ",sb->sb_errno));
						}
					else
						{
						seterrno(sb,0);
						memcpy(buf,serv,sizeof(struct servent));
						}
					}
				}




		    TRACE(("-> "));
	    
			if (threadGetargs[index] != -1)
				SETSIGNAL;

		    threadGetargs[index] = NULL;

			}
    }
#ifndef __GNUC__
    _endthreadex( result );
#endif
    return result;
}
#endif



void host_gethostbynameaddr_real(TrapContext *context, struct socketbase *sb, uae_u32 name68k, uae_u32 namelen, long addrtype) {

  char *name;
  struct hostent *h;
	int size, numaliases = 0, numaddr = 0;
	uae_u32 aptr;
	int i;

  BSDLOG("======== %s -> %s ========\n", __FILE__,__func__);
  if(!getsocketbase()) {
		seterrno(sb,1); /*???*/
    return;
  }

  name = get_real_address(name68k);
  BSDLOG("name: %s\n", name);

  h=gethostbyname(name);
  BSDLOG("h: %lx\n", h);
  if(!h) {
			sb->resultval = -1;
      return;
  }

  // compute total size of hostent
  size = 28;
  if (h->h_name != NULL) size += strlen(h->h_name)+1;

  if (h->h_aliases != NULL)
    while (h->h_aliases[numaliases])  {
      BSDLOG("alias[%d]: %s\n", numaliases, h->h_aliases[numaliases]);
      size += strlen(h->h_aliases[numaliases++])+5;
    }

  if (h->h_addr_list != NULL) {
    while (h->h_addr_list[numaddr]) numaddr++;
    size += numaddr*(h->h_length+4);
  }

  if (sb->hostent) {
    uae_FreeMem(context, sb->hostent, sb->hostentsize );
  }

  sb->hostent = uae_AllocMem(context, size, 0 );

  if (!sb->hostent) {
    write_log("BSDSOCK: WARNING - gethostby%s() ran out of Amiga memory (couldn't allocate %ld bytes) while returning result of lookup for '%s'\n",addrtype == -1 ? "name" : "addr",size,(char *)name);
    seterrno(sb,12); // ENOMEM
    return;
  }
  
  sb->hostentsize = size;
  
  aptr = sb->hostent+28+numaliases*4+numaddr*4;

  // transfer hostent to Amiga memory
  put_long(sb->hostent+4,sb->hostent+20);
  put_long(sb->hostent+8,h->h_addrtype);
  put_long(sb->hostent+12,h->h_length);
  put_long(sb->hostent+16,sb->hostent+24+numaliases*4);
  
  for (i = 0; i < numaliases; i++) {
    put_long(sb->hostent+20+i*4,addstr(&aptr,h->h_aliases[i]));
  }

  put_long(sb->hostent+20+numaliases*4,0);

  for (i = 0; i < numaddr; i++) {
    put_long(sb->hostent+24+(numaliases+i)*4,addmem(&aptr,h->h_addr_list[i],h->h_length));
  }
  put_long(sb->hostent+24+numaliases*4+numaddr*4,0);
  put_long(sb->hostent,aptr);
  addstr(&aptr,h->h_name);

  BSDLOG("OK (%s)\n",h->h_name);
  seterrno(sb,0);


#if 0
	HOSTENT *h;
	int size, numaliases = 0, numaddr = 0;
	uae_u32 aptr;
	char *name_rp;
	int i;

	uae_u32 args[6];

	uae_u32 addr;
	uae_u32 *addr_list[2];

	char buf[MAXGETHOSTSTRUCT];
	unsigned int wMsg = 0;

		


//	char on = 1;
//	InternetSetOption(0,INTERNET_OPTION_SETTINGS_CHANGED,&on,strlen(&on));
//  Do not use: Causes locks with some machines

	name_rp = get_real_address(name);


	if (addrtype == -1)
	{
		TRACE(("gethostbyname(%s) -> ",name_rp));
		
		// workaround for numeric host "names"
		if ((addr = inet_addr(name_rp)) != INADDR_NONE)
		{
			seterrno(sb,0);
			((HOSTENT *)buf)->h_name = name_rp;
			((HOSTENT *)buf)->h_aliases = NULL;
			((HOSTENT *)buf)->h_addrtype = AF_INET;
			((HOSTENT *)buf)->h_length = 4;
			((HOSTENT *)buf)->h_addr_list = (char **)&addr_list;
			addr_list[0] = &addr;
			addr_list[1] = NULL;
	
			goto kludge;
		}
	}
	else
	{
		TRACE(("gethostbyaddr(0x%lx,0x%lx,%ld) -> ",name,namelen,addrtype));
	}

	args[0] = (uae_u32) sb;
	args[1] = 0;
	args[2] = name;
	args[3] = namelen;
	args[4] = addrtype;
	args[5] = (uae_u32) &buf[0];

	for (i = 0; i < MAX_GET_THREADS; i++) 
		{
		if (threadGetargs[i] == -1)
			{
			threadGetargs[i] = 0;
			}
		if (hGetThreads[i] && !threadGetargs[i]) break;
		}

	if (i >= MAX_GET_THREADS)
	{
	    for (i = 0; i < MAX_GET_THREADS; i++)
	    {
		if (!hGetThreads[i])
		{
		    if ((hGetEvents[i] = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL || (hGetThreads[i] = (void *)THREAD(thread_get,i)) == NULL)
		    {
			hGetThreads[i] = 0;
			write_log("BSDSOCK: ERROR - Thread/Event creation failed - error code: %d\n",GetLastError());
			seterrno(sb,12);	// ENOMEM
			sb->resultval = -1;
			return;
		    }
		    break;
		}
	    }
	}
	
	if (i >= MAX_GET_THREADS) write_log("BSDSOCK: ERROR - Too many gethostbyname()s\n");
	else
	{
		bsdsetpriority (hGetThreads[i]);
		threadGetargs[i] = (uae_u32 *)&args[0];

		SetEvent(hGetEvents[i]);
		}
	sb->eintr = 0;
	while ( threadGetargs[i] != 0 && sb->eintr == 0) 
		{	
		WAITSIGNAL;
		if (sb->eintr == 1)
			threadGetargs[i] = -1;
		}
	
	CANCELSIGNAL;
	
	if (!sb->sb_errno)
	{
kludge:
		h = (HOSTENT *)buf;
		
		// compute total size of hostent
		size = 28;
		if (h->h_name != NULL) size += strlen(h->h_name)+1;

		if (h->h_aliases != NULL)
			while (h->h_aliases[numaliases]) size += strlen(h->h_aliases[numaliases++])+5;

		if (h->h_addr_list != NULL)
		{
			while (h->h_addr_list[numaddr]) numaddr++;
			size += numaddr*(h->h_length+4);
		}

		if (sb->hostent)
		{
            uae_FreeMem( sb->hostent, sb->hostentsize );
		}

        sb->hostent = uae_AllocMem( size, 0 );

		if (!sb->hostent)
		{
			write_log("BSDSOCK: WARNING - gethostby%s() ran out of Amiga memory (couldn't allocate %ld bytes) while returning result of lookup for '%s'\n",addrtype == -1 ? "name" : "addr",size,(char *)name);
			seterrno(sb,12); // ENOMEM
			return;
		}
		
		sb->hostentsize = size;
		
		aptr = sb->hostent+28+numaliases*4+numaddr*4;
	
		// transfer hostent to Amiga memory
		put_long(sb->hostent+4,sb->hostent+20);
		put_long(sb->hostent+8,h->h_addrtype);
		put_long(sb->hostent+12,h->h_length);
		put_long(sb->hostent+16,sb->hostent+24+numaliases*4);
		
		for (i = 0; i < numaliases; i++) put_long(sb->hostent+20+i*4,addstr(&aptr,h->h_aliases[i]));
		put_long(sb->hostent+20+numaliases*4,0);
		for (i = 0; i < numaddr; i++) put_long(sb->hostent+24+(numaliases+i)*4,addmem(&aptr,h->h_addr_list[i],h->h_length));
		put_long(sb->hostent+24+numaliases*4+numaddr*4,0);
		put_long(sb->hostent,aptr);
		addstr(&aptr,h->h_name);

		TRACE(("OK (%s)\n",h->h_name));
		seterrno(sb,0);
	}
	else
	{
		TRACE(("failed (%d/%d)\n",sb->sb_errno,sb->sb_herrno));
	}

#endif
}

/* go to according task and call back _real .. */
void host_gethostbynameaddr(TrapContext *context, struct socketbase *sb, uae_u32 name68k, uae_u32 namelen, long addrtype) {
  struct JUAE_bsdsocket_Message msg;

  BSDLOG("======== %s ========\n",__func__);

  BSDLOG("dummy host_gethostbynameaddr sending message ..\n");

  sb->reply_port=CreateMsgPort();

  msg.ExecMessage.mn_Node.ln_Type=NT_MESSAGE;
  msg.ExecMessage.mn_Length = sizeof(struct JUAE_bsdsocket_Message); 
  msg.ExecMessage.mn_ReplyPort = sb->reply_port; 
  BSDLOG("sb->reply_port: %lx\n", sb->reply_port);

  msg.cmd=(ULONG) BSD_gethostbynameaddr;
  msg.a=(ULONG) context;
  msg.b=(ULONG) sb;
  msg.c=(ULONG) name68k;
  msg.d=(ULONG) namelen;
  msg.e=(ULONG) addrtype;

  BSDLOG("PutMsg..\n");
  PutMsg(sb->aros_port, &msg);
  BSDLOG("WaitPort..\n");
  WaitPort(sb->reply_port);
  BSDLOG("done!\n");

  DeleteMsgPort(sb->reply_port);

}

void host_getprotobyname(TrapContext *context, struct socketbase *sb, uae_u32 name) {
	char *name_rp;
  struct protoent *p;

	int size, numaliases = 0;
	uae_u32 aptr;
	int i;

  BSDLOG("======== %s -> %s ========\n", __FILE__,__func__);
  if(!getsocketbase()) {
		seterrno(sb,1); /*???*/
    return;
  }

  BSDLOG("entered(name %lx, sb %lx)\n", name, sb);

	name_rp = get_real_address(name);

	BSDLOG("getprotobyname(%s)\n",name_rp);

  p=getprotobyname(name_rp); /* this returns a static buffer, no need to free it */

  if(!p) {
    BSDLOG("getprotobyname ERROR!\n");
    sb->sb_errno=1;
		seterrno(sb,1); /*???*/
    return;
  }


  // compute total size of protoent
  size = 16;
  if (p->p_name != NULL) size += strlen(p->p_name)+1;

  if (p->p_aliases != NULL) {
    while (p->p_aliases[numaliases]) {
      size += strlen(p->p_aliases[numaliases++])+5;
    }
  }

  if (sb->protoent) {
    uae_FreeMem(context, sb->protoent, sb->protoentsize); /* free last protoent, necessary? */
  }

  sb->protoent = uae_AllocMem(context, size, 0); /* get amiga memory */

  if (!sb->protoent) {
    write_log("BSDSOCK: WARNING - getprotobyname() ran out of Amiga memory (couldn't allocate %ld bytes) while returning result of lookup for '%s'\n",size,(char *)name);
    BSDLOG("BSDSOCK: WARNING - getprotobyname() ran out of Amiga memory (couldn't allocate %ld bytes) while returning result of lookup for '%s'\n",size,(char *)name);
    seterrno(sb,12); // ENOMEM
    return;
  }

  sb->protoentsize = size;
  
  aptr = sb->protoent+16+numaliases*4;

  // transfer protoent to Amiga memory
  put_long(sb->protoent+4,sb->protoent+12);
  put_long(sb->protoent+8,p->p_proto);
  
  for (i = 0; i < numaliases; i++) put_long(sb->protoent+12+i*4,addstr(&aptr,p->p_aliases[i]));
  put_long(sb->protoent+12+numaliases*4,0);
  put_long(sb->protoent,aptr);
  addstr(&aptr,p->p_name);
  BSDLOG("OK (%s, %lx)\n",p->p_name,p->p_proto);
  seterrno(sb,0);

#if 0
	args[0] = (uae_u32) sb;
	args[1] = 1;
	args[2] = name;
	args[5] = (uae_u32) &buf[0];

	for (i = 0; i < MAX_GET_THREADS; i++) 
		{
		if (threadGetargs[i] == -1)
			{
			threadGetargs[i] = 0;
			}
		if (hGetThreads[i] && !threadGetargs[i]) break;
		}
	if (i >= MAX_GET_THREADS)
	{
	    for (i = 0; i < MAX_GET_THREADS; i++)
	    {
		if (!hGetThreads[i])
		{
		    if ((hGetEvents[i] = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL || (hGetThreads[i] = (void *)THREAD(thread_get,i)) == NULL)
		    {
			hGetThreads[i] = 0;
			write_log("BSDSOCK: ERROR - Thread/Event creation failed - error code: %d\n",GetLastError());
			seterrno(sb,12);	// ENOMEM
			sb->resultval = -1;
			return;
		    }
		    break;
		}
	    }
	}
	
	if (i >= MAX_GET_THREADS) write_log("BSDSOCK: ERROR - Too many getprotobyname()s\n");
	else
	{
		bsdsetpriority (hGetThreads[i]);

		threadGetargs[i] = (uae_u32 *)&args[0];

		SetEvent(hGetEvents[i]);
		}

	sb->eintr = 0;
	while ( threadGetargs[i] != 0 && sb->eintr == 0) 
		{	
		WAITSIGNAL;
		if (sb->eintr == 1)
			threadGetargs[i] = -1;
		}

	CANCELSIGNAL;


	if (!sb->sb_errno)
	{
		p = (PROTOENT *)buf;

		// compute total size of protoent
		size = 16;
		if (p->p_name != NULL) size += strlen(p->p_name)+1;

		if (p->p_aliases != NULL)
			while (p->p_aliases[numaliases]) size += strlen(p->p_aliases[numaliases++])+5;

		if (sb->protoent)
		{
            uae_FreeMem( sb->protoent, sb->protoentsize );
		}

		sb->protoent = uae_AllocMem( size, 0 );

		if (!sb->protoent)
		{
			write_log("BSDSOCK: WARNING - getprotobyname() ran out of Amiga memory (couldn't allocate %ld bytes) while returning result of lookup for '%s'\n",size,(char *)name);
			seterrno(sb,12); // ENOMEM
			return;
		}

		sb->protoentsize = size;
		
		aptr = sb->protoent+16+numaliases*4;
	
		// transfer protoent to Amiga memory
		put_long(sb->protoent+4,sb->protoent+12);
		put_long(sb->protoent+8,p->p_proto);
		
		for (i = 0; i < numaliases; i++) put_long(sb->protoent+12+i*4,addstr(&aptr,p->p_aliases[i]));
		put_long(sb->protoent+12+numaliases*4,0);
		put_long(sb->protoent,aptr);
		addstr(&aptr,p->p_name);
		TRACE(("OK (%s, %d)\n",p->p_name,p->p_proto));
		seterrno(sb,0);
	}
	else
	{
		TRACE(("failed (%d)\n",sb->sb_errno));
	}
#endif

}


void host_getservbynameport(TrapContext *context, struct socketbase *sb, uae_u32 nameport, uae_u32 proto, uae_u32 type)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
	SERVENT *s;
	int size, numaliases = 0;
	uae_u32 aptr;
	char *name_rp = NULL, *proto_rp = NULL;
	int i;

	char buf[MAXGETHOSTSTRUCT];
	uae_u32 args[6];

	if (proto) proto_rp = get_real_address(proto);

	if (type)
	{
		TRACE(("getservbyport(%d,%s) -> ",nameport,proto_rp ? proto_rp : "NULL"));
	}
	else
	{
		name_rp = get_real_address(nameport);
		TRACE(("getservbyname(%s,%s) -> ",name_rp,proto_rp ? proto_rp : "NULL"));
	}

	args[0] = (uae_u32) sb;
	args[1] = 2;
	args[2] = nameport;
	args[3] = proto;
	args[4] = type;
	args[5] = (uae_u32) &buf[0];

	for (i = 0; i < MAX_GET_THREADS; i++) 
		{
		if (threadGetargs[i] == -1)
			{
			threadGetargs[i] = 0;
			}
		if (hGetThreads[i] && !threadGetargs[i]) break;
		}
	if (i >= MAX_GET_THREADS)
	{
	    for (i = 0; i < MAX_GET_THREADS; i++)
	    {
		if (!hGetThreads[i])
		{
		    if ((hGetEvents[i] = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL || (hGetThreads[i] = (void *)THREAD(thread_get,i)) == NULL)
		    {
			hGetThreads[i] = 0;
			write_log("BSDSOCK: ERROR - Thread/Event creation failed - error code: %d\n",GetLastError());
			seterrno(sb,12);	// ENOMEM
			sb->resultval = -1;
			return;
		    }
		    
		    break;
		}
	    }
	}
	
	if (i >= MAX_GET_THREADS) write_log("BSDSOCK: ERROR - Too many getprotobyname()s\n");
	else
	{
		bsdsetpriority (hGetThreads[i]);

		threadGetargs[i] = (uae_u32 *)&args[0];

		SetEvent(hGetEvents[i]);
		}

	sb->eintr = 0;
	while ( threadGetargs[i] != 0 && sb->eintr == 0) 
		{	
		WAITSIGNAL;
		if (sb->eintr == 1)
			threadGetargs[i] = -1;
		}

	CANCELSIGNAL;

	if (!sb->sb_errno)
	{
		s = (SERVENT *)buf;

		// compute total size of servent
		size = 20;
		if (s->s_name != NULL) size += strlen(s->s_name)+1;
		if (s->s_proto != NULL) size += strlen(s->s_proto)+1;

		if (s->s_aliases != NULL)
			while (s->s_aliases[numaliases]) size += strlen(s->s_aliases[numaliases++])+5;

		if (sb->servent)
		{
            uae_FreeMem( sb->servent, sb->serventsize );
		}

        sb->servent = uae_AllocMem( size, 0 );

		if (!sb->servent)
		{
			write_log("BSDSOCK: WARNING - getservby%s() ran out of Amiga memory (couldn't allocate %ld bytes)\n",type ? "port" : "name",size);
			seterrno(sb,12); // ENOMEM
			return;
		}

		sb->serventsize = size;
		
		aptr = sb->servent+20+numaliases*4;
	
		// transfer servent to Amiga memory
		put_long(sb->servent+4,sb->servent+16);
		put_long(sb->servent+8,(unsigned short)htons(s->s_port));
		
		for (i = 0; i < numaliases; i++) put_long(sb->servent+16+i*4,addstr(&aptr,s->s_aliases[i]));
		put_long(sb->servent+16+numaliases*4,0);
		put_long(sb->servent,aptr);
		addstr(&aptr,s->s_name);
		put_long(sb->servent+12,aptr);
		addstr(&aptr,s->s_proto);

		TRACE(("OK (%s, %d)\n",s->s_name,(unsigned short)htons(s->s_port)));
		seterrno(sb,0);
	}
	else
	{
		TRACE(("failed (%d)\n",sb->sb_errno));
	}

#endif
}



uae_u32 host_gethostname(uae_u32 name, uae_u32 namelen)
{
  BSDLOG("NOT YET IMPLEMENTED!\n");
#if 0
	return gethostname(get_real_address(name),namelen);
#endif
}

void host_getprotobynumber (TrapContext *context, struct socketbase *sb, uae_u32 any) {
  BSDLOG("NOT YET IMPLEMENTED!\n");
}
#endif

void aros_bsdsocket_kill_thread(struct socketbase *sb) {

  struct JUAE_bsdsocket_Message msg;

  BSDLOG("======== %s ========\n",__func__);

  BSDLOG("sb: %lx\n", sb);

  sb->reply_port=CreateMsgPort();

  msg.ExecMessage.mn_Node.ln_Type=NT_MESSAGE;
  msg.ExecMessage.mn_Length = sizeof(struct JUAE_bsdsocket_Message); 
  msg.ExecMessage.mn_ReplyPort = sb->reply_port; 
  BSDLOG("sb->reply_port: %lx\n", sb->reply_port);

  msg.cmd=(ULONG) BSD_killme;
  msg.a  =(ULONG) sb;

  BSDLOG("PutMsg..\n");
  PutMsg(sb->aros_port, &msg);
  BSDLOG("WaitPort..\n");
  WaitPort(sb->reply_port);
  BSDLOG("done!\n");

  DeleteMsgPort(sb->reply_port);

}

void aros_bsdsocket_kill_thread_real(struct socketbase *sb) {

  BSDLOG("======== %s ========\n",__func__);

  BSDLOG("sb: %lx\n", sb);

  if(sb->mempool) {
    BSDLOG("mempool: %lx\n", sb->mempool);
    DeletePool(sb->mempool);
    sb->mempool=NULL;
  }

  if(SocketBase) {
    BSDLOG("close library..\n");
    CloseLibrary(SocketBase);
    SocketBase=NULL;
  }

  BSDLOG("SocketBase closed, mempool deleted\n");
}


