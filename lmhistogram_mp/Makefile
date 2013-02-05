# $Id$

# $Log$
#

#

# Definitions
CC              = g++
RM              = rm -f
DEBUG           = -O2
#DEBUG           = -g
SRCPATH         = /home/peter/HRRT/HRRT_UG_SW/V_1.0.1/Src/HRRT_1_0_1_src/hrrt_osem3d
BUILD_DIR       = $(SRCPATH)
EXECPATH        = $(HOME)/bin
LIBPATH         = 
CPPFLAGS        = -msse3 -Wno-write-strings
CFLAGS          = $(DEBUG)
LDLIBS          = ../gen_delays_lib/gen_delays_lib.a  -lm -lpthread
INCLUDES        = -I../ -I../gen_delays_lib
MACRO           = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE  -DFILENAME_MAX=1024 -DHRRT_REBINNER_LUT
INSTALLDIR      = ../bin

# ------
.SUFFIXES:
.SUFFIXES: .o .cpp .h

.cpp.o:
				$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

# ------

all:	lmhistogram_mp

lmhistogram_mp:	convert_span9.o dtc.o gantryinfo.o Header.o histogram_mp.o lmhistogram_mp.o LM_Reader_mp.o LM_Rebinner_mp.o Flags.o
		$(CC) -o $@ convert_span9.o dtc.o gantryinfo.o Header.o histogram_mp.o lmhistogram_mp.o LM_Reader_mp.o LM_Rebinner_mp.o Flags.o ${LDLIBS}

install:
		cp lmhistogram_mp $(INSTALLDIR)

clean:
		$(RM) *~ *.o

clean_all:
		$(RM) *~ *.o lmhistogram_mp
