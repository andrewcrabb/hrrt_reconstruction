# $Id$

# $Log$
#
#
.SUFFIXES:	.o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

CFLAGS =	-O2 -fPIC -D__LINUX__ -I ./ -I ../libcommon -I ../libexception

LIBPATH =	../LinkLibs

LIBRARY =	libecat7.so

STATICLIB =	libecat7.a

FILES.h =	

SOURCES.cpp =	data_changer.cpp \
		ecat7_attenuation.cpp \
		ecat7.cpp \
		ecat7_directory.cpp \
		ecat7_image.cpp \
		ecat7_mainheader.cpp \
		ecat7_matrix_abc.cpp \
		ecat7_norm3d.cpp \
		ecat7_norm.cpp \
		ecat7_polar.cpp \
		ecat7_scan3d.cpp \
		ecat7_scan.cpp \
		ecat7_utils.cpp \
		ecat_tmpl.cpp

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
