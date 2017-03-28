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

CC           = /usr/local/amiga/bin/m68k-amigaos-gcc
TARGETDIR    = /home/oli/svn/v0/ABI_V0-20161228/AROS/bin/linux-i386/AROS/m68k/

FLAGS     = -Wall
CDEFS     = 
#CDEFS     = -DDEBUG
INCLUDES  =
DEBUGFLAGS=
LD_FLAGS  =  -noixemul

CFLAGS    = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

JOBJS      = janus-daemon.o sync-mouse.o patch.o sync-windows.o sync-screens.o public-screen.o lock-window.o messages.o debug.o notify.o trap.o
COBJS      = clip-daemon.o debug.o trap.o
LOBJS      = launch-daemon.o debug.o trap.o
SOBJS      = splash.o debug.o
SMOBJS     = clonescreenmode.o

HEADERS    = ../janus-daemon.h ../clip-daemon.h

all: janusd clipd launchd janus-splash clonescreenmode

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
	rm -f *.o janusd $(TARGETDIR)/janusd clipd $(TARGETDIR)/clipd launchd $(TARGETDIR)/launchd janus-splash $(TARGETDIR)/janus-splash clonescreenmode $(TARGETDIR)/clonescreenmode

%.o: ../%.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c ../$*.c

