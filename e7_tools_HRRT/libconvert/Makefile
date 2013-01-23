# $Id$

# $Log$
#
#
.SUFFIXES:	.o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

CFLAGS =	-O2 -fPIC -D__LINUX__ -I ./ -I ../libcommon -I ../libexception -I ../libecat7 -I ../libatten -I ../libsinogram -I ../libinterfile -I ../libimage -I ../libfft -I ../libparser -I ../libipc -I ../libfilter

LIBPATH =	../LinkLibs

LIBRARY =	libconvert.so

STATICLIB =	libconvert.a

FILES.h =	

SOURCES.cpp =	dift.cpp \
		matrix.cpp \
		bckprj3d.cpp \
		fbp.cpp \
		fwdprj3d.cpp \
		osem3d.cpp \
		red_client.cpp \
		convert.cpp \
		atten_reco.cpp \
		scatter.cpp
#
# red.cpp
# red_main.cpp
# 

OBJECTS =	${SOURCES.cpp:.cpp=.o}

# all:		${LIBRARY} ${STATICLIB}
default:	${STATICLIB}
all:	install

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
