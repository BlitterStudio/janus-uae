#
# simple Makefile for uae AROS sources
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
# AROS ABI v0: ABI=AROS_v0
# AROS ABI v1: ABI=AROS
ABI        = AROS_v0
SVN        = /home/oli/svn

EXE        = $(SVN)/aros/$(ABI)/bin/linux-i386/AROS/wuae

CC         = $(SVN)/aros/$(ABI)/bin/linux-i386/tools/crosstools/i386-aros-gcc
CPP        = $(SVN)/aros/$(ABI)/bin/linux-i386/tools/crosstools/i386-aros-g++
#RANLIB = 
AR         = $(SVN)/aros/$(ABI)/bin/linux-i386/tools/crosstools/i386-aros-ar
LD         = $(SVN)/aros/$(ABI)/bin/linux-i386/tools/crosstools/i386-aros-ld

FLAGS      = -O2 #-Wall
DEBUGFLAGS = -g

CDEFS   =
INCLUDES= -I. -I../include -I../md-i386-gcc -I..

CFLAGS = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)
CPPFLAGS = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

LINK_LIBS=-larossupport -lz 

# ============== common ====================

AROS_OBJS= 

	#clipboard_aros.o \
	#ahidsound_dsonly.o \
	#ahidsound_new.o \
	#unicode.o \
	#writelog.o \
	#mman.o \
	#nogui.o \
	#aros.o \
	#scaler_more.o

COMMON_OBJS= \
	aros.o \
	../a2065.o \
	../a2091.o \
	../akiko.o \
	../amax.o \
	../ar.o \
	../arcadia.o \
	../audio.o \
	../autoconf.o \
	../blitfunc.o \
	../blittable.o \
	../blitter.o \
	../blkdev.o \
	../bsdsocket.o \
	../catweasel.o \
	../cd32_fmv.o \
	../cdrom.o \
	../cdtv.o \
	../cfgfile.o \
	../cia.o \
	../cpudefs.o \
	../cpuemu_0.o \
	../cpuemu_11.o \
	../cpuemu_12.o \
	../cpuemu_21.o \
	../cpuemu_20.o \
	../cpuemu_31.o \
	../cpummu.o \
	../cpustbl.o \
	../crc32.o \
	../custom.o \
	../debug.o \
	../disk.o \
	../diskutil.o \
	../dongle.o \
	../drawing.o \
	../driveclick.o \
	../enforcer.o \
	../epsonprinter.o \
	../ersatz.o \
	../expansion.o \
	../fdi2raw.o \
	../filesys.o \
	../fpp.o \
	../fsdb.o \
	../fsusage.o \
	../gayle.o \
	../gfxutil.o \
	../hardfile.o \
	../identify.o \
	../inputdevice.o \
	../inputrecord.o \
	../keybuf.o \
	../main.o \
	../memory.o \
	../native2amiga.o \
	../ncr_scsi.o \
	../newcpu.o \
	../picasso96.o \
	../rommgr.o \
	../readcpu.o \
	../sana2.o \
	../savestate.o \
	../scsi.o \
	../scsiemul.o \
	../traps.o \
	../uaeexe.o \
	../uaeipc.o \
	../uaelib.o \
	../uaeresource.o \
	../uaeserial.o \
	../jit/compemu.o \
	../jit/compemu_fpp.o \
	../jit/compemu_support.o \
	../jit/compstbl.o \
	../nogui.o \
	../zfile.o \
	main.o \
	unicode.o \
	sounddep/sound.o \
	serial.o \
	joystick.o \
	mouse.o \
	scaler_more.o \
	arosgfx.o \
	aros_scaler.o \
	parser.o \
	picasso96_aros.o \
	options.o \
	gfx.o \
	thread.o \
	nogui.o \
	fsdb.o \
	mman.o \
	writelog.o \
	keyboard.o \
	stubs.o

#../zfile_archive.o 
#gui.o \

# build dependencies magic
DEP_OBJS=$(COMMON_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $*.c

%.o: %.cpp
	@echo "building dependency .." $(subst .o,.cpp,$<)
	@$(CPP) -M $(CPPFLAGS) $(subst .o,.cpp,$<) > $@.$$$$;                      \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;     \
	rm -f $@.$$$$
	$(CPP) $(CPPFLAGS) -o $@ -c $*.cpp


all: uae

uae: common aros
	$(CPP) $(CFLAGS) $(COMMON_OBJS) $(AROS_OBJS) -o $(EXE) $(LINK_LIBS)


common: ../blitfunc.cpp ../blittable.cpp ../blit.h ../linetoscr.cpp $(COMMON_OBJS)
	@echo building common ..

aros: $(AROS_OBJS)
	@echo building aros objects ..
	
# ============== autogenerate some .c files ====================

../blit.h:	../genblitter
		../genblitter i >../blit.h

../blitfunc.cpp: 	../genblitter ../blitfunc.h
		../genblitter f >../blitfunc.cpp

../blitfunc.h: 	../genblitter
		../genblitter h >../blitfunc.h

../blittable.cpp:	../genblitter ../blitfunc.h
		../genblitter t >../blittable.cpp

../linetoscr.cpp:    ../genlinetoscr
		../genlinetoscr $(genlinetoscr_args) >../linetoscr.cpp

../jit/compemu.cpp: ../gencomp
		cd ..;./gencomp

../cpuemu_0.cpp: ../gencpu
		cd ..;./gencpu

# ============== clean ====================
clean: 
	rm -f $(COMMON_OBJS)


# ============== dependencies ====================
-include $(subst .o,.dep,$(DEP_OBJS))

