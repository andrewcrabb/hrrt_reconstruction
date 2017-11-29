# $Id$

# $Log$
#
#

# Definitions
CC		= g++
RM		= /bin/rm -f
DEBUG		= 
CPPFLAGS        = -Wno-write-strings
CFLAGS		= $(DEBUG)
LDLIBS		= 
INCLUDES	= -I ..
MACRO		= -D__LINUX__
INSTALLDIR      = ../bin

# ------
.SUFFIXES: .o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<

# ------

all: compute_norm


compute_norm: compute_norm.o gantryinfo.o matrix.o self_norm.o det_to_ve.o gapfill.o plot.o  sino_to_crys.o dwell_sino.o global_tmpl.o polyfit.o fit_dwell.o make_norm.o polyfitw.o
		$(CC) -o $@   compute_norm.o gantryinfo.o matrix.o self_norm.o det_to_ve.o gapfill.o plot.o  sino_to_crys.o dwell_sino.o global_tmpl.o polyfit.o fit_dwell.o make_norm.o polyfitw.o ${LDLIBS}


install:
		cp compute_norm $(INSTALLDIR)

clean:
		$(RM) *.o *~

clean_all:
		$(RM) *.o compute_norm *~

