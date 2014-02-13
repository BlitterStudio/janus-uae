#=====================================
# simple Makefile for janusd and clipd
#
# (c) Oliver Brunner 2009
# 
# $Id$
#
# You may want to change the TARGETDIR
# to the path, where you keep your
# amigaOS installation tree.
#
# Use -DENV_AROS to build tools specially
# optimized for AROS. They are still
# 68k hunk files, though. (TODO)
#=====================================

#CC        = m68k-amigaos-gcc
CC        = /home/oli/svn/m68k/AROS/bin/linux-i386/tools/crosstools/m68k-aros-gcc

#TARGETDIR = /home/oli/svn/v0/AROS/bin/linux-i386/AROS/m68k/
TARGETDIR = /home/oli/svn/v0/AROS/bin/linux-i386/AROS/icaros-aros68k/C

FLAGS     = -Wall
CDEFS     = -DENV_AROS
#CDEFS     = -DDEBUG
INCLUDES  =
DEBUGFLAGS=
#LD_FLAGS  = -noixemul
LD_FLAGS  = 

CFLAGS    = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

JOBJS      = janus-daemon.o sync-mouse.o patch.o sync-windows.o sync-screens.o public-screen.o lock-window.o messages.o debug.o
COBJS      = clip-daemon.o debug.o
LOBJS      = launch-daemon.o debug.o
SOBJS      = splash.o debug.o
SMOBJS     = clonescreenmode.o

all: janusd #clipd launchd janus-splash clonescreenmode

janusd: $(JOBJS) 
	$(CC) $(LD_FLAGS) $(JOBJS) -o janusd #-lauto #-lc -lamiga -lauto
	cp janusd $(TARGETDIR)
 
clipd:  $(COBJS) 
	$(CC) $(LD_FLAGS) $(COBJS) -o clipd 
	cp clipd  $(TARGETDIR)

launchd:  $(LOBJS) 
	$(CC) $(LD_FLAGS) $(LOBJS) -o launchd 
	cp launchd  $(TARGETDIR)

janus-splash: $(SOBJS)
	$(CC) $(LD_FLAGS) $(SOBJS) -o janus-splash 
	cp janus-splash  $(TARGETDIR)

clonescreenmode: $(SMOBJS)
	$(CC) $(LD_FLAGS) $(SMOBJS) -o clonescreenmode 
	cp clonescreenmode  $(TARGETDIR)

clean:
	@rm -f $(JOBJS) $(COBJS) $(LOBJS) janusd clipd launchd janus-splash clonescreenmode

%.o: %.c janus-daemon.h clip-daemon.h
	$(CC) $(CFLAGS) -o $@ -c $*.c

