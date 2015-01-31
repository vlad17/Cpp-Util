#!/bin/bash
# Only run in project directory root.
# really quick build script to just quickly check correctness (before commiting)

set -e

NJOBS=`nproc`

cmake . -DCMAKE_BUILD_TYPE=debug
make -j$NJOBS
ctest -j$NJOBS

cmake . -DCMAKE_BUILD_TYPE=release -DASSERTIONS_ON=1
make -j$NJOBS
ctest -j$NJOBS

cmake . -DCMAKE_BUILD_TYPE=release -DASSERTIONS_ON=0
make -j$NJOBS
ctest -j$NJOBS

./release/mpmc-test.exe bench

echo
echo "Ran all tests SUCCESSFULLY!"
