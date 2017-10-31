#!/bin/bash

mypwd=`pwd`
# Note -D _GLIBCXX_USE_CXX11_ABI=0 is necessary to turn off C++11 symbols.
# libscatter.a uses C++03 symbols and can't resolve Logging::logMsg if progs are compiled with C++11
export MYCPPFLAGS="-D_LINUX -D__LINUX__ -D _GLIBCXX_USE_CXX11_ABI=0 -I${mypwd}/AIR5.3.0/src -L${mypwd}/AIR5.3.0/src"
# export MYCPPFLAGS="-D_LINUX -D__LINUX__ -D__MACOSX__ -I${mypwd}/AIR5.3.0/src -L${mypwd}/AIR5.3.0/src"
export MYCFLAGS='-g'
