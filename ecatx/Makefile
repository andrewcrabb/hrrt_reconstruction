# $Id$

# $Log$
#
#
# INCLUDES=-I /usr/include/sys

# CC=gcc -arch x86_64
CC=gcc -Wall -pedantic

.SUFFIXES: .o .c .h

.c.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<

CFLAGS =

INSTALL_DIR= ../lib
LIBRARY =	./libecatx.a

SOURCES.c =	analyze.c machine_indep.c plandefs.c \
		crash.c interfile.c matrix.c matrix_slice.c scanner_model.c \
		DICOM.c isotope_info.c matrix_extra.c num_sort.c matpkg.c


OBJECTS =	${SOURCES.c:.c=.o}

all:		${LIBRARY}

${LIBRARY}:	${OBJECTS}
#		libtool -o ${LIBRARY} ${OBJECTS}
		ar rv ${LIBRARY} ${OBJECTS}
		ranlib ${LIBRARY}

install: ${LIBRARY}
	cp  ${LIBRARY} ${INSTALL_DIR}
clean:
		rm -f *.o *~

clean_all:
		rm -f ${LIBRARY} *.o *~
