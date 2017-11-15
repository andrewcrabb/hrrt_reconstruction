# $Id$

# $Log$
#
#

# Definitions
CC		= cc
RM		= /bin/rm -f
DEBUG		= 
SRCPATH		= /home/peter/HRRT/HRRT_UG_SW/V1.1/Src/HRRT_U_1_1_src/norm_process
BUILD_DIR	= /home/peter/HRRT/HRRT_UG_SW/V1.1/Src/HRRT_U_1_1_src/norm_process
EXECPATH	= /home/peter/Src/Bin
LIBPATH		=
CPPFLAGS        = -Wno-write-strings
CFLAGS		= $(DEBUG)
# ahc
GEN_DELAYS      = ../gen_delays_lib/gen_delays_lib.a
LDLIBS		= ${GEN_DELAYS} -lm -lpthread -lstdc++
INCLUDES	= -I .. -I/usr/include 
MACRO		=  -D_FILE_OFFSET_BITS=64 -D_LINUX
# If want to report timing of the normalisation calculation threads
# -D_THREADTIME
INSTALLDIR      = ../bin

# ------
.SUFFIXES: .o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<

# ------

all:		norm_process

norm_process:	norm_process.o cal_sen.o fan_sum.o get_gs.o Header.o inter.o MarkTime.o rod_dwell.o rotation_dwell_sino.o ${GEN_DELAYS}
		$(CC) -o $@ norm_process.o cal_sen.o fan_sum.o get_gs.o Header.o inter.o MarkTime.o rod_dwell.o rotation_dwell_sino.o ${LDLIBS}

norm_process.o:	norm_process.cpp Header.h scanner_params.h rod_dwell.h fan_sum.h cal_sen.h get_gs.h

cal_sen.o:	cal_sen.cpp cal_sen.h scanner_params.h

fan_sum.o:	fan_sum.cpp fan_sum.h scanner_params.h

get_gs.o:	get_gs.cpp get_gs.h gr_ga.h scanner_params.h

Header.o:	Header.cpp Header.h

inter.o:	inter.cpp inter.h scanner_params.h

MarkTime.o:	MarkTime.cpp MarkTime.h

rod_dwell.o:	rod_dwell.cpp inter.h rod_dwell.h scanner_params.h

rotation_dwell_sino.o:	rotation_dwell_sino.cpp rotation_dwell_sino.h

install:
		cp norm_process $(INSTALLDIR)

clean:
		$(RM) *.o *~

clean_all:
		$(RM) *.o norm_process *~

