# $Id$

# $Log$
#
#

# Definitions
CC		= g++
RM		= /bin/rm -f
DEBUG		= -O2
DEBUG		= -g
SRCPATH		= .
EXEPATH		= ${HOME}/bin
CPPFLAGS        = 
CFLAGS		= $(DEBUG)
LDLIBS		= ../ecatx/libecatx.a -lm 
INCLUDES	= -I ../
MACRO		= -D_LINUX
INSTALLDIR      = ../bin

# ------
.SUFFIXES: .o .cpp .h

.cpp.o:
			$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

# ------

all:			volume_reslice

volume_reslice:	volume_reslice.o load_volume_v2.o Volume.o matpkg.o matrix_resize.o
			$(CC) -o $@  volume_reslice.o load_volume_v2.o Volume.o matpkg.o matrix_resize.o ${LDLIBS}

install:
			cp volume_reslice $(INSTALLDIR)

clean:
			$(RM) *.o *~

clean_all:
			$(RM) *.o volume_reslice *~
