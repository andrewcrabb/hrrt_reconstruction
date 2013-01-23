# $Id$

# $Log$
#

#

# Definitions
CC              = g++
RM              = rm -f
DEBUG           = -O2
SRCPATH         = .
BUILD_DIR       = $(SRCPATH)
EXECPATH        = ${HOME}/bin
LIBPATH         = 
CPPFLAGS        = -Wno-write-strings 
CFLAGS          = $(DEBUG)
LDLIBS          = ../gen_delays_lib/gen_delays_lib.a -lpthread  -lstdc++ -lm
INCLUDES        =  -I..
MACRO           = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE  -DHRRT -D_LUT_ -D_LINUX

# ------
.SUFFIXES:
.SUFFIXES: .o .cpp .h

.cpp.o:
				$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

# ------

all:		SortHrrtLM-64bit

SortHrrtLM-64bit:	SortHrrtLM-64bit.o 
			$(CC) -o $@ SortHrrtLM-64bit.o ${LDLIBS}

SortHrrtLM-64bit.o:		SortHrrtLM-64bit.cpp

