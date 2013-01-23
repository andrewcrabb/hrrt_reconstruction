# $Id$

# $Log$
#
#

# Definitions
CC		= g++
RM		= /bin/rm -f
DEBUG		= -O2
SRCPATH		= .
BUILD_DIR	= .
EXECPATH	= ${HOME}/Bin
LIBPATH		=
CPPFLAGS        = -Wno-write-strings
CFLAGS		= $(DEBUG)
LDLIBS		= ../ecatx/libecatx.a -L/usr/local/lib -lRPW -lm
INCLUDES	= -I../include
MACRO		=  -D_FILE_OFFSET_BITS=64 -D_LINUX

# ------
.SUFFIXES: .o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

# ------

all:		motion_distance

motion_distance:	motion_distance.o 
		$(CC) -o $@  motion_distance.o $(LDLIBS)


install:
		mv motion_distance $(EXECPATH)

clean:
		$(RM) *.o *~

clean_all:
		$(RM) *.o motion_distance *~

