#!/bin/sh

echo 'Generating necessary files...'
rm -rf config
mkdir -p config
libtoolize --force
aclocal
autoheader -Wall
automake --foreign --add-missing -Wall -Wno-portability
autoconf

