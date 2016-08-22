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
	SUB        = AROS/Development
endif

EXE        = $(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/AROS/wuae

CC         = $(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-gcc
CPP        = $(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-g++
#RANLIB = 
AR         = $(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-ar
LD         = $(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-ld

# suppress warning: deprecated conversion from string constant to ‘char*’
FLAGS      = -Wno-write-strings -O2 -Wall -Wno-unused -Wno-unused-function -Wno-unused-label -Wno-unused-parameter -Wno-unused-value -Wno-unused-variable -Wno-unused-but-set-parameter -Wno-unused-but-set-variable -Wno-sign-compare #-Werror
DEBUGFLAGS = -g

CDEFS   = -DUAE_ABI_$(ABI)
INCLUDES= -I. -I../include -I../md-i386-gcc -I.. -I../archivers/zip -I$(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/$(SUB)/include/SDL -Ilibmpeg2/include

#-I/home/oli/svn/$(ABI)/AROS/bin/linux-$(CPU)/AROS/Development/include/SDL

CFLAGS = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)
CPPFLAGS = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

V0_LIBS  =

ifeq ($(ABI), v0)
V0_LIBS=-lGL
endif

LINK_LIBS=-Lgui -lgui -limagedata -larossupport -lz -lGL -lmui -lSDL -LSDL_Config -lSDL_Config -lpng -Llibmpeg2/libmpeg2/.libs -lmpeg2 -Llibmpeg2/libmpeg2/convert/.libs -lmpeg2convert $(V0_LIBS)
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
	../blkdev_cdimage.o \
	../bsdsocket.o \
	../calc.o \
	../catweasel.o \
	../cd32_fmv.o \
	../cd32_fmv_genlock.o \
	cda_play.o \
	../cdrom.o \
	../cdtv.o \
	../cdtvcr.o \
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
	../cpuemu_23.o \
	../cpuemu_24.o \
	../cpuemu_31.o \
	../cpuemu_32.o \
	../cpuemu_33.o \
	../cpuemu_40.o \
	../cpuemu_50.o \
	../cpummu.o \
	../cpummu30.o \
	../cpustbl.o \
	../cpuboard.o \
	../x86.o \
	../crc32.o \
	../custom.o \
	../debug.o \
	../def_icons.o \
	../devices.o \
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
	../fake86_cpu.o \
	../fdi2raw.o \
	../filesys.o \
	../flashrom.o \
	filesys.o \
	../fpp.o \
	../fsdb.o \
	../fsusage.o \
	../gayle.o \
	../gfxboard.o \
	../ide.o \
	../qemuvga/cirrus_vga.o \
	../qemuvga/qemuuaeglue.o \
	../qemuvga/vga.o \
	../qemuvga/lsi53c895a.o \
	../qemuvga/lsi53c710.o \
	../qemuvga/es1370.o \
	../qemuvga/ne2000.o \
	../qemuvga/esp.o \
	../gfxutil.o \
	../hardfile.o \
	../hrtmon.rom.o \
	../identify.o \
	../idecontrollers.o \
	../inputdevice.o \
	../inputrecord.o \
	../keybuf.o \
	../main.o \
	../mame/a2410.o \
	../mame/tm34010/tms34010.o \
	../memory.o \
	../native2amiga.o \
	../ncr_scsi.o \
	../ncr9x_scsi.o \
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
	../test_card.o \
	../traps.o \
	../uaeexe.o \
	../uaeipc.o \
	../uaelib.o \
	../uaeresource.o \
	../uaeserial.o \
	../isofs.o \
	../jit/compemu.o \
	../jit/compemu_fpp.o \
	../jit/compemu_support.o \
	../jit/compstbl.o \
	../pci.o \
	../zfile.o \
	../zfile_archive.o \
	../archivers/zip/zip.o \
	../archivers/zip/ioapi.o \
	../archivers/mp2/kjmp2.o \
	../dosbox/cmos.o \
	../dosbox/core_full.o \
	../dosbox/core_normal.o \
	../dosbox/core_simple.o \
	../dosbox/cpu.o \
	../dosbox/db_memory.o \
	../dosbox/flags.o \
	../dosbox/fpu.o \
	../dosbox/keyboard.o \
	../dosbox/modrm.o \
	../dosbox/paging.o \
	../dosbox/pic.o \
	../dosbox/timer.o \
	../ppc/ppc.o \
	../ppc/ppcd.o \
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
	memory_aros.o \
	writelog.o \
	keymap.o \
	shm.o \
	posixemu.o \
	sdlgfx.o \
	sdlkeys_dik.o \
	rp.o \
	registry.o \
	muigui_helper.o \
	muigui.o \
	extensions.o \
	fsdb_aros.o \
	nogui_aros.o \
	hardfile_aros.o \
	screenshot.o \
	vm.o \
	keyboard.o \
	stubs.o

	#../picasso96.o \
	#../ersatz.o \
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

uae: string_resource.h sdl_config_lib libmpeg2 gui common aros
	$(CPP) $(CFLAGS) -static-libstdc++ $(COMMON_OBJS) $(AROS_OBJS) -o $(EXE) $(LINK_LIBS) 

sdl_config_lib:
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
	
libmpeg2: libmpeg2/libmpeg2/.libs/libmpeg2.a libmpeg2/libmpeg2/convert/.libs/libmpeg2convert.a

# generate libmpeg2.a and libmpeg2convert.a
# this is still some magic here, works on some machines, on others build will break here.
# the libraries are not difficult to build, it is harder to integrate them into the build system ;-)
libmpeg2/libmpeg2/.libs/libmpeg2.a:
	@echo building mpeg2 libraries ..
	@cd libmpeg2;export SDLCONFIG="$(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/AROS/Development/bin/sdl-config";./configure --host=$(CPU)-aros
	@cd libmpeg2/libmpeg2/convert;make
	@cd libmpeg2/libmpeg2;make


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

string_resource.h: ../od-win32/resources/winuae.rc ../od-win32/resources/resource.h
		./parse_res.pl ../od-win32/resources/winuae.rc ../od-win32/resources/resource.h string_resource.h 

# ============== clean ====================
clean: 
	rm -f $(COMMON_OBJS) $(EXE)
	# remove dependencies
	rm -f $(subst .o,.dep,$(DEP_OBJS))
	# remove autogenerated files
	rm -f ../blit.h ../blitfunc.cpp ../blitfunc.h ../blittable.cpp ../linetoscr.cpp ../jit/compemu.cpp ../cpuemu_0.cpp string_resource.h
	@cd gui;make clean ABI=$(ABI)
	@cd SDL_Config;make clean ABI=$(ABI)
	# without configure, there might be no Makefile to call
	@cd libmpeg2;./configure --host=$(CPU)-aros
	@cd libmpeg2;make clean

# ============== build package ====================
#package: clean uae
package: uae
	@rm -rf  $(RELEASEDIR)/`date +%Y%m%d`
	@mkdir -p $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/`uname -m`/ABI_$(ABI)
	@cp $(EXE) $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/`uname -m`/ABI_$(ABI)/janus-uae2
	@mkdir -p $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/docs
	@cp docs/* $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/docs/
	@$(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/ilbmtoicon --png icons/j-uae-drawer.info.src icons/j-uae-drawer.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2.info
	@$(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/ilbmtoicon --png icons/j-uae-drawer.info.src icons/j-uae-drawer.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/docs.info
	@$(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/ilbmtoicon --png icons/j-uae-drawer.info.src icons/j-uae-drawer.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/`uname -m`.info
	@$(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/ilbmtoicon --png icons/j-uae-drawer.info.src icons/j-uae-drawer.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/`uname -m`/ABI_$(ABI).info
	@$(SVN)/$(ABI)/AROS/bin/linux-$(CPU)/tools/ilbmtoicon --png icons/j-uae.info.src icons/j-uae.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/`uname -m`/ABI_$(ABI)/janus-uae2.info
	@cd $(RELEASEDIR)/`date +%Y%m%d`;tar cf Janus-UAE2.tar Janus-UAE2 Janus-UAE2.info;gzip Janus-UAE2.tar
	@echo
	@echo Package built at  $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/Janus-UAE2.tar.gz
	@echo

# ============== dependencies ====================
../rommgr.o: string_resource.h
nogui.o: string_resource.h
-include $(subst .o,.dep,$(DEP_OBJS))

