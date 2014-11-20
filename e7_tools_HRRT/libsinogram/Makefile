# $Id$

# $Log$
#
#
.SUFFIXES:	.o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(MYCPPFLAGS) $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<

CFLAGS =	 -fPIC -D__LINUX__ -I ./ -I ../libexception -I ../libcommon -I ../libecat7 -I ../libfft -I ../libinterfile -I ../libipc

LIBPATH =	../LinkLibs

LIBRARY =	libsinogram.so

STATICLIB =	libsinogram.a

FILES.h =	

SOURCES.cpp =	fore_fft.cpp \
		randoms_smoothing.cpp \
		rebin_x.cpp \
		norm_ecat.cpp \
		rebin_sinogram.cpp \
		ssr.cpp \
		fore.cpp \
		gapfill.cpp \
		sino2d_3d.cpp \
		sinogram_conversion.cpp \
		sinogram_io.cpp

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
