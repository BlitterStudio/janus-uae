#
# simple Makefile to build SDL_Config link library for AROS
#
# Copyright 2011-2017 Oliver Brunner - aros<at>oliver-brunner.de
#

# to build for 
# AROS ABI v0: ABI=v0
# AROS ABI v1: ABI=v1


BITS := $(shell getconf LONG_BIT)
ifeq ($(BITS),64)
	# 64 bit
	CPU      = x86_64
	# AROS source svn
	SVN        = /home/oli/tc/svn
else
	# 32 bit
	CPU      = i386
	SVN        = /home/oli/svn
endif

OBJDIR  = obj/$(CPU)
# build library in the parent object dir. It is fetched from there during linking 
BIN     = ../$(OBJDIR)/libSDL_Config.a

CPP     = $(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-c++
CC      = $(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-gcc
AR      = $(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-ar

OBJS    = $(OBJDIR)/SDL_config.o 

LINKOBJ = SDL_config.o 

#INCS    = -I$(SVN)/$(ABI)/AROS/bin/linux-i386/gen/include/SDL
#INCS    = -I$(SVN)/$(ABI)/AROS/bin/linux-i386/AROS/Development/include/SDL
INCS     = -I$(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/AROS/Extras/Developer/include -I$(SVN)/$(ABI)/AROS/bin/amiga-$(CPU)/AROS/Extras/Developer/include

CPPFLAGS= $(INCS) -fexpensive-optimizations -O3
RM      = rm -f


.PHONY: all all-before all-after clean clean-custom

all: prepare $(BIN)

prepare:
	mkdir -p $(OBJDIR)

clean:
	${RM} $(OBJS) $(BIN) $(TESTOBJ)

$(BIN): $(OBJS)
	$(AR) r $(BIN) $(LINKOBJS)

#ranlib $(BIN)

$(OBJDIR)/%.o: %.c
	$(CPP) $(CPPFLAGS) -o $@ -c $*.c
