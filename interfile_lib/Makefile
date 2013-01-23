# $Id$

# $Log$
#
#
CFLAGS =	-O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LINUX -I./

LIBRARY =	./interfile_lib.a

FILES.h =	interfile_reader.h

SOURCES.c =	interfile_reader.c

OBJECTS =	${FILES.h} ${SOURCES.c:.c=.o}

all:		${LIBRARY}

${LIBRARY}:	${OBJECTS}
		ar rv ${LIBRARY} ${SOURCES.c:.c=.o}
		ranlib ${LIBRARY}

clean:
		rm -f *.o *~

clean_all:
		rm -f ${LIBRARY} *.o *~
