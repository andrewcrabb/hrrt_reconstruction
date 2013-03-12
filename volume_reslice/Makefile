# $Id$

# $Log$
#
#

# Definitions
CC		?= g++
RM		= /bin/rm -f
DEBUG		= 
DEBUG		= -g
SRCPATH		= .
EXEPATH		= ${HOME}/bin
CPPFLAGS        = -I../include
CFLAGS		= $(DEBUG)
LDLIBS		= ../ecatx/libecatx.a -lm 
INCLUDES	= -I ../
MACRO		= -D_LINUX
INSTALLDIR      = ../bin
LIBAIR      = ../AIR5.3.0/src/libRPW.a

# ------
.SUFFIXES: .o .cpp .h

.cpp.o:
			$(CC) -c $(CPPFLAGS) ${MYCPPFLAGS} $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<

# ------

all:			volume_reslice

volume_reslice:	volume_reslice.o load_volume_v2.o Volume.o matpkg.o matrix_resize.o ${LIBAIR}
			$(CC) -o $@  volume_reslice.o load_volume_v2.o Volume.o matpkg.o matrix_resize.o ${LIBAIR} ${LDLIBS}

install:
			cp volume_reslice $(INSTALLDIR)

clean:
			$(RM) *.o *~

clean_all:
			$(RM) *.o volume_reslice *~
