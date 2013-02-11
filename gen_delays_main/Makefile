# $Id$

# $Log$
#

#

# Definitions
CC              = cc
RM              = rm -f
DEBUG           = 
EXECPATH        = ${HOME}/bin
LIBPATH         = ../gen_delays_lib
# ahc
GEN_DELAYS      = $(LIBPATH)/gen_delays_lib.a
CPPFLAGS        =
CFLAGS          = $(DEBUG)
LDLIBS          = $(GEN_DELAYS) -lm -lpthread  -lstdc++
INCLUDES        = -I../gen_delays_lib
MACRO           = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LINUX
INSTALLDIR      = ../bin

# ------
.SUFFIXES:
.SUFFIXES: .o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<

# ------

all:		gen_delays

gen_delays:	main.o
		$(CC) -o $@ main.o ${LDLIBS}

main.o:		main.cpp $(GEN_DELAYS)

install: 	gen_delays
		cp gen_delays $(INSTALLDIR)

clean:
		$(RM) *~ *.o

clean_all:
		$(RM) *~ *.o gen_delays

