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
#=====================================

CC        = m68k-amigaos-gcc

TARGETDIR = /home/oli/svn/v0/AROS/bin/linux-i386/AROS/m68k/

FLAGS     = -Wall
CDEFS     = 
#CDEFS     = -DDEBUG
INCLUDES  =
DEBUGFLAGS=
LD_FLAGS  = -noixemul

CFLAGS    = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

JOBJS      = janus-daemon.o sync-mouse.o patch.o sync-windows.o sync-screens.o public-screen.o lock-window.o messages.o debug.o
COBJS      = clip-daemon.o debug.o
LOBJS      = launch-daemon.o debug.o
SOBJS      = splash.o debug.o

all: janusd clipd launchd janus-splash

janusd: $(JOBJS) 
	$(CC) $(LD_FLAGS) $(JOBJS) -o janusd -lauto #-lc -lamiga -lauto
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


clean:
	@rm -f $(JOBJS) $(COBJS) $(LOBJS) janusd clipd launchd

%.o: %.c janus-daemon.h clip-daemon.h
	$(CC) $(CFLAGS) -o $@ -c $*.c

