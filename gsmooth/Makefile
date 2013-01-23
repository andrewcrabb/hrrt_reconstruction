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
EXECPATH	= ${HOME}/bin
LIBPATH		=
CPPFLAGS        = -D GSMOOTH -msse3 -Wno-write-strings
CFLAGS		= $(DEBUG)
LDLIBS		= ../ecatx/libecatx.a -lm
INCLUDES	= -I../include
MACRO		=  -D_FILE_OFFSET_BITS=64 -D_LINUX

# ------
.SUFFIXES: .o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

# ------

all:		gsmooth_ps

gsmooth_ps:	gsmooth.o image_filter.o
		$(CC) -o $@  gsmooth.o image_filter.o $(LDLIBS)

gsmooth.o:	image_filter.h 
image_filter.o:	image_filter.h global_tmpl.h global_tmpl.cpp

install:
		mv gsmooth_ps $(EXECPATH)

clean:
		$(RM) *.o *~

clean_all:
		$(RM) *.o gsmooth_ps *~

