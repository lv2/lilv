#!/bin/sh

echo 'Generating necessary files...'
mkdir -p config
libtoolize --force
aclocal
autoheader -Wall
automake --foreign --add-missing -Wall
autoconf

