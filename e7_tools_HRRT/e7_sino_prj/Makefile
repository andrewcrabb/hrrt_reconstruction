# $Id$

# $Log$
#
#

# Definitions
CC		= cc
RM		= /bin/rm -f
DEBUG		= -O2
SRCPATH		= /home/peter/HRRT/HRRT_UG_SW/E7_Tools/18082009/le7_tools_inux/e7_sino_prj
BUILD_DIR	= /home/peter/HRRT/HRRT_UG_SW/E7_Tools/18082009/le7_tools_inux/e7_sino_prj
EXECPATH	= ../E7_Bin
LIBPATH1	= ../LinkLibs
LIBPATH2	= ../LinkLibsScatter
CPPFLAGS	=
CFLAGS		= $(DEBUG)
LDLIBS		= -L$(LIBPATH1) -L$(LIBPATH2) -latten -lfft -lfilter -lconvert -lipc -lsinogram -linterfile -lexception -limage -lparser -lcommon -lecat7 -latten -lconvert -lfft -lfilter -lscatter -lm -l pthread -lstdc++
INCLUDES	= -I ./ -I ../libcommon -I ../libexception -I ../libconvert -I ../libimage -I ../libecat7 -I ../libfft -I ../libfilter -I ../libinterfile -I ../libparser -I ../libsinogram -I ../libipc -I ../libatten -I ../libscatter
MACRO		= -fPIC -D__LINUX__

.SUFFIXES:	.o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

all:		e7_sino_u

e7_sino_u:		e7_sino.o
		$(CC) -o $@ e7_sino.o ${LDLIBS}

e7_sino.o:	e7_sino.cpp

install:
		mv e7_sino $(EXECPATH)

clean:
		$(RM) *.o *~

clean_all:
		$(RM) *.o e7_sino *~

