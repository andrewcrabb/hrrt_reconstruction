# $Id$

# $Log$
#
#

# Definitions

CC              = g++
RM              = rm -f
DEBUG           = -O2
SRCPATH         = /home/peter/HRRT/HRRT_UG_SW/V_1.0.1/Src/HRRT_1_0_1_src/calcRingRoiRatio
BUILD_DIR       = $(SRCPATH)
EXECPATH        = /home/peter/Src/Bin
LIBPATH         = ../interfile_lib
CPPFLAGS        =
CFLAGS          = $(DEBUG)
LDLIBS          = $(LIBPATH)/interfile_lib.a -lm -lstdc++
INCLUDES        = -I../interfile_lib
MACRO           = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LINUX
INSTALLDIR      = ../bin

# ------
.SUFFIXES:
.SUFFIXES: .o .cpp .h

.cpp.o:
			$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

# ------

all:			calcRingRoiRatio

calcRingRoiRatio:	calcRingRoiRatio.o
			$(CC) -o $@ calcRingRoiRatio.o ${LDLIBS}

calcRingRoiRatio.o:	calcRingRoiRatio.cpp

install:
			cp calcRingRoiRatio $(INSTALLDIR)

clean:
			$(RM) *.o *~

clean_all:
			$(RM) *.o calcRingRoiRatio *~
