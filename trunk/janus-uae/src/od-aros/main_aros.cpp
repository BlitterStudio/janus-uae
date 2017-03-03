/************************************************************************ 
 *
 * main
 *
 * Copyright 2011 Oliver Brunner - aros<at>oliver-brunner.de
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

#include <exec/execbase.h>
#include <proto/dos.h>

//#define JUAE_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "uae.h"
#include "options.h"
#include "aros.h"

#include "gfx.h"
#include "memory.h"
#include "gui.h"
#include "registry.h"
#ifdef __AROS__
#include "gui/gui_mui.h"
#include "winnt.h"
#endif

TCHAR VersionStr[256];
TCHAR BetaStr[64];

int start_data = 0;

void fullpath(TCHAR *linkfile, int size);
extern void logging_deinit (void);

/*
 * getstartpaths
 *
 * WinUAE does quite some magic here, to detect PROGDIR etc.
 * This seems uneccessary for AROS.
 *
 * What might be still to do, is the AmigaForever etc. stuff.
 */
static void getstartpaths (void)
{
#if 0
	UAEREG *key;
#endif

  DebOut("entered\n");

  DebOut("inipath: %s\n", inipath);

  path_type = PATH_TYPE_WINUAE;
  relativepaths = 1;

  strcpy(start_path_exe,  "PROGDIR:");
  strcpy(start_path_data, "PROGDIR:");

  if(!my_existsdir("PROGDIR:Configurations")) {
    my_mkdir("PROGDIR:Configurations");
  }
  if(!my_existsdir("PROGDIR:Screenshots")) {
    my_mkdir("PROGDIR:Screenshots");
  }
  if(!my_existsdir("PROGDIR:Savestates")) {
    my_mkdir("PROGDIR:Savestates");
  }
  if(!my_existsdir("PROGDIR:Amiga Files")) {
    my_mkdir("PROGDIR:Amiga Files");
  }
  if(!my_existsdir("PROGDIR:Roms")) {
    my_mkdir("PROGDIR:Roms");
  }


#if 0

  /* _wpgmptr: in Windows: "The path of the executable file" */
	GetFullPathName (_wpgmptr, sizeof(start_path_exe) / sizeof (TCHAR), start_path_exe, NULL);
	if((posn = _tcsrchr (start_path_exe, '\\')))
		posn[1] = 0;

	if (path_type == PATH_TYPE_DEFAULT && inipath) {
		path_type = PATH_TYPE_WINUAE;
		_tcscpy (xstart_path_uae, start_path_exe);
		relativepaths = 1;
	} else if (path_type == PATH_TYPE_DEFAULT && start_data == 0 && key) {
		bool ispath = false;
		_tcscpy (tmp2, start_path_exe);
		_tcscat (tmp2, _T("configurations\\configuration.cache"));
		v = GetFileAttributes (tmp2);
		if (v != INVALID_FILE_ATTRIBUTES && !(v & FILE_ATTRIBUTE_DIRECTORY))
			ispath = true;
		_tcscpy (tmp2, start_path_exe);
		_tcscat (tmp2, _T("roms"));
		v = GetFileAttributes (tmp2);
		if (v != INVALID_FILE_ATTRIBUTES && (v & FILE_ATTRIBUTE_DIRECTORY))
			ispath = true;
		if (!ispath) {
			if (SUCCEEDED (SHGetFolderPath (NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, tmp))) {
				GetFullPathName (tmp, sizeof tmp / sizeof (TCHAR), tmp2, NULL);
				// installed in Program Files?
				if (_tcsnicmp (tmp, tmp2, _tcslen (tmp)) == 0) {
					if (SUCCEEDED (SHGetFolderPath (NULL, CSIDL_COMMON_DOCUMENTS, NULL, SHGFP_TYPE_CURRENT, tmp))) {
						fixtrailing (tmp);
						_tcscpy (tmp2, tmp);
						_tcscat (tmp2, _T("Amiga Files"));
						CreateDirectory (tmp2, NULL);
						_tcscat (tmp2, _T("\\WinUAE"));
						CreateDirectory (tmp2, NULL);
						v = GetFileAttributes (tmp2);
						if (v != INVALID_FILE_ATTRIBUTES && (v & FILE_ATTRIBUTE_DIRECTORY)) {
							_tcscat (tmp2, _T("\\"));
							path_type = PATH_TYPE_NEWWINUAE;
							_tcscpy (tmp, tmp2);
							_tcscat (tmp, _T("Configurations"));
							CreateDirectory (tmp, NULL);
							_tcscpy (tmp, tmp2);
							_tcscat (tmp, _T("Screenshots"));
							CreateDirectory (tmp, NULL);
							_tcscpy (tmp, tmp2);
							_tcscat (tmp, _T("Savestates"));
							CreateDirectory (tmp, NULL);
							_tcscpy (tmp, tmp2);
							_tcscat (tmp, _T("Screenshots"));
							CreateDirectory (tmp, NULL);
						}
					}
				}
			}
		}
	}

	_tcscpy (tmp, start_path_exe);
	_tcscat (tmp, _T("roms"));
#if 0
	if (isfilesindir (tmp)) {
		_tcscpy (xstart_path_uae, start_path_exe);
	}
#endif
	_tcscpy (tmp, start_path_exe);
	_tcscat (tmp, _T("configurations"));
#if 0
	if (isfilesindir (tmp)) {
		_tcscpy (xstart_path_uae, start_path_exe);
	}
#endif

#if 0
	p = _wgetenv (_T("AMIGAFOREVERDATA"));
	if (p) {
		_tcscpy (tmp, p);
		fixtrailing (tmp);
		_tcscpy (start_path_new2, p);
		v = GetFileAttributes (tmp);
		if (v != INVALID_FILE_ATTRIBUTES && (v & FILE_ATTRIBUTE_DIRECTORY)) {
			_tcscpy (xstart_path_new2, start_path_new2);
			_tcscpy (xstart_path_new2, _T("WinUAE\\"));
			af_path_2005 |= 2;
		}
	}

	if (SUCCEEDED (SHGetFolderPath (NULL, CSIDL_COMMON_DOCUMENTS, NULL, SHGFP_TYPE_CURRENT, tmp))) {
		fixtrailing (tmp);
		_tcscpy (tmp2, tmp);
		_tcscat (tmp2, _T("Amiga Files\\"));
		_tcscpy (tmp, tmp2);
		_tcscat (tmp, _T("WinUAE"));
		v = GetFileAttributes (tmp);
		if (v != INVALID_FILE_ATTRIBUTES && (v & FILE_ATTRIBUTE_DIRECTORY)) {
			TCHAR *p;
			_tcscpy (xstart_path_new1, tmp2);
			_tcscat (xstart_path_new1, _T("WinUAE\\"));
			_tcscpy (xstart_path_uae, start_path_exe);
			_tcscpy (start_path_new1, xstart_path_new1);
			p = tmp2 + _tcslen (tmp2);
			_tcscpy (p, _T("System"));
			if (isfilesindir (tmp2)) {
				af_path_2005 |= 1;
			} else {
				_tcscpy (p, _T("Shared"));
				if (isfilesindir (tmp2)) {
					af_path_2005 |= 1;
				}
			}
		}
	}
#endif

	if (start_data == 0) {
		start_data = 1;
		if (path_type == PATH_TYPE_WINUAE && xstart_path_uae[0]) {
			_tcscpy (start_path_data, xstart_path_uae);
		} else if (path_type == PATH_TYPE_NEWWINUAE && xstart_path_new1[0]) {
			_tcscpy (start_path_data, xstart_path_new1);
			create_afnewdir (0);
		} else if (path_type == PATH_TYPE_NEWAF && (af_path_2005 & 1) && xstart_path_new1[0]) {
			_tcscpy (start_path_data, xstart_path_new1);
			create_afnewdir (0);
		} else if (path_type == PATH_TYPE_AMIGAFOREVERDATA && (af_path_2005 & 2) && xstart_path_new2[0]) {
			_tcscpy (start_path_data, xstart_path_new2);
		} else if (path_type == PATH_TYPE_DEFAULT) {
			_tcscpy (start_path_data, xstart_path_uae);
			if (af_path_2005 & 1) {
				path_type = PATH_TYPE_NEWAF;
				create_afnewdir (1);
				_tcscpy (start_path_data, xstart_path_new1);
			}
			if (af_path_2005 & 2) {
				_tcscpy (tmp, xstart_path_new2);
				_tcscat (tmp, _T("system\\rom"));
#if 0
				if (isfilesindir (tmp)) {
					path_type = PATH_TYPE_AMIGAFOREVERDATA;
				} else {
#endif
					_tcscpy (tmp, xstart_path_new2);
					_tcscat (tmp, _T("shared\\rom"));
#if 0
					if (isfilesindir (tmp)) {
						path_type = PATH_TYPE_AMIGAFOREVERDATA;
					} else {
#endif
						path_type = PATH_TYPE_NEWWINUAE;
#if 0
					}
				}
#endif
				_tcscpy (start_path_data, xstart_path_new2);
			}
		}
	}

	v = GetFileAttributes (start_path_data);
	if (v == INVALID_FILE_ATTRIBUTES || !(v & FILE_ATTRIBUTE_DIRECTORY) || start_data == 0 || start_data == -2) {
		_tcscpy (start_path_data, start_path_exe);
	}
	fixtrailing (start_path_data);
	fullpath (start_path_data, sizeof start_path_data / sizeof (TCHAR));
	SetCurrentDirectory (start_path_data);

	// use path via command line?
	if (!start_path_plugins[0]) {
		// default path
		_tcscpy (start_path_plugins, start_path_data);
		_tcscat (start_path_plugins, WIN32_PLUGINDIR);
	}
	v = GetFileAttributes (start_path_plugins);
	if (!start_path_plugins[0] || v == INVALID_FILE_ATTRIBUTES || !(v & FILE_ATTRIBUTE_DIRECTORY)) {
		// not found, exe path?
		_tcscpy (start_path_plugins, start_path_exe);
		_tcscat (start_path_plugins, WIN32_PLUGINDIR);
		v = GetFileAttributes (start_path_plugins);
		if (v == INVALID_FILE_ATTRIBUTES || !(v & FILE_ATTRIBUTE_DIRECTORY))
			_tcscpy (start_path_plugins, start_path_data); // not found, very default path
	}
	fixtrailing (start_path_plugins);
	fullpath (start_path_plugins, sizeof start_path_plugins / sizeof (TCHAR));
	setpathmode (path_type);
#endif
}

/************************************************************************ 
 * makeverstr
 * 
 * create a nice version
 ************************************************************************/
void makeverstr (TCHAR *s) {

	if (_tcslen (WINUAEBETA) > 0) {
		_stprintf (BetaStr, " (%sBeta %s, %s)", WINUAEPUBLICBETA > 0 ? "Public " : "", WINUAEBETA, __DATE__);
		_stprintf (s, "Janus-UAE2%s%d.%d.%d%s%s",
#if (SIZEOF_VOID_P == 8)
                        " 64bit ",
#else
                        " ",
#endif
			UAEMAJOR, UAEMINOR, UAESUBREV, WINUAEREV, BetaStr);
	} else {
		_stprintf (s, "Janus-UAE2%s%d.%d.%d%s (%s)",
#if (SIZEOF_VOID_P == 8)
                        " 64bit ",
#else
                        " ",
#endif
			UAEMAJOR, UAEMINOR, UAESUBREV, WINUAEREV, __DATE__);
	}
	if (_tcslen (WINUAEEXTRA) > 0) {
		_tcscat (s, " ");
		_tcscat (s, WINUAEEXTRA);
	}
}

extern int log_scsi;

static TCHAR *getdefaultini (void) {
  return strdup("PROGDIR:winuae.ini");
}
/************************************************************************ 
 * main
 *
 * the AROS main must init / deinit all stuff around real_main
 *
 * This is the equivalent to WinMain2 in win32.cpp (hopefully)
 ************************************************************************/
void WIN32_HandleRegistryStuff (void);
void vm_prep(void);
void vm_free(void);

int graphics_setup (void);

#if 0
static int newstack_main (int argc, TCHAR **argv) {
#else
int main (int argc, TCHAR **argv) {
#endif
  ULONG stacksize=0;
  struct Process *proc;
  
  /* we need a big stack size. so check, if we are safe (well, not totally unsafe at least..) */
  proc=(struct Process *)FindTask(NULL); 
  stacksize=(ULONG)((IPTR) proc->pr_Task.tc_SPUpper - (IPTR) proc->pr_Task.tc_SPLower);
  DebOut("stacksize: %d\n", stacksize);

  if(stacksize < 499999) {
    char text[256]; /* waist some more stack ;-) */
    snprintf(text, 255, "\nStack size %d is too small. Use at least 500000 bytes.\n", stacksize);
    printf(text);
    MessageBox(NULL, text, "Janus-UAE2 - Fatal Error!", 0);
    exit(1);
  }

#if (UNLOCKTDNESTCNT)
  if(SysBase->TDNestCnt>=0) {
    bug("Permit required a1\n");
    Permit();
    Permit();
    Permit();
  }
#endif

#ifdef UAE_ABI_v0
  vm_prep();
#endif

  inipath=getdefaultini();
  reginitializeinit(&inipath);
  getstartpaths ();
	makeverstr(VersionStr);
	DebOut("%s\n", VersionStr);
	logging_init();
	log_scsi=1;

  write_log("Stack size: %d\n", stacksize);

#if (UNLOCKTDNESTCNT)
  if(SysBase->TDNestCnt>=0) {
    bug("Permit required a2\n");
  }
#endif

#ifndef NATMEM_OFFSET
  if(!init_memory()) {
    write_log(_T("init_memory FAILED\n"));
  }
  else {
    enumeratedisplays (FALSE /* multi_display*/);
  }

#else

  if(preinit_shm() /* && WIN32_RegisterClasses () && WIN32_InitLibraries ()*/ ) {
    write_log (_T("Enumerating display devices.. \n"));
    enumeratedisplays (FALSE /* multi_display*/);
  }
  else {

    write_log(_T("preinit_shm FAILED\n"));
  }
#endif /* NATMEM_OFFSET */

  WIN32_HandleRegistryStuff ();


// warning ====================================================================
  max_z3fastmem=100 * 1024 * 1024;
// warning  max_z3fastmem=100 * 1024 * 1024; !!!!!!!!!!!!!!!!!!!!!!!!!
// warning ====================================================================


#if (UNLOCKTDNESTCNT)
  if(SysBase->TDNestCnt>=0) {
    bug("Permit required a3\n");
  }
#endif

  /* call graphics_setup already here, this fills the screenmodes, we need in sortdisplays.. */
  graphics_setup();
  sortdisplays(); 

  // TODO: Command line parsing here!
  // some process_arg()/argv magic
#if 0
	enumerate_sound_devices ();
	for (i = 0; sound_devices[i].name; i++) {
		int type = sound_devices[i].type;
		write_log (L"%d:%s: %s\n", i, type == SOUND_DEVICE_DS ? L"DS" : (type == SOUND_DEVICE_AL ? L"AL" : (type == SOUND_DEVICE_WASAPI ? L"WA" : L"PA")), sound_devices[i].name);
	}
	write_log (L"Enumerating recording devices:\n");
	for (i = 0; record_devices[i].name; i++) {
		int type = record_devices[i].type;
		write_log (L"%d:%s: %s\n", i, type == SOUND_DEVICE_DS ? L"DS" : (type == SOUND_DEVICE_AL ? L"AL" : (type == SOUND_DEVICE_WASAPI ? L"WA" : L"PA")), record_devices[i].name);
	}
	write_log (L"done\n");
#endif

/* this tries to get the dmDisplayFrequency from Windows, we don't use that for now,
   as only p96 needs it anyways
	memset (&devmode, 0, sizeof (devmode));
	devmode.dmSize = sizeof (DEVMODE);
	if (EnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &devmode)) {
		default_freq = devmode.dmDisplayFrequency;
		if (default_freq >= 70)
			default_freq = 70;
		else
			default_freq = 60;
	}
*/

	//WIN32_InitLang ();
	//WIN32_InitHtmlHelp ();
	//DirectDraw_Release ();

	DebOut("keyboard_settrans ..\n");
	keyboard_settrans ();
#ifdef CATWEASEL
	//catweasel_init ();
#endif
#ifdef PARALLEL_PORT
	//paraport_mask = paraport_init ();
#endif
	//globalipc = createIPC (L"WinUAE", 0);
	//serialipc = createIPC (COMPIPENAME, 1);
	//enumserialports ();
	//enummidiports ();

#if (UNLOCKTDNESTCNT)
  if(SysBase->TDNestCnt>=0) {
    bug("Permit required a4\n");
  }
#endif

	DebOut("calling real_main..\n");
	real_main (argc, argv);

  regflushfile();

#ifdef UAE_ABI_v0
  vm_free();
#endif

  /* every call to a log function will re-init logging. So deinit it as late as possible */
  logging_deinit(); 

	return 0;
}

#if 0
struct StartMessage { 
  struct Message msg; 
  int argc; 
  char **argv; 
  int rc; 
}; 

static void proc_entry () { 

  struct Process *me=(struct Process *)FindTask(NULL); 
  IPTR stacksize=(IPTR)me->pr_Task.tc_SPUpper-(IPTR)me->pr_Task.tc_SPLower; 
  struct MsgPort *my_mp=&me->pr_MsgPort; 
  struct StartMessage *my_msg; 

  DebOut("Stack size: %d\n", (unsigned int) stacksize);
  
  while (!(my_msg = (struct StartMessage *)GetMsg(my_mp))) { 
    WaitPort(my_mp); 
  } 
  
  my_msg->rc=newstack_main(my_msg->argc, my_msg->argv); 
  
  Forbid(); 
  ReplyMsg(&my_msg->msg);
}
#define MIN_STACK 128*1024

int main (int argc, char *argv[]) { 
  struct Process *me=(struct Process *) FindTask(NULL); 
  IPTR stacksize=(IPTR)me->pr_Task.tc_SPUpper-(IPTR)me->pr_Task.tc_SPLower; 
  int rc;

  DebOut("Stack size: %d\n", (unsigned int) stacksize);
  if(stacksize > MIN_STACK) {
    DebOut("-> enough stack\n");
    rc=newstack_main(argc, argv);
  }
  else {
    struct StartMessage msg; 
    struct Process *child_proc; 
    struct MsgPort *my_mp; 
    struct MsgPort *child_mp; 
    struct StartMessage *my_msg; 
    
    DebOut("stack too small for us. Starting child process with bigger stack ..\n"); 
    child_proc = CreateNewProcTags( NP_Entry, proc_entry, 
                                    NP_StackSize, MIN_STACK, 
                                    TAG_END); 

    if (!child_proc) { 
      DebOut("Unable to locate bigger stack!\n");
      fprintf(stderr, "Unable to locate bigger stack!\n"); 
      return RETURN_FAIL; 
    } 
    
    my_mp = &me->pr_MsgPort; 
    child_mp = &child_proc->pr_MsgPort; 
    
    msg.msg.mn_Node.ln_Type = NT_MESSAGE; 
    msg.msg.mn_Length = sizeof(msg); 
    msg.msg.mn_ReplyPort = my_mp; 
    msg.argc = argc; 
    msg.argv = argv; 
    
    PutMsg(child_mp, &msg.msg); 
    while (!(my_msg = (struct StartMessage *)GetMsg(my_mp))) { 
      WaitPort(my_mp); 
    } 
    rc = my_msg->rc; 
  } 
    
  return rc; 
} 
#endif
 
