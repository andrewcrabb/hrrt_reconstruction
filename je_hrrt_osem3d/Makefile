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

LDLIBS          = -L../ecatx -l ecatx ${GEN_DELAYS} -lm -lpthread
INCLUDES        = -I../ -I../gen_delays_lib
MACRO           = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
INSTALLDIR      = ../bin

# ------
.SUFFIXES:
.SUFFIXES: .o .cpp .h

.cpp.o:
				$(CC) -c $(CPPFLAGS) $(CFLAGS) ${MYCFLAGS} $(MACRO) $(INCLUDES) $<

# ------

all:				je_hrrt_osem3d

je_hrrt_osem3d:			file_io_processor.o  hrrt_osem3d.o  hrrt_osem3dv_sbrt.o  nr_utils.o  psf.o  scanner_model.o  simd_operation.o  write_image_header.o write_ecat_image.o interfile_reader.o ${GEN_DELAYS} ${ECATX}
				$(CC) -o $@ file_io_processor.o  hrrt_osem3d.o  hrrt_osem3dv_sbrt.o  nr_utils.o  psf.o  scanner_model.o  simd_operation.o  write_image_header.o write_ecat_image.o interfile_reader.o ${LDLIBS}

install:
				cp hrrt_osem3d je_hrrt_osem3d $(INSTALLDIR)

clean:
				$(RM) *~ *.o

clean_all:
				$(RM) *~ *.o hrrt_osem3d
