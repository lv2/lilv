#!/bin/sh

echo 'Generating necessary files...'
rm -rf config
mkdir -p config
# Mac
if [ -x "`which glibtoolize`" ]; then
	glibtoolize --force
else
	libtoolize --force
fi
aclocal
autoheader -Wall
automake --foreign --add-missing
autoconf

