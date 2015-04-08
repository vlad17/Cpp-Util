#!/bin/bash
# Only run in project directory root.
# really quick build script to just quickly check correctness (before commiting)

set -e

NJOBS=`nproc`

cmake . -DCMAKE_BUILD_TYPE=debug
make -j$NJOBS
ctest -j$NJOBS

cmake . -DCMAKE_BUILD_TYPE=release -DASSERTIONS=1
make -j$NJOBS
ctest -j$NJOBS

cmake . -DCMAKE_BUILD_TYPE=release -DASSERTIONS=2
make -j$NJOBS
ctest -j$NJOBS

./release/mpmc-test.exe bench

rm -rf CMakeFiles/ CMakeCache.txt

echo
echo "Ran all tests SUCCESSFULLY!"
