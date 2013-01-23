# $Id$

# $Log$
#
#

# Definitions
CC		= cc
RM		= /bin/rm -f
DEBUG		= -O2
SRCPATH		= ../e7_tools_linux/gm_check_prj
BUILD_DIR	= ../e7_tools_linux/gm_check_prj
EXECPATH	= ../E7_Bin

LIBPATH		= ../LinkLibs
CPPFLAGS	= 
CFLAGS		= $(DEBUG)
LDLIBS		= -L $(LIBPATH) -latten -lfft -lfilter -lconvert -lipc -lsinogram -linterfile -lexception -limage -lparser -lcommon -lecat7 -latten -lconvert -lfft -lfilter -lm -l pthread -lstdc++
INCLUDES	= -I ./ -I ../libcommon -I ../libexception -I ../libconvert -I ../libimage -I ../libecat7 -I ../libfft -I ../libfilter -I ../libinterfile -I ../libparser -I ../libsinogram -I ../libipc -I ../libatten
MACRO		= -fPIC -D__LINUX__

.SUFFIXES:	.o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<


all:		gm_check

gm_check:		gm_check.o
		$(CC) -o $@ gm_check.o ${LDLIBS}

gm_check.o:	gm_check.cpp

install:
		mv gm_check $(EXECPATH)

clean:
		$(RM) *.o *~

clean_all:
		$(RM) *.o gm_check *~

