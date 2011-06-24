#!/bin/bash

VERSION="\"0.14a\""


echo "Compiling muchine ..."
gcc -fomit-frame-pointer -O2 -D MU_VER="$VERSION" -Wall -o muchine muchine.c
strip --strip-all muchine

echo "Compiling muasm ..."
gcc  -D MU_VER="$VERSION" -Wall -o muasm muasm.c
strip --strip-all muasm
