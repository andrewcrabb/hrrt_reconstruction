# $Id$

# $Log$
#
#

# Definitions
CC		= cc
RM		= /bin/rm -f
DEBUG		= 
SRCPATH		= /home/peter/HRRT/HRRT_UG_SW/E7_Tools/18082009/le7_tools_inux/e7_fwd_prj
BUILD_DIR	= /home/peter/HRRT/HRRT_UG_SW/E7_Tools/18082009/le7_tools_inux/e7_fwd_prj
EXECPATH	= ../E7_Bin
LIBPATH1	= ../LinkLibs
LIBPATH2	= ../LinkLibsScatter
CPPFLAGS	= 
CFLAGS		= $(DEBUG)
LDLIBS		= -L $(LIBPATH1) -L $(LIBPATH2) -latten -lfft -lfilter -lconvert -lipc -lsinogram -linterfile -lexception -limage -lparser -lcommon -lecat7 -latten -lconvert -lfft -lfilter -lscatter -lm -l pthread -lstdc++
INCLUDES	= -I ./ -I ../libcommon -I ../libexception -I ../libconvert -I ../libimage -I ../libecat7 -I ../libfft -I ../libfilter -I ../libinterfile -I ../libparser -I ../libsinogram -I ../libipc -I ../libatten -I ../libscatter
MACRO		= -fPIC -D__LINUX__
INSTALLDIR      = ../bin

.SUFFIXES:	.o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<


all:		e7_fwd_u

e7_fwd_u:		e7_fwd.o
		$(CC) -o $@ e7_fwd.o ${LDLIBS}

e7_fwd.o:	e7_fwd.cpp

install:
		cp e7_fwd_u $(INSTALLDIR)

clean:
		$(RM) *.o *~

clean_all:
		$(RM) *.o e7_fwd *~

