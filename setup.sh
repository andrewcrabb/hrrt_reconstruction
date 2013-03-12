#!/bin/bash

mypwd=`pwd`
export MYCPPFLAGS="-D_LINUX -D__LINUX__ -I${mypwd}/AIR5.3.0/src -L${mypwd}/AIR5.3.0/src"
export MYCFLAGS='-g'
