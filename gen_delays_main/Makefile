# $Id$

# $Log$
#

#

# Definitions
CC              = cc
RM              = rm -f
DEBUG           = -O2
EXECPATH        = ${HOME}/bin
LIBPATH         = ../gen_delays_lib
CPPFLAGS        =
CFLAGS          = $(DEBUG)
LDLIBS          = $(LIBPATH)/gen_delays_lib.a -lm -lpthread  -lstdc++
INCLUDES        = -I../gen_delays_lib
MACRO           = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LINUX

# ------
.SUFFIXES:
.SUFFIXES: .o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

# ------

all:		gen_delays

gen_delays:	main.o
		$(CC) -o $@ main.o ${LDLIBS}

main.o:		main.cpp

install: 	gen_delays
		mv gen_delays $(EXECPATH)

clean:
		$(RM) *~ *.o

clean_all:
		$(RM) *~ *.o gen_delays

