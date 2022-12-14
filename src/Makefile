#
# simple Makefile to build native tools and generate all necessary files
#
# Copyright 2015 Oliver Brunner - aros<at>oliver-brunner.de
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
#---------------------------------------------------------------------
# Bug: cpuemu_11.cpp and compstbl.cpp might fail to be generated on 
#      the first run.
#      => run make again and it will be there. Sorry for that.
#---------------------------------------------------------------------

# to build for 
# AROS ABI v0: ABI=v0 (currently not supported)
# AROS ABI v1: ABI=v1

BITS := $(shell getconf LONG_BIT)
ifeq ($(BITS),64)
# 64 bit
CPU      = x86_64
# AROS source svn
SVN        = $(HOME)/projects
ABI        = v1
else
# 32 bit
CPU      = i386
SVN        = $(HOME)/projects
ABI        = v0
endif

# you have to adjust this of course..
TARGET     = $(SVN)/AROS/bin/linux-$(CPU)/AROS/wuae

CC         = $(SVN)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-gcc
#RANLIB = 
AR         = $(SVN)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-ar
LD         = $(SVN)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-ld

RELEASEDIR = $(HOME)/projects/aros-releases

HOST_CC    = gcc

# suppress warning: deprecated conversion from string constant to ‘char*’
FLAGS      = -Wno-write-strings -O2 #-Wall 
DEBUGFLAGS = -g

CDEFS   =
INCLUDES= -Iinclude -I.

CFLAGS = $(CDEFS) $(DEBUGFLAGS) $(FLAGS) $(INCLUDES)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $*.c

all: tools uae


clean: toolsclean uaeclean

# ============== BUILD TOOLS ====================
# Built with host compiler!

tools: genblitter build68k gencpu gencomp genlinetoscr

HOST_CC   =gcc
HOST_CPP  =g++
HOST_INCLUDES=-Iod-aros
HOST_FLAGS=$(DEBUGFLAGS) $(HOST_INCLUDES) -DCROSS_COMPILE -Wall -Wno-switch  -fpermissive
LDFLAGS   =

%.o: %.c
	$(HOST_CC) $(HOST_FLAGS) $(INCLUDES) -o $@ -c $*.c  

%.o: %.cpp
	$(HOST_CPP) $(HOST_FLAGS) $(INCLUDES) -o $@ -c $*.cpp  

GENBLITTER_OBJS=genblitter.o blitops.o
genblitter: $(GENBLITTER_OBJS)
	$(HOST_CPP) -Wall $(GENBLITTER_OBJS) -o genblitter

BUILD68K_OBJS=build68k.o unix_missing.o
build68k: $(BUILD68K_OBJS)
	$(HOST_CPP) -Wall $(BUILD68K_OBJS) -o build68k

#GENCPU_OBJS=gencpu.o missing.o readcpu.o unix_missing.o cpudefs.o od-aros/unicode.o 
GENCPU_OBJS=gencpu.o host_readcpu.o cpudefs.o missing.o
gencpu: $(GENCPU_OBJS)
	$(HOST_CPP) -Wall $(GENCPU_OBJS) -o gencpu

#missing.o
GENCOMP_OBJS=jit/gencomp.o host_readcpu.o cpudefs.o unix_missing2.o 
gencomp: $(GENCOMP_OBJS)
	$(HOST_CPP) -Wall $(GENCOMP_OBJS) -o gencomp

# (host_)readcpu.o 
# this gets linked both by host and AROS crosscompiler!
# so have to build and link two different object files..
host_readcpu.o: readcpu.cpp
	$(HOST_CPP) $(HOST_FLAGS) $(INCLUDES) -o host_readcpu.o -c readcpu.cpp  

genlinetoscr: genlinetoscr.o
	$(HOST_CPP) -Wall -o genlinetoscr genlinetoscr.o

cpudefs.cpp: build68k table68k
	@echo "building cpudefs.cpp .."
	./build68k <table68k >cpudefs.cpp

toolsclean:
	rm -f $(GENBLITTER_OBJS) $(BUILD68K_OBJS) $(GENCPU_OBJS) $(GENCOMP_OBJS) genlinetoscr.o cpudefs.cpp blitfunc.cpp blittable.cpp cpuemu_0.cpp cpuemu_11.cpp cpuemu_12.cpp cpuemu_20.cpp cpuemu_21.cpp cpuemu_31.cpp cpustbl.cpp linetoscr.cpp genblitter build68k gencpu gencomp genlinetoscr

# ============== UAE ====================
# Build with target compiler!

SVN = $(HOME)/projects

uae:
	@echo "PLEASE: use make x86_64 | i386 | m68k"

x86_64: tools
	cd od-aros;make SVN=$(SVN) ABI="v1" CPU="x86_64" ARCH="linux" 

i386: tools
	cd od-aros;make SVN=$(SVN) ABI="v0" CPU="i386" ARCH="linux"

m68k: tools
	cd od-aros;make SVN=$(SVN) ABI="m68k" CPU="m68k" ARCH="amiga"

uaeclean:
	cd od-aros;make clean ABI="$(ABI)"

# ============== build package ====================
package: 
	cd od-aros;make package ABI="$(ABI)" RELEASEDIR="$(RELEASEDIR)"

