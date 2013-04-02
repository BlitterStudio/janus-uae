# P-UAE
#
# 2006-2011 Mustafa TUFAN (aka GnoStiC/BRONX)
#
# this is the main script to build (and test) PUAE
#
base=" --with-sdl --with-sdl-gfx --with-sdl-sound --enable-drvsnd --with-sdl-gui --with-sdl-gl "
wiqt=" "
cd32=" --enable-cd32 "
a600=" --enable-gayle "
scsi=" --enable-scsi-device --enable-ncr --enable-a2091 "
other=" --with-caps --enable-amax --enable-gccopt --enable-serial-port "
debug=" --enable-gccdebug "
#
#
./bootstrap.sh
./configure $debug $base $cd32 $a600 $scsi $other CFLAGS="-m32" LDFLAGS="-m32" CPPFLAGS="-m32"
make clean
make
