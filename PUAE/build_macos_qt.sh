# P-UAE
#
# 2006-2011 Mustafa TUFAN (aka GnoStiC/BRONX)
#
#
#
base=" --with-sdl --with-sdl-gl --with-sdl-gfx --with-sdl-sound --enable-drvsnd --with-qt "
cd32=" --enable-cd32 "
a600=" --enable-gayle "
scsi=" --enable-scsi-device --enable-ncr --enable-a2091 "
other=" --with-caps --enable-amax "
debug="--enable-profiling"
debug=""
#
#
./bootstrap.sh
./configure $base $cd32 $a600 $scsi $other $debug CFLAGS="-m32" LDFLAGS="-m32" CPPFLAGS="-m32" CXXFLAGS="-m32"
make clean
make
