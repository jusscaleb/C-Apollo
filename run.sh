#!/usr/bin/env sh

set -eu

rm -rf build

cmake -B build -G "MinGW Makefiles"

if [ "$#" -lt 1 ]; then
  echo "Usage: ./run.sh <file.apl>" >&2
  exit 1
fi

APL_FILE="$1"

cmake --build build
./build/apollo.exe "$APL_FILE"
