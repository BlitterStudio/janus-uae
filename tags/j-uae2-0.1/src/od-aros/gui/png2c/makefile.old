#
# Makefile for png2c
# (c) 2001 by David Gerber <zapek@meanmachine.ch>
#
# $Id: makefile,v 1.6 2002/11/29 10:41:11 zapek Exp $

ifeq ($(shell $(CC) -dumpmachine), ppc-morphos)
BUILD=MORPHOS
else
BUILD=UNIX
endif

OBJS = png2c.o	
CFLAGS = -O2
LDFLAGS = -s
ifeq ($(BUILD),MORPHOS)
CFLAGS += -mcpu=604e -fomit-frame-pointer -mmultiple
LDFLAGS += -noixemul
endif
EXE = png2c

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(EXE) -lpng -lz

clean:
	-rm -f *.o $(EXE)

archive: clean
	-rm -rf png2c
	-rm png2c.tar.gz
	mkdir -p png2c
	cp png2c.c INSTALL README LICENSE makefile png2c
	tar -czvf png2c.tar.gz png2c
	-rm -rf png2c
