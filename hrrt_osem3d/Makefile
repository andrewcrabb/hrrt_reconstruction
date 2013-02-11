# $Id$

# $Log$
#

#

# Definitions
CC              = g++
RM              = rm -f
DEBUG           = 
SRCPATH         = .
BUILD_DIR       = $(SRCPATH)
EXECPATH        = ${HOME}/bin
LIBPATH         = 
CPPFLAGS        = -msse3 -Wno-write-strings
CFLAGS          = $(DEBUG)
# ahc
GEN_DELAYS      = ../gen_delays_lib/gen_delays_lib.a
ECATX           = ../ecatx/libecatx.a
LDLIBS          = ${GEN_DELAYS} ${ECATX} -lm -lpthread
INCLUDES        = -I../ -I../gen_delays_lib
MACRO           = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
INSTALLDIR      = ../bin

# ------
.SUFFIXES:
.SUFFIXES: .o .cpp .h

.cpp.o:
				$(CC) -c $(CPPFLAGS) $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<

# ------

all:				hrrt_osem3d

hrrt_osem3d:			file_io_processor.o  hrrt_osem3d_span3.o  hrrt_osem3dv_sbrt_span3.o  nr_utils.o  psf.o  scanner_model.o  simd_operation.o  write_image_header.o write_ecat_image.o interfile_reader.o ${GEN_DELAYS} ${ECATX}
				$(CC) -o $@ file_io_processor.o  hrrt_osem3d_span3.o  hrrt_osem3dv_sbrt_span3.o  nr_utils.o  psf.o  scanner_model.o  simd_operation.o  write_image_header.o write_ecat_image.o interfile_reader.o ${LDLIBS}

file_io_processor.o:		file_io_processor.cpp

hrrt_osem3d_span3.o:		hrrt_osem3d_span3.cpp

hrrt_osem3dv_sbrt_span3.o:	hrrt_osem3dv_sbrt_span3.cpp

nr_utils.o:			nr_utils.cpp

psf.o:				psf.cpp

scanner_model.o:		scanner_model.cpp

simd_operation.o:		simd_operation.cpp

write_image_header.o:		write_image_header.cpp

write_ecat_image.o:		write_ecat_image.cpp

interfile_reader.o:		interfile_reader.cpp

install:
				cp hrrt_osem3d $(INSTALLDIR)

clean:
				$(RM) *~ *.o

clean_all:
				$(RM) *~ *.o hrrt_osem3d
