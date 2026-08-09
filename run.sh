#!/usr/bin/env sh

set -eu



if [ "$1" != 'build' ]; then
    rm -rf build
    cmake -B build -G "MinGW Makefiles"

    cmake --build build

    echo "Apollo compiled successfully!"

fi

if [ "$#" -lt 1 ]; then
  echo "No .apl argument, so exiting..."
  exit 0

else
  echo "Executing apl file"
  if [ "$1" == 'build' ]; then

    ./build/apollo.exe "$2"

  else
    APL_FILE="$1"
    ./build/apollo.exe "$APL_FILE"
  
  fi
fi
