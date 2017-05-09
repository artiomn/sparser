#!/bin/sh

./configure --build=x86 --host=i586-mingw32msvc --enable-nls=yes&& make -j5
