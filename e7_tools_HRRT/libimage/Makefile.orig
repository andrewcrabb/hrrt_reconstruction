# $Id$

# $Log$
#
#
.SUFFIXES:	.o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(MYCPPFLAGS) $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<

CFLAGS =	 -fPIC -D__LINUX__ -I ./ -I ../libcommon -I ../libexception -I ../libecat7 -I ../libinterfile -I ../libparser -I ../libsinogram -I ../libfilter -I ../libfft -I ../libipc

LIBPATH =	../LinkLibs

LIBRARY =	libimage.so

STATICLIB =	libimage.a

FILES.h =	

SOURCES.cpp =	rebin_xy.cpp \
		wholebody.cpp \
		image_conversion.cpp \
		image_io.cpp

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
