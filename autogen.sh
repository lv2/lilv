#!/bin/sh

echo 'Generating necessary files...'
libtoolize --copy --force
aclocal
autoheader -Wall
automake --foreign --add-missing -Wall
autoconf

