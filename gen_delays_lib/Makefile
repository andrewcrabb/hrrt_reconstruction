# $Id$

# $Log$
#
#

# ahc
MACRO		=  -D_FILE_OFFSET_BITS=64 -D_LINUX

.SUFFIXES: .o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<

CFLAGS =	 -msse -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LINUX -I./

LIBRARY =	./gen_delays_lib.a

FILES.h =	gen_delays.h \
		geometry_info.h \
		lor_sinogram_map.h \
		segment_info.h

SOURCES.cpp =	gen_delays.cpp \
		geometry_info.cpp \
		lor_sinogram_map.cpp \
		segment_info.cpp

OBJECTS =	${FILES.h} ${SOURCES.cpp:.cpp=.o}

all:		${LIBRARY}

${LIBRARY}:	${OBJECTS}
		ar rv ${LIBRARY} ${SOURCES.cpp:.cpp=.o}
		ranlib ${LIBRARY}

install:
	cp ${LIBRARY} ../lib

clean:
		rm -f *.o *~

clean_all:
		rm -f ${LIBRARY} *.o *~
