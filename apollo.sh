#!/usr/bin/env bash
set -e

# Require the Apollo source file path, such as output/main.apl.
if [ $# -lt 1 ]; then
    echo "Usage: ./apollo.sh <file.apl>"
    exit 1
fi

# Keep the driver executable next to the generated compiler output.
mkdir -p output

# Build the Apollo driver.
gcc apl.c -o output/apl.exe

# Run the driver from output/ and pass the requested Apollo file through.
(
    cd output
    ./apl.exe "$1"
)
