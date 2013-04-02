# P-UAE
#
# 2006-2011 Mustafa TUFAN (aka GnoStiC/BRONX)
#
#
#
base=" --with-sdl --with-sdl-gfx --with-sdl-sound --enable-drvsnd "
glgl=" --with-sdl-gl "
wiqt=" "
cd32=" --enable-cd32 "
a600=" --enable-gayle "
scsi=" --enable-scsi-device --enable-ncr --enable-a2091 "
other=" --with-caps --enable-amax "
#
#
./bootstrap.sh
./configure $base $wiqt $cd32 $a600 $scsi $other
make clean
make
