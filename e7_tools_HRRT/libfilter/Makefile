# $Id$

# $Log$
#
#
.SUFFIXES:	.o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

CFLAGS =	-O2 -fPIC -D__LINUX__ -I ./ -I ../libcommon -I ../libexception -I ../libecat7 -I ../libfft

LIBPATH =	../LinkLibs

LIBRARY =	libfilter.so

STATICLIB =	libfilter.a

FILES.h =	

SOURCES.cpp =	image_filter.cpp

OBJECTS =	${SOURCES.cpp:.cpp=.o}

# all:		${LIBRARY} ${STATICLIB}
default:	${STATICLIB}
all:		install

# ${LIBRARY}:	${OBJECTS}
# 		${CC} --shared -Wl,-soname,${LIBRARY} -o ${LIBRARY} ${SOURCES.cpp:.cpp=.o}

${STATICLIB}:	${OBJECTS}
		${AR} r ${STATICLIB} ${SOURCES.cpp:.cpp=.o}
		ranlib ${STATICLIB}

install:	${STATICLIB}
		cp ${STATICLIB} ${LIBPATH}

clean:
		rm -f *.o *~

clean_all:
		rm -f ${STATICLIB} ${LIBRARY} *.o *~
