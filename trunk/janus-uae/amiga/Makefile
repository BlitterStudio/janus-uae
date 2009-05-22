#====================================
# simple Makefile for janus-daemon
#====================================

CC        = m68k-amigaos-gcc

TARGETDIR = /home/oli/svn/aros/AROS/bin/linux-i386/AROS/m68k

FLAGS     = -Wall
#CDEFS     = 
CDEFS     = -DDEBUG
INCLUDES  =
DEBUGFLAGS=
LD_FLAGS  = -noixemul

CFLAGS    = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

OBJS      = janus-daemon.o sync-mouse.o patch.o sync-windows.o sync-screens.o public-screen.o messages.o debug.o

all: $(OBJS) 
	$(CC) $(LD_FLAGS) $(OBJS) -o $(TARGETDIR)/janus-daemon 
 
clean:
	@rm -f $(OBJS) $(TARGETDIR)/janus-daemon

%.o: %.c janus-daemon.h
	$(CC) $(CFLAGS) -o $@ -c $*.c

