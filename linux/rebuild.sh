#!/bin/sh

set -x

SOURCE=`pwd`

git pull
if [ ! -x "$SOURCE/bld" ]; then
	mkdir $SOURCE/bld
fi
cd $SOURCE/bld
make clean
cmake $SOURCE/..
make -C $SOURCE/bld

