# $Id$

# $Log$
#

#

# Definitions
CC              = g++
RM              = rm -f
DEBUG           = -O2
SRCPATH         = .
BUILD_DIR       = $(SRCPATH)
EXECPATH        = ${HOME}/bin
LIBPATH         = 
CPPFLAGS        = -msse3 -Wno-write-strings
CFLAGS          = $(DEBUG)
LDLIBS          = ../gen_delays_lib/gen_delays_lib.a ../ecatx/libecatx.a -lm -lpthread
INCLUDES        = -I../ -I../gen_delays_lib
MACRO           = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE

# ------
.SUFFIXES:
.SUFFIXES: .o .cpp .h

.cpp.o:
				$(CC) -c $(CPPFLAGS) $(CFLAGS) $(MACRO) $(INCLUDES) $<

# ------

all:				je_hrrt_osem3d

je_hrrt_osem3d:			file_io_processor.o  hrrt_osem3d.o  hrrt_osem3dv_sbrt.o  nr_utils.o  psf.o  scanner_model.o  simd_operation.o  write_image_header.o write_ecat_image.o interfile_reader.o
				$(CC) -o $@ file_io_processor.o  hrrt_osem3d.o  hrrt_osem3dv_sbrt.o  nr_utils.o  psf.o  scanner_model.o  simd_operation.o  write_image_header.o write_ecat_image.o interfile_reader.o ${LDLIBS}

file_io_processor.o:		file_io_processor.cpp

hrrt_osem3d.o:		hrrt_osem3d.cpp dpfcalc.c

hrrt_osem3dv_sbrt.o:	hrrt_osem3dv_sbrt.cpp

nr_utils.o:			nr_utils.cpp

psf.o:				psf.cpp

scanner_model.o:		scanner_model.cpp

simd_operation.o:		simd_operation.cpp

write_image_header.o:		write_image_header.cpp

write_ecat_image.o:		write_ecat_image.cpp

interfile_reader.o:		interfile_reader.cpp

install:
				mv hrrt_osem3d $(EXECPATH)

clean:
				$(RM) *~ *.o

clean_all:
				$(RM) *~ *.o hrrt_osem3d
