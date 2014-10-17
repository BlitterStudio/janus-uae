#
# simple Makefile for uae MUI gui
#
# Copyright 2012 Oliver Brunner - aros<at>oliver-brunner.de
#
# This file is part of Janus-UAE.
#
# Janus-UAE is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Janus-UAE is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
#

# to build for 
# AROS ABI v0: ABI=v0
# AROS ABI v1: ABI=v1
ABI        = v0
SVN        = /home/oli/svn

EXE        = $(SVN)/$(ABI)/AROS/bin/linux-i386/AROS/gui

CC         = $(SVN)/$(ABI)/AROS/bin/linux-i386/tools/crosstools/i386-aros-gcc
CPP        = $(SVN)/$(ABI)/AROS/bin/linux-i386/tools/crosstools/i386-aros-g++
#RANLIB = 
AR         = $(SVN)/$(ABI)/AROS/bin/linux-i386/tools/crosstools/i386-aros-ar
LD         = $(SVN)/$(ABI)/AROS/bin/linux-i386/tools/crosstools/i386-aros-ld

# suppress warning: deprecated conversion from string constant to ‘char*’
FLAGS      = -Wno-write-strings -O2 #-Wall
DEBUGFLAGS = -g

CDEFS   =
INCLUDES= -I. -I.. -I../../include -I../../md-i386-gcc  -I/home/oli/svn/v0/AROS/bin/linux-i386/gen/include/SDL

CFLAGS = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)
CPPFLAGS = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

LINK_LIBS=-larossupport -lmui

# ============== common ====================

COMMON_OBJS= \
	mui_head.o \
	mui_class.o

	#../picasso96.o \
	#../ersatz.o \
	#keyboard.o \
#../zfile_archive.o 
#gui.o \

# build dependencies magic
DEP_OBJS=$(COMMON_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $*.c

%.o: %.cpp
	@echo "building dependency for "$(subst .o,.cpp,$<)" .."
	@$(CPP) -M $(CPPFLAGS) $(subst .o,.cpp,$<) > $@.$$$$;                      \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $(subst .o,.dep,$@);     \
	rm -f $@.$$$$
	$(CPP) $(CPPFLAGS) -o $@ -c $*.cpp


all: uae

uae: common 
	$(CPP) $(CFLAGS) $(COMMON_OBJS) -o $(EXE) $(LINK_LIBS)


common: $(COMMON_OBJS)
	@echo building common ..

aros: $(AROS_OBJS)
	@echo building aros objects ..
	
# ============== autogenerate some .c files ====================

# ============== clean ====================
clean: 
	rm -f $(COMMON_OBJS) $(EXE)


# ============== dependencies ====================

-include $(subst .o,.dep,$(DEP_OBJS))

