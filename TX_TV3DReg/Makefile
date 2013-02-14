# $Id$

# $Log$
#
#

# Definitions
CC		= g++
RM		= /bin/rm -f
DEBUG		= -g
CPPFLAGS	= 
CFLAGS		= $(DEBUG) -D cimg_OS=0 -D cimg_display_type=0
LDLIBS		= 
INCLUDES	= -I .
MACRO		= -fPIC -D__LINUX__
INSTALLDIR  = ../bin

.SUFFIXES:	.o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<


all:		TX_TV3DReg

TX_TV3DReg:		TX_TV3DReg.o
		$(CC) -o $@ TX_TV3DReg.o ${LDLIBS}

TX_TV3DReg.o:	TX_TV3DReg.cpp

install:
		cp TX_TV3DReg $(INSTALLDIR)

clean:
		$(RM) *.o *~

clean_all:
		$(RM) *.o e7_fwd *~

