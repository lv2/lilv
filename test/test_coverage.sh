#!/bin/sh

# Run this script (from the directory it's in) after building
# (with waf configure --debug --test) to run the
# unit tests and generate a code coverage report

cd ../build/default

lcov -d ./src -z
./test/slv2_test
lcov -d ./src -d ./test -b .. -c > coverage.lcov
mkdir -p ./coverage
genhtml -o coverage coverage.lcov
echo "Report written to:"
echo "../build/default/coverage/index.html"

