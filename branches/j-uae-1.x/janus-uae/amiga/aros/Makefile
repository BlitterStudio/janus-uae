#=====================================
# simple Makefile for guest tools
#
# (c) Oliver Brunner 2009-2014
# 
# $Id$
#
# You may want to change the TARGETDIR
# to the path, where you keep your
# AROS/m68k installation tree.
#
#=====================================

CC           = /home/oli/svn/m68k/AROS/bin/linux-i386/tools/crosstools/m68k-aros-gcc
TARGETDIR    = /home/oli/svn/v0/ABI_V0-20161228/AROS/bin/linux-i386/AROS/icaros-aros68k/C

FLAGS     = -Wall -fno-strict-aliasing
CDEFS     = 
#CDEFS     = -DDEBUG
INCLUDES  = -I../include
DEBUGFLAGS=
LD_FLAGS  = 

CFLAGS    = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

JOBJS      = janus-daemon.o sync-mouse.o patch.o sync-windows.o sync-screens.o public-screen.o lock-window.o messages.o debug.o notify.o version.o trap.o
COBJS      = clip-daemon.o debug.o trap.o
LOBJS      = launch-daemon.o debug.o trap.o
SOBJS      = splash.o debug.o
SMOBJS     = clonescreenmode.o

HEADERS    = ../janus-daemon.h ../clip-daemon.h

# only build janusd as AROS executeable. 
# All other daemons should run fine in 68k.
# launchd will not work compiled for AROS (see warning in source)
all: janusd 
	@rm -f version.h

#clipd launchd janus-splash clonescreenmode

janusd: $(JOBJS) 
	@rm -f $(TARGETDIR)/janusd
	$(CC) $(JOBJS) $(LD_FLAGS) -o janusd 
	cp janusd $(TARGETDIR)
 
clipd:  $(COBJS) $(HEADERS)
	$(CC) $(LD_FLAGS) $(COBJS) -o clipd 
	cp clipd  $(TARGETDIR)

launchd:  $(LOBJS) $(HEADERS)
	$(CC) $(LD_FLAGS) $(LOBJS) -o launchd 
	cp launchd  $(TARGETDIR)

janus-splash: $(SOBJS) $(HEADERS)
	$(CC) $(LD_FLAGS) $(SOBJS) -o janus-splash 
	cp janus-splash  $(TARGETDIR)

clonescreenmode: $(SMOBJS) $(HEADERS)
	$(CC) $(LD_FLAGS) $(SMOBJS) -o clonescreenmode 
	cp clonescreenmode  $(TARGETDIR)

clean:
	rm -f *.o janusd $(TARGETDIR)/janusd

# not really nice, but working ;)
# generate version.h for every build
version.h: 
	@echo '#define LIB_DATE      "'`date +%d.%m.%Y`'"'                                 >version.h
	@echo '#define LIB_CPU       "[AROS/x86]"'                                        >>version.h
	@echo '#define LIB_VER       "1.4"'                                               >>version.h
	@echo '#define LIB_COPYRIGHT "(C) 2014 Oliver Brunner, GPL"' >>version.h

version.o: version.h
	 $(CC) $(CFLAGS) -o $@ -c $*.c

# build notify with -O0, as gcc might use registers differently otherwise
notify.o: ../notify.c  $(HEADERS)
	$(CC) $(CFLAGS) -O0 -Wall -o $@ -c ../$*.c

%.o: ../%.c  $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c ../$*.c

