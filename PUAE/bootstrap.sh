#!/bin/sh

echo .
echo .
echo Please Wait..
echo .
aclocal -I m4 \
&& automake --foreign --add-missing \
&& autoconf
echo ..almost over..
echo .
cd src/tools
aclocal
autoconf
echo Done. Thank you.
echo .
