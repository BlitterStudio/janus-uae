#=====================================
# simple Makefile for amirun
#
# (c) Oliver Brunner 2010
# 
# $Id$
#
# You may want to change the TARGETDIR
# to the path, where you keep your
# aros installation tree.
#=====================================

CC        = i686-aros-gcc

TARGETDIR = /home/oli/svn/aros/AROS/bin/linux-i386/AROS/C/

FLAGS     = -Wall
#CDEFS     = 
CDEFS     = -DDEBUG
INCLUDES  =
DEBUGFLAGS=
LD_FLAGS  = 

CFLAGS    = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

OBJS      = amirun.o

all: amirun

amirun: $(OBJS)
	$(CC) $(LD_FLAGS) $(OBJS) -o amirun 
	cp amirun $(TARGETDIR)
 
clean:
	@rm -f $(OBJS) amirun

#janus-daemon.h clip-daemon.h
%.o: %.c 
	$(CC) $(CFLAGS) -o $@ -c $*.c

