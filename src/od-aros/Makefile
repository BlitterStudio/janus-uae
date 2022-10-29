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


EXE        = $(SVN)/AROS/bin/$(ARCH)-$(CPU)/AROS/wuae

OBJDIR     = obj/$(CPU)
PARENT     = $(OBJDIR)/parent
SUB        = AROS/Developer

CC         = $(SVN)/AROS/bin/$(ARCH)-$(CPU)/tools/crosstools/$(CPU)-aros-gcc
CPP        = $(SVN)/AROS/bin/$(ARCH)-$(CPU)/tools/crosstools/$(CPU)-aros-g++
#RANLIB = 
AR         = $(SVN)/AROS/bin/$(ARCH)-$(CPU)/tools/crosstools/$(CPU)-aros-ar
LD         = $(SVN)/AROS/bin/$(ARCH)-$(CPU)/tools/crosstools/$(CPU)-aros-ld

# suppress warning: deprecated conversion from string constant to ‘char*’
FLAGS      = -Wno-write-strings -O2 -Wall -Wno-unused -Wno-unused-function -Wno-unused-label -Wno-unused-parameter -Wno-unused-value -Wno-unused-variable -Wno-unused-but-set-parameter -Wno-unused-but-set-variable -Wno-sign-compare #-Werror
DEBUGFLAGS = -g #-DJUAE_DEBUG

CDEFS   = -DUAE_ABI_$(ABI) #-DJIT
INCLUDES= -I. -I../include -Iinclude -I../md-i386-gcc -I.. -I../archivers/zip -I$(SVN)/AROS/bin/$(ARCH)-$(CPU)/$(SUB)/include -I$(SVN)/AROS/bin/$(ARCH)-$(CPU)/AROS/Developer/SDK/Extras/include -Ilibmpeg2/include

CFLAGS = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)
CPPFLAGS = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

V0_LIBS  =

ifeq ($(ABI), v0)
V0_LIBS=-lGL
endif

LINK_LIBS=-L$(OBJDIR) -Lgui -lgui -larossupport -lz -lGL -lmui -lSDL -lSDL_Config -lpng -lmpeg2 -lmpeg2convert $(V0_LIBS)
# ============== common ====================

AROS_OBJS= 

COMMON_OBJS= \
	$(OBJDIR)/aros.o \
	$(OBJDIR)/thread.o \
	$(OBJDIR)/cda_play.o \
	$(OBJDIR)/fsusage.o \
	$(OBJDIR)/filesys.o \
	$(OBJDIR)/main_aros.o \
	$(OBJDIR)/avioutput.o \
	$(OBJDIR)/unicode.o \
	$(OBJDIR)/serial.o \
	$(OBJDIR)/joystick.o \
	$(OBJDIR)/mouse.o \
	$(OBJDIR)/scaler_more.o \
	$(OBJDIR)/arosgfx.o \
	$(OBJDIR)/aros_scaler.o \
	$(OBJDIR)/parser.o \
	$(OBJDIR)/picasso96_aros.o \
	$(OBJDIR)/options.o \
	$(OBJDIR)/mman.o \
	$(OBJDIR)/memory_aros.o \
	$(OBJDIR)/writelog.o \
	$(OBJDIR)/keymap.o \
	$(OBJDIR)/shm.o \
	$(OBJDIR)/posixemu.o \
	$(OBJDIR)/sdlgfx.o \
	$(OBJDIR)/sdlkeys_dik.o \
	$(OBJDIR)/rp.o \
	$(OBJDIR)/registry.o \
	$(OBJDIR)/muigui_helper.o \
	$(OBJDIR)/muigui.o \
	$(OBJDIR)/extensions.o \
	$(OBJDIR)/fsdb_aros.o \
	$(OBJDIR)/nogui_aros.o \
	$(OBJDIR)/hardfile_aros.o \
	$(OBJDIR)/screenshot.o \
	$(OBJDIR)/vm.o \
	$(OBJDIR)/keyboard.o \
	$(OBJDIR)/video.o \
	$(OBJDIR)/sound.o \
	$(OBJDIR)/stubs.o \
	$(PARENT)/a2065.o \
	$(PARENT)/a2091.o \
	$(PARENT)/akiko.o \
	$(PARENT)/amax.o \
	$(PARENT)/ar.o \
	$(PARENT)/arcadia.o \
	$(PARENT)/audio.o \
	$(PARENT)/autoconf.o \
	$(PARENT)/blitfunc.o \
	$(PARENT)/blittable.o \
	$(PARENT)/blitter.o \
	$(PARENT)/blkdev.o \
	$(PARENT)/blkdev_cdimage.o \
	$(PARENT)/bsdsocket.o \
	$(PARENT)/calc.o \
	$(PARENT)/catweasel.o \
	$(PARENT)/cd32_fmv.o \
	$(PARENT)/cd32_fmv_genlock.o \
	$(PARENT)/cdrom.o \
	$(PARENT)/cdtv.o \
	$(PARENT)/cdtvcr.o \
	$(PARENT)/cfgfile.o \
	$(PARENT)/cia.o \
	$(PARENT)/consolehook.o \
	$(PARENT)/cpudefs.o \
	$(PARENT)/cpuemu_0.o \
	$(PARENT)/cpuemu_11.o \
	$(PARENT)/cpuemu_13.o \
	$(PARENT)/cpuemu_20.o \
	$(PARENT)/cpuemu_21.o \
	$(PARENT)/cpuemu_22.o \
	$(PARENT)/cpuemu_23.o \
	$(PARENT)/cpuemu_24.o \
	$(PARENT)/cpuemu_31.o \
	$(PARENT)/cpuemu_32.o \
	$(PARENT)/cpuemu_33.o \
	$(PARENT)/cpuemu_40.o \
	$(PARENT)/cpuemu_50.o \
	$(PARENT)/cpummu.o \
	$(PARENT)/cpummu30.o \
	$(PARENT)/cpustbl.o \
	$(PARENT)/cpuboard.o \
	$(PARENT)/x86.o \
	$(PARENT)/crc32.o \
	$(PARENT)/custom.o \
	$(PARENT)/debug.o \
	$(PARENT)/def_icons.o \
	$(PARENT)/devices.o \
	$(PARENT)/disk.o \
	$(PARENT)/diskutil.o \
	$(PARENT)/dongle.o \
	$(PARENT)/drawing.o \
	$(PARENT)/driveclick.o \
	$(PARENT)/enforcer.o \
	$(PARENT)/epsonprinter.o \
	$(PARENT)/ethernet.o \
	$(PARENT)/events.o \
	$(PARENT)/expansion.o \
	$(PARENT)/fake86_cpu.o \
	$(PARENT)/fdi2raw.o \
	$(PARENT)/filesys.o \
	$(PARENT)/flashrom.o \
	$(PARENT)/fpp.o \
	$(PARENT)/fsdb.o \
	$(PARENT)/gayle.o \
	$(PARENT)/gfxboard.o \
	$(PARENT)/ide.o \
	$(PARENT)/gfxutil.o \
	$(PARENT)/hardfile.o \
	$(PARENT)/hrtmon.rom.o \
	$(PARENT)/identify.o \
	$(PARENT)/idecontrollers.o \
	$(PARENT)/inputdevice.o \
	$(PARENT)/inputrecord.o \
	$(PARENT)/keybuf.o \
	$(PARENT)/main.o \
	$(PARENT)/memory.o \
	$(PARENT)/native2amiga.o \
	$(PARENT)/ncr_scsi.o \
	$(PARENT)/ncr9x_scsi.o \
	$(PARENT)/newcpu.o \
	$(PARENT)/newcpu_common.o \
	$(PARENT)/rommgr.o \
	$(PARENT)/readcpu.o \
	$(PARENT)/sana2.o \
	$(PARENT)/savestate.o \
	$(PARENT)/scsi.o \
	$(PARENT)/scsitape.o \
	$(PARENT)/scsiemul.o \
	$(PARENT)/specialmonitors.o \
	$(PARENT)/statusline.o \
	$(PARENT)/test_card.o \
	$(PARENT)/traps.o \
	$(PARENT)/uaeexe.o \
	$(PARENT)/uaeipc.o \
	$(PARENT)/uaelib.o \
	$(PARENT)/uaeresource.o \
	$(PARENT)/uaeserial.o \
	$(PARENT)/isofs.o \
	$(PARENT)/pci.o \
	$(PARENT)/zfile.o \
	$(PARENT)/zfile_archive.o \
	$(PARENT)/qemuvga/cirrus_vga.o \
	$(PARENT)/qemuvga/qemuuaeglue.o \
	$(PARENT)/qemuvga/vga.o \
	$(PARENT)/qemuvga/lsi53c895a.o \
	$(PARENT)/qemuvga/lsi53c710.o \
	$(PARENT)/qemuvga/es1370.o \
	$(PARENT)/qemuvga/ne2000.o \
	$(PARENT)/qemuvga/esp.o \
	$(PARENT)/mame/a2410.o \
	$(PARENT)/jit/compemu.o \
	$(PARENT)/jit/compemu_fpp.o \
	$(PARENT)/jit/compemu_support.o \
	$(PARENT)/jit/compstbl.o \
	$(PARENT)/dosbox/cmos.o \
	$(PARENT)/dosbox/core_full.o \
	$(PARENT)/dosbox/core_normal.o \
	$(PARENT)/dosbox/core_simple.o \
	$(PARENT)/dosbox/cpu.o \
	$(PARENT)/dosbox/db_memory.o \
	$(PARENT)/dosbox/flags.o \
	$(PARENT)/dosbox/fpu.o \
	$(PARENT)/dosbox/keyboard.o \
	$(PARENT)/dosbox/modrm.o \
	$(PARENT)/dosbox/paging.o \
	$(PARENT)/dosbox/pic.o \
	$(PARENT)/dosbox/timer.o \
	$(PARENT)/ppc/ppc.o \
	$(PARENT)/ppc/ppcd.o \
	$(PARENT)/mame/tm34010/tms34010.o \
	$(PARENT)/archivers/zip/zip.o \
	$(PARENT)/archivers/zip/ioapi.o \
	$(PARENT)/archivers/mp2/kjmp2.o 

# build dependencies magic
DEP_OBJS=$(COMMON_OBJS)

# build source files in current dir
$(OBJDIR)/%.o: %.c
	@echo "building $@ .."
	$(CC) $(CFLAGS) -o $@ -c $*.c

#build winuae sources from parent dir
$(PARENT)/%.o: ../%.cpp
	@echo "building dependency for "$(subst .o,.cpp,$<)" .."
	@$(CPP) -M $(CPPFLAGS) $(subst .o,.cpp,$<) > $@.$$$$;                      \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $(subst .o,.dep,$@);     \
	rm -f $@.$$$$
	$(CPP) $(CPPFLAGS) -o $@ -c ../$*.cpp

$(OBJDIR)/%.o: %.cpp
	@echo "building dependency for "$(subst .o,.cpp,$<)" .."
	@$(CPP) -M $(CPPFLAGS) $(subst .o,.cpp,$<) > $@.$$$$;                      \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $(subst .o,.dep,$@);     \
	rm -f $@.$$$$
	$(CPP) $(CPPFLAGS) -o $@ -c $*.cpp

all: prepare uae $(COMMON_OBJS)

uae: obj/$(CPU)/libmpeg2.a string_resource.h sdl_config_lib gui common aros 
	$(CPP) $(CFLAGS) -static-libstdc++ $(COMMON_OBJS) $(AROS_OBJS) -o $(EXE) $(LINK_LIBS) 

prepare:
	mkdir -p $(OBJDIR) $(PARENT) $(PARENT)/qemuvga $(PARENT)/mame $(PARENT)/jit $(PARENT)/archivers $(PARENT)/dosbox $(PARENT)/ppc $(PARENT)/mame/tm34010 $(PARENT)/archivers/zip $(PARENT)/archivers/mp2

sdl_config_lib:
	@echo building SDL_Config library ..
	@cd SDL_Config;make SVN=$(SVN) ABI=$(ABI) CPU=$(CPU) ARCH=$(ARCH)

common: ../blitfunc.cpp ../blittable.cpp ../blit.h ../linetoscr.cpp $(COMMON_OBJS)
	@echo building common ..

aros: $(AROS_OBJS)
	@echo building aros objects for ABI $(ABI) ..

.PHONY: gui
gui: 
	@echo CPU: $(CPU)
	@echo ABI: $(ABI)
	@echo ARCH: $(ARCH)
	@echo SVN: $(SVN)
	@echo building gui library SVN $(SVN) ..
	@cd gui;make SVN=$(SVN) ABI=$(ABI) CPU=$(CPU) ARCH=$(ARCH)
	
# generate libmpeg2.a and libmpeg2convert.a
# this is still some magic here, works on some machines, on others build will break here.
# the libraries are not difficult to build, it is harder to integrate them into the build system ;-)
obj/$(CPU)/libmpeg2.a: 
	@echo building mpeg2 libraries ..
#	@cd libmpeg2;make clean
	@cd libmpeg2;PATH=$(PATH):$(SVN)/AROS/bin/linux-x86_64/tools/crosstools;./configure --host=$(CPU)-aros
	@cd libmpeg2/libmpeg2;PATH=$(PATH):$(SVN)/AROS/bin/linux-x86_64/tools/crosstools;make
	#pwd
	mv libmpeg2/libmpeg2/convert/.libs/*.a $(OBJDIR)
	mv libmpeg2/libmpeg2/.libs/*.a $(OBJDIR)


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
package:
	@rm -rf  $(RELEASEDIR)/`date +%Y%m%d`
	@if [ -f $(SVN)/v0/AROS/bin/linux-i386/AROS/wuae ]; then \
		echo "Adding i386/v0 version to package.."; \
		mkdir -p $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/i386/ABI_v0; \
		$(SVN)/v0/AROS/bin/linux-x86_64/tools/ilbmtoicon --png icons/j-uae-drawer.info.src icons/j-uae-drawer.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/i386.info;\
		$(SVN)/v0/AROS/bin/linux-x86_64/tools/ilbmtoicon --png icons/j-uae-drawer.info.src icons/j-uae-drawer.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/i386/ABI_v0.info;\
		cp $(SVN)/v0/AROS/bin/linux-i386/AROS/wuae $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/i386/ABI_v0/Janus-UAE2; \
		$(SVN)/v0/AROS/bin/linux-x86_64/tools/ilbmtoicon --png icons/j-uae.info.src icons/j-uae.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/i386/ABI_v0/Janus-UAE2.info; \
		mkdir -p $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/i386/ABI_v0/images; \
		cp -r gui/images/*.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/i386/ABI_v0/images; \
	fi

	@if [ -f $(SVN)/v1.gcc.6.1/AROS/bin/linux-x86_64/AROS/wuae ]; then \
		echo "Adding x86_64/v1 version to package.."; \
		mkdir -p $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/x86_64/ABI_v1; \
		$(SVN)/v0/AROS/bin/linux-x86_64/tools/ilbmtoicon --png icons/j-uae-drawer.info.src icons/j-uae-drawer.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/x86_64.info;\
		$(SVN)/v0/AROS/bin/linux-x86_64/tools/ilbmtoicon --png icons/j-uae-drawer.info.src icons/j-uae-drawer.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/x86_64/ABI_v1.info;\
		cp $(SVN)/v1.gcc.6.1/AROS/bin/linux-x86_64/AROS/wuae $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/x86_64/ABI_v1/Janus-UAE2; \
		$(SVN)/v0/AROS/bin/linux-x86_64/tools/ilbmtoicon --png icons/j-uae.info.src icons/j-uae.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/x86_64/ABI_v1/Janus-UAE2.info; \
		mkdir -p $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/x86_64/ABI_v1/images; \
		cp -r gui/images/*.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/x86_64/ABI_v1/images; \
	fi

	@$(SVN)/v0/AROS/bin/linux-x86_64/tools/ilbmtoicon --png icons/j-uae-drawer.info.src icons/j-uae-drawer.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2.info
	@echo "Adding documentation to package.."; 
	@mkdir -p $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/docs
	@cp docs/* $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/docs/
	@$(SVN)/v0/AROS/bin/linux-x86_64/tools/ilbmtoicon --png icons/j-uae-drawer.info.src icons/j-uae-drawer.png $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/docs.info

	@echo "Gzip package.."; 
	@cd $(RELEASEDIR)/`date +%Y%m%d`;tar cf Janus-UAE2.tar Janus-UAE2 Janus-UAE2.info;gzip Janus-UAE2.tar
	@echo
	@echo Package built at  $(RELEASEDIR)/`date +%Y%m%d`/Janus-UAE2/Janus-UAE2.tar.gz
	@echo

# ============== dependencies ====================
../rommgr.o: string_resource.h
nogui.o: string_resource.h
-include $(subst .o,.dep,$(DEP_OBJS))

