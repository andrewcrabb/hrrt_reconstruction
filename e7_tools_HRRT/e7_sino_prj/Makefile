# $Id$

# $Log$
#
#

# Definitions
CC		= cc
RM		= /bin/rm -f
DEBUG		= 
SRCPATH		= /home/peter/HRRT/HRRT_UG_SW/E7_Tools/18082009/le7_tools_inux/e7_sino_prj
BUILD_DIR	= /home/peter/HRRT/HRRT_UG_SW/E7_Tools/18082009/le7_tools_inux/e7_sino_prj
EXECPATH	= ../E7_Bin
LIBPATH1	= ../LinkLibs
LIBPATH2	= ../LinkLibsScatter
# ahc my additions
LIBATTEN      = ../libatten/libatten.a
LIBCOMMON     = ../libcommon/libcommon.a
LIBCONVERT    = ../libconvert/libconvert.a
LIBECAT7      = ../libecat7/libecat7.a
LIBEXCEPTION  = ../libexception/libexception.a
LIBFFT        = ../libfft/libfft.a
LIBFILTER     = ../libfilter/libfilter.a
LIBIMAGE      = ../libimage/libimage.a
LIBINTERFILE  = ../libinterfile/libinterfile.a
LIBIPC        = ../libipc/libipc.a
LIBPARSER     = ../libparser/libparser.a
LIBSCATTER    = ../LinkLibsScatter/libscatter.a
LIBSINOGRAM   = ../libsinogram/libsinogram.a

LINKLIBS      =  ${LIBCOMMON} ${LIBCONVERT} ${LIBATTEN} ${LIBECAT7} ${LIBEXCEPTION} \
		 ${LIBFFT} ${LIBFILTER} ${LIBIMAGE} ${LIBINTERFILE} ${LIBIPC} \
		 ${LIBPARSER} ${LIBSINOGRAM} ${LIBSCATTER}

CPPFLAGS	=
CFLAGS		= $(DEBUG)
LDLIBS		= -L$(LIBPATH1) -L$(LIBPATH2) -lconvert -lipc -lsinogram -linterfile -lexception -limage -lparser -lcommon -lecat7 -latten -lfft -lfilter -lscatter -lm -l pthread -lstdc++

INCLUDES	= -I ./ -I ../libcommon -I ../libexception -I ../libconvert -I ../libimage -I ../libecat7 -I ../libfft -I ../libfilter -I ../libinterfile -I ../libparser -I ../libsinogram -I ../libipc -I ../libatten -I ../libscatter
MACRO		= -fPIC -D__LINUX__
INSTALLDIR      = ../../bin

.SUFFIXES:	.o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<

all:		e7_sino_u

e7_sino_u:		e7_sino.o ${LINKLIBS}
		$(CC) -o $@ e7_sino.o ${LDLIBS}

e7_sino.o:	e7_sino.cpp

install:
		cp e7_sino_u $(INSTALLDIR)

clean:
		$(RM) *.o *~

clean_all:
		$(RM) *.o e7_sino *~

