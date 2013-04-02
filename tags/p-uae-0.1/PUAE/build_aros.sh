# P-UAE
#
# 2006-2011 Mustafa TUFAN (aka GnoStiC/BRONX)
#
# Script by Mequa
#
#--host=i386-aros --enable-ui --without-x host_alias=i386-aros
#base=" --with-sdl --with-sdl-gl --with-sdl-gfx --with-sdl-sound --enable-drvsnd "
base=" --with-sdl --with-sdl-gfx --with-sdl-sound --enable-drvsnd "
wiqt=" "
cd32=" --enable-cd32 --enable-cdtv "
a600=" --enable-gayle "
#scsi=" --enable-scsi-device --enable-ncr --enable-a2091 "
scsi=" --enable-ncr --enable-a2091 "
other=" --with-caps --enable-amax --enable-jit --enable-gccopt --target=i386-aros host_alias=i386-aros"
#
#
./bootstrap.sh
./configure $base $wiqt $cd32 $a600 $scsi $other CFLAGS="-m32" LDFLAGS="-m32" CPPFLAGS="-m32"
make clean
make

