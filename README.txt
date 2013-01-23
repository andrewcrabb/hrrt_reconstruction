


1) Update name of directory in the path in .cshrc:

set path = (/home/rahmim/ForAndy/hrrt_codes_Arman/*/ $path)
set path = (/home/rahmim/ForAndy/hrrt_codes_Arman/*/*/ $path)

Both above lines are really needed.

2)  Make sure  libraries/hrrt_rebinner.lut is executable:
 chmod r+x file (i.e. executable)

3) Typing make clear; make does the compiling job except for few directories which you need to go in there specifically:

Perform Make in:
 *ecatx/utils
 *gsmooth

In folder TX_TV3DReg, do the following (turn display mode off) [took a lot of struggle and feedback from Merence/Sune to figure this out; they also modified their source code to accomodate this]:
g++ TX_TV3DReg.cpp -I . -D cimg_OS=0 -D cimg_display_type=0 -o TX_TV3DReg 

4) The motion programs use the gnuplot command. The package needs to be installed.


