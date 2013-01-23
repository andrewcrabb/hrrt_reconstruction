# $Id$

# $Log$
#
#
.SUFFIXES:	.o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

CFLAGS =	-O2 -fPIC -D__LINUX__ -I ./ -I ../libcommon -I ../libexception -I ../libecat7

LIBPATH =	../LinkLibs

LIBRARY =	libatten.so

STATICLIB =	libatten.a

FILES.h =	

SOURCES.cpp =	Gauss_3D.cpp \
		GM_3D.cpp \
		PB_3D.cpp \
		PB_TV_3D.cpp \
		Projected_Image_3D.cpp \
		TV_3D.cpp \
		Tx_PB_Gauss_3D.cpp \
		Tx_PB_GM_3D.cpp \
		Tx_PB_TV_3D.cpp \
		Tx_PB_3D.cpp

OBJECTS =	${SOURCES.cpp:.cpp=.o}

# all:		${LIBRARY} ${STATICLIB}
default:	${LIBRARY}
all:		install

# ${LIBRARY}:	${OBJECTS}
#		${CC} --shared -Wl,-soname,${LIBRARY} -o ${LIBRARY} ${SOURCES.cpp:.cpp=.o}

${STATICLIB}:	${OBJECTS}
		${AR} r ${STATICLIB} ${SOURCES.cpp:.cpp=.o}
		ranlib ${STATICLIB}

install:	${STATICLIB}
		cp ${STATICLIB} ${LIBPATH}

clean:
		rm -f *.o *~

clean_all:
		rm -f ${STATICLIB} ${LIBRARY} *.o *~
