
#include "sysconfig.h"

#include <stdlib.h>
#include <stdarg.h>

#include "sysdeps.h"
#include "uaeipc.h"
#include "options.h"
#include "zfile.h"
#include "inputdevice.h"

#ifndef __AROS__
#include <windows.h>
#endif

#define IPC_BUFFER_SIZE 16384
#define MAX_OUTMESSAGES 30
#define MAX_BINMESSAGE 32

struct uaeipc
{
#ifndef __AROS__
	
	HANDLE hipc, olevent;
	OVERLAPPED ol;
	uae_u8 buffer[IPC_BUFFER_SIZE], outbuf[IPC_BUFFER_SIZE];
	int connected, readpending, writepending;
	int binary;
	TCHAR *outmsg[MAX_OUTMESSAGES];
	int outmessages;
	uae_u8 outbin[MAX_OUTMESSAGES][MAX_BINMESSAGE];
	int outbinlen[MAX_OUTMESSAGES];
#endif
};

static void parsemessage(TCHAR *in, struct uae_prefs *p, TCHAR *out, int outsize)
{
	TODO();
#if 0
	out[0] = 0;
	if (!_tcsncmp (in, _T("CFG "), 4) || !_tcsncmp (in, _T("EVT "), 4)) {
		TCHAR tmpout[256];
		int index = -1;
		int cnt = 0;
		in += 4;
		for (;;) {
			int ret;
			tmpout[0] = 0;
			ret = cfgfile_modify (index, in, _tcslen (in), tmpout, sizeof (tmpout) * sizeof (TCHAR));
			index++;
			if (_tcslen (tmpout) > 0) {
				if (_tcslen (out) == 0)
					_tcscat (out, _T("200 "));
				_tcsncat (out, _T("\n"), outsize);
				_tcsncat (out, tmpout, outsize);
			}
			cnt++;
			if (ret >= 0)
				break;
		}
		if (_tcslen (out) == 0)
			_tcscat (out, _T("404"));
	} else {
		_tcscpy (out, _T("501"));
	}
#endif
}

static int listenIPC (void *vipc)
{
	TODO();
	return 0;
#if 0
	struct uaeipc *ipc = (struct uaeipc*)vipc;
	DWORD err;

	memset(&ipc->ol, 0, sizeof (OVERLAPPED));
	ipc->ol.hEvent = ipc->olevent;
	if (ConnectNamedPipe(ipc->hipc, &ipc->ol)) {
		write_log (_T("IPC: ConnectNamedPipe init failed, err=%d\n"), GetLastError());
		closeIPC(ipc);
		return 0;
	}
	err = GetLastError();
	if (err == ERROR_PIPE_CONNECTED) {
		if (SetEvent(ipc->olevent)) {
			write_log (_T("IPC: ConnectNamedPipe SetEvent failed, err=%d\n"), GetLastError());
			closeIPC(ipc);
			return 0;
		}
	} else if (err != ERROR_IO_PENDING) {
		write_log (_T("IPC: ConnectNamedPipe failed, err=%d\n"), err);
		closeIPC(ipc);
		return 0;
	}
	return 1;
#endif
}

static void disconnectIPC (void *vipc)
{
	TODO();
#if 0
	struct uaeipc *ipc = (struct uaeipc*)vipc;
	ipc->readpending = ipc->writepending = FALSE;
	if (ipc->connected) {
		if (!DisconnectNamedPipe(ipc->hipc))
			write_log (_T("IPC: DisconnectNamedPipe failed, err=%d\n"), GetLastError());
		ipc->connected = FALSE;
	}
#endif
}

static void resetIPC (void *vipc)
{
	TODO();
#if 0
	struct uaeipc *ipc = (struct uaeipc*)vipc;
	disconnectIPC (ipc);
	listenIPC (ipc);
#endif
}

void closeIPC (void *vipc)
{
	TODO();
#if 0
	struct uaeipc *ipc = (struct uaeipc*)vipc;
	if (!ipc)
		return;
	disconnectIPC (ipc);
	if (ipc->hipc == INVALID_HANDLE_VALUE)
		return;
	CloseHandle (ipc->hipc);
	ipc->hipc = INVALID_HANDLE_VALUE;
	if (ipc->olevent != INVALID_HANDLE_VALUE)
		CloseHandle (ipc->olevent);
	ipc->olevent = INVALID_HANDLE_VALUE;
	xfree (ipc);
#endif
}

void *createIPC (const TCHAR *name, int binary)
{
	TODO();
#if 0
	TCHAR tmpname[100];
	int cnt = 0;
	struct uaeipc *ipc;

	ipc = xcalloc (struct uaeipc, 1);
	ipc->connected = FALSE;
	ipc->readpending = FALSE;
	ipc->writepending = FALSE;
	ipc->olevent = INVALID_HANDLE_VALUE;
	ipc->binary = 1;
	while (cnt < 10) {
		_stprintf (tmpname, _T("\\\\.\\pipe\\%s"), name);
		if (cnt > 0) {
			TCHAR *p = tmpname + _tcslen (tmpname);
			_stprintf (p, _T("_%d"), cnt);
		}
		ipc->hipc = CreateNamedPipe (tmpname,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
			1, IPC_BUFFER_SIZE, IPC_BUFFER_SIZE,
			NMPWAIT_USE_DEFAULT_WAIT, NULL);
		if (ipc->hipc == INVALID_HANDLE_VALUE) {
			DWORD err = GetLastError ();
			if (err == ERROR_ALREADY_EXISTS || err == ERROR_PIPE_BUSY) {
				cnt++;
				continue;
			}
			return 0;
		}
		break;
	}
	write_log (_T("IPC: Named Pipe '%s' open\n"), tmpname);
	ipc->olevent = CreateEvent(NULL, TRUE, TRUE, NULL);
	if (listenIPC(ipc))
		return ipc;
	closeIPC(ipc);
#endif
	return NULL;
}

void *geteventhandleIPC (void *vipc)
{
	TODO();
#if 0
	struct uaeipc *ipc = (struct uaeipc*)vipc;
	if (!ipc)
		return INVALID_HANDLE_VALUE;
	return ipc->olevent;
#endif
	return NULL;
}

int sendIPC (void *vipc, TCHAR *msg)
{
	TODO();
#if 0
	struct uaeipc *ipc = (struct uaeipc*)vipc;
	if (ipc->hipc == INVALID_HANDLE_VALUE)
		return 0;
	if (ipc->outmessages >= MAX_OUTMESSAGES)
		return 0;
	ipc->outmsg[ipc->outmessages++] = my_strdup (msg);
	if (!ipc->readpending && !ipc->writepending)
		SetEvent (ipc->olevent);
	return 1;
#endif
	return 0;
}
int sendBinIPC (void *vipc, uae_u8 *msg, int len)
{
	TODO();
#if 0
	struct uaeipc *ipc = (struct uaeipc*)vipc;
	if (ipc->hipc == INVALID_HANDLE_VALUE)
		return 0;
	if (ipc->outmessages >= MAX_OUTMESSAGES)
		return 0;
	ipc->outbinlen[ipc->outmessages] = len;
	memcpy (&ipc->outbin[ipc->outmessages++][0], msg, len);
	if (!ipc->readpending && !ipc->writepending)
		SetEvent (ipc->olevent);
	return 1;
#endif
	return 0;
}

int checkIPC (void *vipc, struct uae_prefs *p)
{
	TODO();
#if 0
	struct uaeipc *ipc = (struct uaeipc*)vipc;
	BOOL ok;
	DWORD ret, err;

	if (!ipc)
		return 0;
	if (ipc->hipc == INVALID_HANDLE_VALUE)
		return 0;
	if (WaitForSingleObject(ipc->olevent, 0) != WAIT_OBJECT_0)
		return 0;
	if (!ipc->readpending && !ipc->writepending && ipc->outmessages > 0) {
		memset (&ipc->ol, 0, sizeof (OVERLAPPED));
		ipc->ol.hEvent = ipc->olevent;
		if (ipc->binary) {
			ok = WriteFile (ipc->hipc, &ipc->outbin[ipc->outmessages][0], ipc->outbinlen[ipc->outmessages], &ret, &ipc->ol);
		} else {
			ok = WriteFile (ipc->hipc, ipc->outmsg[ipc->outmessages], (_tcslen (ipc->outmsg[ipc->outmessages]) + 1) * sizeof (TCHAR), &ret, &ipc->ol);
		}
		xfree (ipc->outmsg[ipc->outmessages--]);
		err = GetLastError ();
		if (!ok && err != ERROR_IO_PENDING) {
			write_log (_T("IPC: WriteFile() err=%d\n"), err);
			resetIPC (ipc);
			return 0;
		}
		ipc->writepending = TRUE;
		return 1;
	}
	if (ipc->readpending || ipc->writepending) {
		ok = GetOverlappedResult (ipc->hipc, &ipc->ol, &ret, FALSE);
		if (!ok) {
			err = GetLastError ();
			if (err == ERROR_IO_INCOMPLETE)
				return 0;
			write_log (_T("IPC: GetOverlappedResult error %d\n"), err);
			resetIPC (ipc);
			return 0;
		}
		if (!ipc->connected) {
			write_log (_T("IPC: Pipe connected\n"));
			ipc->connected = TRUE;
			return 0;
		}
		if (ipc->writepending) {
			ipc->writepending = FALSE;
			SetEvent (ipc->ol.hEvent);
			memset (&ipc->ol, 0, sizeof (OVERLAPPED));
			ipc->ol.hEvent = ipc->olevent;
			return 0;
		}
	}
	if (!ipc->readpending) {
		ok = ReadFile (ipc->hipc, ipc->buffer, IPC_BUFFER_SIZE, &ret, &ipc->ol);
		err = GetLastError ();
		if (!ok) {
			if (err == ERROR_IO_PENDING) {
				ipc->readpending = TRUE;
				return 0;
			} else if (err == ERROR_BROKEN_PIPE) {
				write_log (_T("IPC: IPC client disconnected\n"));
			} else {
				write_log (_T("IPC: ReadFile() err=%d\n"), err);
			}
			resetIPC (ipc);
			return 0;
		}
	}
	ipc->readpending = FALSE;
	if (ipc->binary) {

	} else {
		write_log (_T("IPC: got message '%s'\n"), ipc->buffer);
		parsemessage ((TCHAR*)ipc->buffer, p, (TCHAR*)ipc->outbuf, sizeof ipc->outbuf);
		memset (&ipc->ol, 0, sizeof (OVERLAPPED));
		ipc->ol.hEvent = ipc->olevent;
		ok = WriteFile (ipc->hipc, ipc->outbuf, strlen ((char*)ipc->outbuf) + 1, &ret, &ipc->ol);
		err = GetLastError ();
		if (!ok && err != ERROR_IO_PENDING) {
			write_log (_T("IPC: WriteFile() err=%d\n"), err);
			resetIPC (ipc);
			return 0;
		}
		ipc->writepending = TRUE;
	}
	return 1;
#endif
	return 0;
}

int isIPC (const TCHAR *pipename)
{
	TODO();
#if 0
	HANDLE p;

	p = CreateFile(
		pipename,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (p == INVALID_HANDLE_VALUE)
		return 0;
	CloseHandle (p);
	return 1;
#endif
	return 0;
}
