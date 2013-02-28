# $Id$

# $Log$
#
#

# Definitions
CC		= g++
RM		= /bin/rm -f
DEBUG		= 
SRCPATH		= .
BUILD_DIR	= .
EXECPATH	= ${HOME}/bin
LIBPATH		=
CPPFLAGS        = -Wno-write-strings
CFLAGS		= $(DEBUG)
LDLIBS		= ../ecatx/libecatx.a -L/usr/local/lib -lRPW -lm
INCLUDES	= -I../include
# MACRO		=  -D_FILE_OFFSET_BITS=64 -D_LINUX
MACRO		=  -D_FILE_OFFSET_BITS=64
INSTALLDIR      = ../bin
LIBDEPS     = ../ecatx/libecatx.a ../AIR5.3.0/src/libRPW.a

# ------
.SUFFIXES: .o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(MYCPPFLAGS) $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<

# ------

all:		motion_qc motion_correct_recon MAF_join

motion_qc:	motion_qc.o ${LIBDEPS}
		$(CC) -o $@  motion_qc.o  $(LDLIBS) $(MYCPPFLAGS)

motion_correct_recon:	motion_correct_recon.o qmatrix.o  ${LIBDEPS}
		$(CC) -o $@  motion_correct_recon.o qmatrix.o  $(LDLIBS) $(MYCPPFLAGS)

MAF_join:	MAF_join.o  ${LIBDEPS}
		$(CC) -o $@  MAF_join.o  $(LDLIBS) $(MYCPPFLAGS)

sino_transform:	sino_transform.o matpkg.o mm_malloc.o nr_utils.o ${LIBDEPS}
		$(CC) -o $@  sino_transform.o matpkg.o mm_malloc.o nr_utils.o $(LDLIBS) $(MYCPPFLAGS)

sino_transform.o:	matpkg.h
matpkg.o:	matpkg.h
mm_malloc.o:	mm_malloc.h
nr_utils.o:	nr_utils.h


install:
		cp motion_qc motion_correct_recon $(INSTALLDIR)

clean:
		$(RM) *.o *~

clean_all:
		$(RM) *.o sino_transform *~

