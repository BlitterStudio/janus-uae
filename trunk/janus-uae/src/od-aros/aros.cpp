#include "sysconfig.h"
#include "sysdeps.h"

#include "od-aros/tchar.h"

/* visual C++ symbols ..*/

int _daylight=0; 
int _dstbias=0 ;
long _timezone=0; 
char tzname[2][4]={"PST", "PDT"};

/* win32.cpp stuff */
int pissoff_value = 25000;

int log_scsi;
int log_net;

int pause_emulation;
int sleep_resolution;
TCHAR start_path_data[MAX_DPATH];
int uaelib_debug;
