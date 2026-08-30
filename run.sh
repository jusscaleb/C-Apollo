#!/usr/bin/env sh

set -eu

export PATH="/mingw64/bin:/c/msys64/mingw64/bin:/c/msys64/clang64/bin:$PATH"

args="${1:-}"


if [ "$args" != 'build' ]; then
    if [ ! -f "build/Makefile" ]; then
        cmake -B build -G "MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    fi

    cmake --build build 

    echo "Apollo compiled successfully!"

fi

if [ "$#" -lt 1 ]; then
  echo "No <file.apl> argument, so exiting..."
  exit 0

else
  echo "Executing apl file"
  if [ "$1" = 'build' ]; then

    ./build/apollo.exe "$2"

  else
    APL_FILE="$1"
    ./build/apollo.exe "$APL_FILE"
  
  fi
fi
