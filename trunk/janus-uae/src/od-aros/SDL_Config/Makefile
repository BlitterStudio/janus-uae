#
# simple Makefile to build SDL_Config link library for AROS
#
# Copyright 2011 Oliver Brunner - aros<at>oliver-brunner.de
#

# to build for 
# AROS ABI v0: ABI=v0
# AROS ABI v1: ABI=v1

SVN     = /home/oli/svn

CPP     = $(SVN)/$(ABI)/AROS/bin/linux-i386/tools/crosstools/i386-aros-g++
CC      = $(SVN)/$(ABI)/AROS/bin/linux-i386/tools/crosstools/i386-aros-gcc
AR      = $(SVN)/$(ABI)/AROS/bin/linux-i386/tools/crosstools/i386-aros-ar

OBJS    = SDL_config.o 
TESTOBJ = initest.o
TESTEXE = $(SVN)/$(ABI)/AROS/bin/linux-i386/AROS/initest
LINKOBJ = SDL_config.o 
#INCS    = -I$(SVN)/$(ABI)/AROS/bin/linux-i386/gen/include/SDL
#INCS    = -I$(SVN)/$(ABI)/AROS/bin/linux-i386/AROS/Development/include/SDL
INCS =

BIN     = libSDL_Config.a
CPPFLAGS= $(INCS) -fexpensive-optimizations -O3
RM      = rm -f


.PHONY: all all-before all-after clean clean-custom

all: $(BIN)

test: $(BIN) $(TESTOBJ)
	$(CPP) $(CPPFLAGS) $(TESTOBJ) -L. -lSDL_Config -lSDL -o $(TESTEXE) 

clean:
	${RM} $(OBJS) $(BIN) $(TESTOBJ)

$(BIN): $(LINKOBJ)
	$(AR) r $(BIN) $(LINKOBJ)

#ranlib $(BIN)

%.o: %.c
	$(CPP) $(CPPFLAGS) -o $@ -c $*.c
