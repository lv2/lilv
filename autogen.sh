#!/bin/sh

echo 'Generating necessary files...'
rm -rf config
mkdir -p config
if [ -x "`which glibtoolize`" ]; then # OSX
	glibtoolize --force
else # Unix
	libtoolize --force
fi
aclocal
autoheader -Wall
automake --foreign --add-missing
autoconf

