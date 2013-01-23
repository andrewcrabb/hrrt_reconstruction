# $Id$

# $Log$
#
#
.SUFFIXES:	.o .cpp .h

.cpp.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

CFLAGS =	-O2 -fPIC -D__LINUX__ -I ./ -I ./ -I ../libexception -I ../libcommon -I ../libecat7 -I ../libimage -I ../libparser -I ../libinterfile -I ../libsinogram

LIBPATH =	../LinkLibs

LIBRARY =	libcommon.so

STATICLIB =	libcommon.a

FILES.h =	

SOURCES.cpp =	vecmath.cpp \
		unique_name.cpp \
		thread_wrapper.cpp \
		syngo_msg.cpp \
		str_tmpl.cpp \
		stopwatch.cpp \
		rng.cpp \
		raw_io.cpp \
		hwinfo.cpp \
		gm.cpp \
		global_tmpl.cpp \
		gantryinfo.cpp \
		stream_buffer.cpp \
		swap_tmpl.cpp \
		e7_common.cpp \
		mem_ctrl.cpp

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
