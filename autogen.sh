#!/bin/sh

echo 'Generating necessary files...'
rm -rf config
mkdir -p config
libtoolize --force
aclocal
autoheader -Wall
automake --gnu --add-missing -Wall
autoconf

