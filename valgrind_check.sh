#!/bin/bash
# Another common script that I use, this one leak-checks with valgring
# all the executables in the debug folder. Like the rest of the scripts, should
# be run from the root of the project directory.

set -e

for i in ./debug/*.exe; do
  echo $i
  valgrind --error-exitcode= $i
done

echo
echo "Check all debug executables with valgrind SUCCESSFULLY!"

