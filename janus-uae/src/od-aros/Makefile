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
# AROS ABI v0: ABI=v0
# AROS ABI v1: ABI=v1
#ABI        = v0
#
# ABI is usually set in parent Makefile
# => so do not use this Makefile directly!


BITS := $(shell getconf LONG_BIT)
ifeq ($(BITS),64)
	# 64 bit
	CPU      = x86_64
	# AROS source svn
	SVN        = /home/oli/tc/svn
	# SDL is somewhere else !?
	SUB        = AROS/Development
else
	# 32 bit
	CPU      = i386
	SVN        = /home/oli/svn
	SUB        = gen
endif

EXE        = $(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/AROS/wuae

CC         = $(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-gcc
CPP        = $(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-g++
#RANLIB = 
AR         = $(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-ar
LD         = $(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-ld

# suppress warning: deprecated conversion from string constant to ‘char*’
FLAGS      = -Wno-write-strings -O2 #-Wall
DEBUGFLAGS = -g

CDEFS   = -DUAE_ABI_$(ABI)
INCLUDES= -I. -I../include -I../md-i386-gcc -I.. -I$(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/$(SUB)/include/SDL 

#-I/home/oli/svn/$(ABI)/AROS/bin/linux-$(CPU)/AROS/Development/include/SDL

CFLAGS = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)
CPPFLAGS = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

V0_LIBS  =

ifeq ($(ABI), v0)
V0_LIBS=-lGL
endif

LINK_LIBS=-Lgui -lgui -limagedata -larossupport -lz -lGL -lmui -lSDL -LSDL_Config -lSDL_Config $(V0_LIBS)
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
	thread.o \
	../blkdev.o \
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
	../bsdsocket.o \
	../calc.o \
	../catweasel.o \
	../cd32_fmv.o \
	../cdrom.o \
	../cdtv.o \
	../cfgfile.o \
	../cia.o \
	../consolehook.o \
	../cpudefs.o \
	../cpuemu_0.o \
	../cpuemu_11.o \
	../cpuemu_13.o \
	../cpuemu_20.o \
	../cpuemu_21.o \
	../cpuemu_22.o \
	../cpuemu_31.o \
	../cpuemu_32.o \
	../cpuemu_33.o \
	../cpummu.o \
	../cpummu30.o \
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
	../ethernet.o \
	../events.o \
	../expansion.o \
	../fdi2raw.o \
	../filesys.o \
	../fpp.o \
	../fsdb.o \
	../fsusage.o \
	../gayle.o \
	../gfxboard.o \
	../qemuvga/cirrus_vga.o \
	../qemuvga/qemuuaeglue.o \
	../qemuvga/vga.o \
	../qemuvga/lsi53c895a.o \
	../gfxutil.o \
	../hardfile.o \
	../hrtmon.rom.o \
	../identify.o \
	../inputdevice.o \
	../inputrecord.o \
	../keybuf.o \
	../main.o \
	../memory.o \
	../native2amiga.o \
	../ncr_scsi.o \
	../newcpu.o \
	../newcpu_common.o \
	../rommgr.o \
	../readcpu.o \
	../sana2.o \
	../savestate.o \
	../scsi.o \
	../scsitape.o \
	../scsiemul.o \
	../specialmonitors.o \
	../statusline.o \
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
	../isofs.o \
	../zfile.o \
	../zfile_archive.o \
	main_aros.o \
	avioutput.o \
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
	mman.o \
	writelog.o \
	keymap.o \
	shm.o \
	posixemu.o \
	sdlgfx.o \
	sdlkeys_dik.o \
	rp.o \
	registry.o \
	muigui.o \
	extensions.o \
	fsdb_aros.o \
	nogui_aros.o \
	hardfile_aros.o \
	stubs.o

	#../picasso96.o \
	#../ersatz.o \
	#keyboard.o \
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

uae: string_resource.h sdl_config gui common aros
	$(CPP) $(CFLAGS) $(COMMON_OBJS) $(AROS_OBJS) -o $(EXE) $(LINK_LIBS) 

sdl_config:
	@echo building SDL_Config library ..
	@cd SDL_Config;make ABI="$(ABI)"

common: ../blitfunc.cpp ../blittable.cpp ../blit.h ../linetoscr.cpp $(COMMON_OBJS)
	@echo building common ..

aros: $(AROS_OBJS)
	@echo building aros objects for ABI $(ABI) ..

.PHONY: gui
gui: 
	@echo building gui library ..
	@cd gui;make ABI="$(ABI)"
	
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

string_resource.h: ../od-win32/resources/winuae.rc
		./parse_res.pl ../od-win32/resources/winuae.rc string_resource.h

# ============== clean ====================
clean: 
	rm -f $(COMMON_OBJS) $(EXE)
	@cd gui;make clean ABI=$(ABI)
	@cd SDL_Config;make clean ABI=$(ABI)


# ============== dependencies ====================

nogui.o: string_resource.h
-include $(subst .o,.dep,$(DEP_OBJS))

