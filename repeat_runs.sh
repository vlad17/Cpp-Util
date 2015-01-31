#!/bin/bash
# Repeats the execution of the $2 mode $1 times. Defaults are debug and 1000,
# for $1 and $2 arguments, respectively.
#
# This is useful for coaxing hard bugs that only arise with rare thread
# interleavings.

mode="$1"
runs="$2"

if [ -z "$mode" ]; then
  mode="debug"
fi

if [ -z "$runs" ]; then
  runs=1000
fi

cmake . -DCMAKE_BUILD_TYPE="$mode"

make -j8

for i in `seq 1 $runs`; do
  if ! ./"$mode"/mpmc-test.exe; then
    break
  fi
done


