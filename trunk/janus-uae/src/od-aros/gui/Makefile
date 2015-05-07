#
# simple Makefile for a Zune gui
#
# Copyright 2014 Oliver Brunner - aros<at>oliver-brunner.de
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
#ABI        = v0

SVN        = /home/oli/svn

LIB        = libgui.a

CC         = $(SVN)/$(ABI)/AROS/bin/linux-i386/tools/crosstools/i386-aros-gcc
CPP        = $(SVN)/$(ABI)/AROS/bin/linux-i386/tools/crosstools/i386-aros-g++
#RANLIB = 
AR         = $(SVN)/$(ABI)/AROS/bin/linux-i386/tools/crosstools/i386-aros-ar
LD         = $(SVN)/$(ABI)/AROS/bin/linux-i386/tools/crosstools/i386-aros-ld

# suppress warning: deprecated conversion from string constant to ‘char*’
FLAGS      = -Wno-write-strings -O2 #-Wall
ARFLAGS    = rc
DEBUGFLAGS = -g

CDEFS   = -DUAE_ABI_$(ABI)
INCLUDES= -I. -I.. -I../../include -I../../md-i386-gcc  -I/home/oli/svn/v0/AROS/bin/linux-i386/gen/include/SDL

CFLAGS = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)
CPPFLAGS = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

LINK_LIBS=-larossupport -lmui

# ============== common ====================

COMMON_OBJS= \
	mui_head.o \
	mui_data.o \
	mui_file_request.o \
	combo.o \
	mui_class.o 

IMAGES =

	#images/35floppy.o \
	#images/amigainfo.o \
	#images/avioutput.o \
	#images/chip.o \
	#images/configfile.o \
	#images/cpu.o \
	#images/diskimage.o \
	#images/drive.o \
	#images/expansion.o \
	#images/file.o \
	#images/folder.o \
	#images/joystick.o \
	#images/misc.o \
	#images/move_dow.o \
	#images/move_up.o \
	#images/paths.o \
	#images/port.o \
	#images/quickstart.o \
	#images/root.o \
	#images/screen.o \
	#images/sound.o \
	#images/winuae.o


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

uae: mui_data.h common images
	$(AR) $(ARFLAGS) $(LIB) $(COMMON_OBJS) $(IMAGES)  


common: $(COMMON_OBJS)
	@echo building common ..

images: $(IMAGES)
	@echo building images ..

aros: $(AROS_OBJS)
	@echo building aros objects ..

tools:
	@echo builing tools ..
	cd png2c;export CC=gcc;make

#for i in $(IMAGES);do echo " $(i)  .."; icotool -i 1 -x $i;done
	
# ============== autogenerate some .cpp files ====================
# never know, which one needs to be generated first. But only one
# is actually executed as the other condition is already fulfilled.
mui_data.cpp: ../../od-win32/resources/winuae.rc parse_fixed.pl
	./parse_fixed.pl ../../od-win32/resources/winuae.rc mui_data.cpp

mui_data.h: ../../od-win32/resources/winuae.rc parse_fixed.pl
	./parse_fixed.pl ../../od-win32/resources/winuae.rc mui_data.cpp


# ============== clean ====================
clean: 
	rm -f $(COMMON_OBJS) $(LIB)
	cd png2c;make clean


# ============== dependencies ====================

-include $(subst .o,.dep,$(DEP_OBJS))

