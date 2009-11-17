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

TARGETDIR = /home/oli/svn/aros/AROS/bin/linux-i386/AROS/m68k/

FLAGS     = -Wall
#CDEFS     = 
CDEFS     = -DDEBUG
INCLUDES  =
DEBUGFLAGS=
LD_FLAGS  = -noixemul

CFLAGS    = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

JOBJS      = janus-daemon.o sync-mouse.o patch.o sync-windows.o sync-screens.o public-screen.o messages.o debug.o
COBJS      = clip-daemon.o debug.o

all: janusd clipd

janusd: $(JOBJS) 
	$(CC) $(LD_FLAGS) $(JOBJS) -o janusd -lauto #-lc -lamiga -lauto
	cp janusd $(TARGETDIR)
 
clipd:  $(COBJS) 
	$(CC) $(LD_FLAGS) $(COBJS) -o clipd 
	cp clipd  $(TARGETDIR)
 
clean:
	@rm -f $(JOBJS) $(COBJS) janusd clipd

%.o: %.c janus-daemon.h clip-daemon.h
	$(CC) $(CFLAGS) -o $@ -c $*.c

