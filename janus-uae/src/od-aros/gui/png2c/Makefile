#
# simple Makefile for png2c
#

# to build for 
# AROS ABI v0: ABI=v0
# AROS ABI v1: ABI=v1
#ABI        = v1

BITS := $(shell getconf LONG_BIT)
ifeq ($(BITS),64)
	# 64 bit
	CPU      = x86_64
	# AROS source svn
	SVN        = $(HOME)/projects
else
	# 32 bit
	CPU      = i386
	SVN        = $(HOME)/projects
endif

CC         = $(SVN)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-gcc
CPP        = $(SVN)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-g++
AR         = $(SVN)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-ar
LD         = $(SVN)/AROS/bin/linux-$(CPU)/tools/crosstools/$(CPU)-aros-ld

HOST_CC    = gcc
EXE        = ./png2c
LIB        = ../libimagedata.a

FLAGS      = -Wno-write-strings -O2 
ARFLAGS    = rc
DEBUGFLAGS = -g

INCLUDES   = 

LINK_LIBS  = -lpng

# ============== common ====================

OBJS   = png2c.o

IMAGES = floppy35.o \
	amigainfo.o \
	chip.o \
	avioutput.o \
	chip.o \
	configfile.o \
	cpu.o \
	diskimage.o \
	drive.o \
	expansion.o \
	file.o \
	folder.o \
	joystick.o \
	misc.o \
	move_dow.o \
	move_up.o \
	paths.o \
	port.o \
	quickstart.o \
	root.o \
	screen.o \
	sound.o \
	winuae2.o

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $*.c

%.o: ../images/%.png
	@echo "converting $< to "$(notdir $(subst .png,.o,$<))" .."
	$(EXE) $< $(notdir $(subst .png,.cpp,$<)) $(notdir $(subst .png,.h,$<)) >/dev/null
	$(CC) -c $(notdir $(subst .png,.cpp,$<)) -o $(notdir $(subst .png,.o,$<))

all: png2c lib

png2c: png2c.o
	$(HOST_CC) png2c.c -o $(EXE) $(LINK_LIBS)

lib: $(IMAGES)
	$(AR) $(ARFLAGS) $(LIB) $(IMAGES)

# ============== clean ====================
clean: 
	rm -f $(EXE) png2c.o $(IMAGES)
	rm -f $(subst .o,.cpp,$(IMAGES))
	rm -f $(subst .o,.h,$(IMAGES))
	rm -f $(LIB)

