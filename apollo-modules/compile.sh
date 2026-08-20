#!/usr/bin/env sh


if [ "$#" -lt 1 ]; then
  echo "No <file.c> argument, so exiting..."
  exit 0

fi


clang -emit-llvm -O3 -flto  -ffunction-sections -fdata-sections -DNDEBUG -c "$(pwd)"/"$1"/"$1".c -o "$(pwd)"/"$1"/"$1".bc

