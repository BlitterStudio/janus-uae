# P-UAE
#
# 2006-2013 Mustafa TUFAN
#
# this script cleans up various files so that we don't push unnecessary files to git..
#

make clean

rm -rf ./df0.adz
rm -rf ./kick.rom
rm -rf ./kick_ext.rom

rm -rf ./aclocal.m4

rm -rf ./src/gfxdep
rm -rf ./src/guidep
rm -rf ./src/joydep
rm -rf ./src/machdep
rm -rf ./src/osdep
rm -rf ./src/sounddep
rm -rf ./src/threaddep
rm -rf ./src/PUAE.app

rm -rf `find . -type d -name autom4te.cache`
rm -rf `find . -type d -name .deps`
rm -rf `find . -type f -name Makefile`
rm -rf `find . -type f -name *~`
rm -rf `find . -type f -name *.o`
rm -rf `find . -type f -name *.a`
rm -rf `find . -type f -name configure`
rm -rf `find . -type f -name config.log`
rm -rf `find . -type f -name config.status`
rm -rf `find . -type f -name .DS_Store`
rm -rf `find . -type f -name sysconfig.h`
rm -rf `find . -type f -name gmon.out`

rm -rf Makefile.in
rm -rf src/Makefile.in
rm -rf src/archivers/dms/Makefile.in
rm -rf src/archivers/zip/Makefile.in
rm -rf src/caps/Makefile.in
rm -rf src/gfx-amigaos/Makefile.in
rm -rf src/gfx-beos/Makefile.in
rm -rf src/gfx-curses/Makefile.in
rm -rf src/gfx-sdl/Makefile.in
rm -rf src/gfx-svga/Makefile.in
rm -rf src/gfx-x11/Makefile.in
rm -rf src/gfx-cocoa/Makefile.in
rm -rf src/gui-beos/Makefile.in
rm -rf src/gui-cocoa/Makefile.in
rm -rf src/gui-gtk/Makefile.in
rm -rf src/gui-muirexx/Makefile.in
rm -rf src/gui-none/Makefile.in
rm -rf src/gui-qt/Makefile.in
rm -rf src/gui-sdl/Makefile.in
rm -rf src/jd-amigainput/Makefile.in
rm -rf src/jd-amigaos/Makefile.in
rm -rf src/jd-beos/Makefile.in
rm -rf src/jd-linuxold/Makefile.in
rm -rf src/jd-none/Makefile.in
rm -rf src/jd-sdl/Makefile.in
rm -rf src/keymap/Makefile.in
rm -rf src/md-68k/Makefile.in
rm -rf src/md-amd64-gcc/Makefile.in
rm -rf src/md-generic/Makefile.in
rm -rf src/md-i386-gcc/Makefile.in
rm -rf src/md-ppc-gcc/Makefile.in
rm -rf src/md-ppc/Makefile.in
rm -rf src/od-amiga/Makefile.in
rm -rf src/od-beos/Makefile.in
rm -rf src/od-generic/Makefile.in
rm -rf src/od-linux/Makefile.in
rm -rf src/od-macosx/Makefile.in
rm -rf src/od-macosx/Credits.rtf
rm -rf src/od-macosx/Info.plist
rm -rf src/sd-alsa/Makefile.in
rm -rf src/sd-amigaos/Makefile.in
rm -rf src/sd-beos/Makefile.in
rm -rf src/sd-none/Makefile.in
rm -rf src/sd-sdl/Makefile.in
rm -rf src/sd-solaris/Makefile.in
rm -rf src/sd-uss/Makefile.in
rm -rf src/td-amigaos/Makefile.in
rm -rf src/td-beos/Makefile.in
rm -rf src/td-none/Makefile.in
rm -rf src/td-posix/Makefile.in
rm -rf src/td-sdl/Makefile.in
rm -rf src/test/Makefile.in

# remove generated files, just in case
# blitter
rm -rf src/blit.h
rm -rf src/blitfunc.c
rm -rf src/blitfunc.h
rm -rf src/blittable.c
rm -rf src/linetoscr.c
# cpu
rm -rf src/cpudefs.c
rm -rf src/cputbl.h
rm -rf src/cpustbl.c
rm -rf src/compemu.c
rm -rf src/comptbl.h
rm -rf src/compstbl.c
rm -rf src/cpuemu_0.c
rm -rf src/cpuemu_11.c
rm -rf src/cpuemu_12.c
rm -rf src/cpuemu_20.c
rm -rf src/cpuemu_21.c
rm -rf src/cpuemu_22.c
rm -rf src/cpuemu_31.c
rm -rf src/cpuemu_32.c
rm -rf src/cpuemu_33.c
# jit
rm -rf src/compemu.cpp
rm -rf src/compstbl.h
rm -rf src/compstbl.cpp

rm -rf src/md-fpp.h
rm -rf src/target.h

# tools
rm -rf src/tools/blitops.c
rm -rf src/tools/build68k.c
rm -rf src/tools/genblitter.c
rm -rf src/tools/gencomp.c
rm -rf src/tools/gencpu.c
rm -rf src/tools/genlinetoscr.c
rm -rf src/tools/missing.c
rm -rf src/tools/readcpu.c
rm -rf src/tools/writelog.c

echo "=================================================="
echo "Current Commit: "
tail -1 .git/packed-refs | awk '{print $1}'
echo "=================================================="
