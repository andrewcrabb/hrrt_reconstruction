# $Id$

# $Log$
#
#

# Definitions
CC		= g++ 
RM		= /bin/rm -f
DEBUG		= -g
SRCPATH		= .
BUILD_DIR	= .
EXECPATH	= ${HOME}/bin
LIBPATH		=
CPPFLAGS        = -Wno-write-strings
CFLAGS		= $(DEBUG)
LDLIBS		= ../ecatx/libecatx.a -lm
INCLUDES	= -I../include
MACRO		=  -D_FILE_OFFSET_BITS=64 -D_LINUX
INSTALLDIR      = ../bin

# ------
.SUFFIXES: .o .c .h

.c.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

# ------

all:		if2e7

if2e7:	if2e7.o splitpath.o interfile_reader.o calibration_table.o
		$(CC) -o $@  if2e7.o splitpath.o interfile_reader.o calibration_table.o $(LDLIBS)

install:
		cp if2e7 $(INSTALLDIR)

clean:
		$(RM) *.o *~

clean_all:
		$(RM) *.o if2e7 *~

