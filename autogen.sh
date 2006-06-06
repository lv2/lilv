#!/bin/sh

echo 'Generating necessary files...'
libtoolize --copy --force
aclocal
autoheader -Wall
automake --gnu --add-missing -Wall
autoconf

