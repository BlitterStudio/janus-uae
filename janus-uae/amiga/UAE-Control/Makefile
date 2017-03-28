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

OBJS      = UAE-Control.o uae_pragmas.c

HEADERS    = UAE-Control.h 

all: UAE-Control

UAE-Control: $(OBJS) 
	@rm -f $(TARGETDIR)/UAE-Control UAE-Control
	$(CC) $(OBJS) $(LD_FLAGS) -o UAE-Control 
	cp UAE-Control $(TARGETDIR)
 
clean:
	rm -f *.o UAE-Control $(TARGETDIR)/UAE-Control 

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c $*.c

