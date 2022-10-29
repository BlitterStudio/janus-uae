#
# simple Makefile to build SDL_Config link library for AROS
#
# Copyright 2011-2017 Oliver Brunner - aros<at>oliver-brunner.de
#

# to build for 
# AROS ABI v0: ABI=v0
# AROS ABI v1: ABI=v1

OBJDIR  = obj/$(CPU)
# build library in the parent object dir. It is fetched from there during linking 
BIN     = ../$(OBJDIR)/libSDL_Config.a

CPP     = $(SVN)/AROS/bin/$(ARCH)-$(CPU)/tools/crosstools/$(CPU)-aros-c++
CC      = $(SVN)/AROS/bin/$(ARCH)-$(CPU)/tools/crosstools/$(CPU)-aros-gcc
AR      = $(SVN)/AROS/bin/$(ARCH)-$(CPU)/tools/crosstools/$(CPU)-aros-ar

OBJS    = $(OBJDIR)/SDL_config.o 

LINKOBJ = SDL_config.o 
SUB     = AROS/Developer
INCS     = -I$(SVN)/AROS/bin/$(ARCH)-$(CPU)/$(SUB)/include -I$(SVN)/AROS/bin/amiga-$(CPU)/$(SUB)/include

CPPFLAGS= $(INCS) -fexpensive-optimizations -O3
RM      = rm -f


.PHONY: all all-before all-after clean clean-custom

all: prepare $(BIN)

prepare:
	mkdir -p $(OBJDIR)

clean:
	${RM} $(OBJS) $(BIN) $(TESTOBJ)

$(BIN): $(OBJS)
	$(AR) r $(BIN) $(OBJS)

#ranlib $(BIN)

$(OBJDIR)/%.o: %.c
	$(CPP) $(CPPFLAGS) -o $@ -c $*.c
