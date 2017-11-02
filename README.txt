My notes ahc
------------

invert_air must be copied from the AIR directory to the path of all the e7_ programs.

Old notes from Arman
--------------------

1. Preprocessor flags:

MYCPPFLAGS="-D_LINUX -D__LINUX__ -L${mypwd}/AIR5.3.0/src"

2)  Make sure  libraries/hrrt_rebinner.lut is executable:

3) Typing make clear; make does the compiling job except for few directories which you need to go in there specifically:

Perform Make in:
 *ecatx/utils
 *gsmooth

In folder TX_TV3DReg, do the following (turn display mode off) [took a lot of struggle and feedback from Merence/Sune to figure this out; they also modified their source code to accomodate this]:
g++ TX_TV3DReg.cpp -I . -D cimg_OS=0 -D cimg_display_type=0 -o TX_TV3DReg 

4) The motion programs use the gnuplot command. The package needs to be installed.


