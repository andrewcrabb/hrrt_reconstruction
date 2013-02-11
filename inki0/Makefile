CC = g++ -g -msse3 -mfpmath=sse -funroll-all-loops -ffast-math -lm -lpthread 
DEBUG = -O3
RM = rm
CP = cp
SRCDIR = .
CFLAGS =  $(DEBUG) -c -D IS_LINUX -U IS_WIN32
LDFLAGS = $(DEBUG)

ALL : hrrt_osem3d_span3

hrrt_osem3d_span3 : scanner_model.o  hrrt_osem3dv_sbrt_span3.o nr_utils.o file_io_processor.o simd_operation.o hrrt_osem3d_span3.o mm_malloc.o psf.o write_image_header.o
	$(CC) $(LDFLAGS) $(LIB) hrrt_osem3d_span3.o nr_utils.o file_io_processor.o simd_operation.o hrrt_osem3dv_sbrt_span3.o scanner_model.o mm_malloc.o psf.o write_image_header.o -o hrrt_osem3d_span3


hrrt_osem3d_span3.o : $(SRCDIR)/hrrt_osem3d_span3.cpp $(SRCDIR)/hrrt_osem3dv_sbrt_span3.cpp $(SRCDIR)/hrrt_osem3d.h
	$(CC) $(CFLAGS) ${MYCFLAGS} $(SRCDIR)/hrrt_osem3d_span3.cpp


hrrt_osem3dv_sbrt_span3.o :   $(SRCDIR)/hrrt_osem3dv_sbrt_span3.cpp $(SRCDIR)/hrrt_osem3d.h
	$(CC) $(CFLAGS) ${MYCFLAGS} $(SRCDIR)/hrrt_osem3dv_sbrt_span3.cpp

file_io_processor.o  :  $(SRCDIR)/file_io_processor.cpp
	$(CC) $(CFLAGS) ${MYCFLAGS} $(SRCDIR)/file_io_processor.cpp

simd_operation.o  :  $(SRCDIR)/simd_operation.cpp
	$(CC) $(CFLAGS) ${MYCFLAGS} $(SRCDIR)/simd_operation.cpp

mm_malloc.o  :  $(SRCDIR)/mm_malloc.cpp
	$(CC) $(CFLAGS) ${MYCFLAGS} $(SRCDIR)/mm_malloc.cpp

nr_utils.o  :  $(SRCDIR)/nr_utils.cpp
	$(CC) $(CFLAGS) ${MYCFLAGS} $(SRCDIR)/nr_utils.cpp

scanner_model.o : $(SRCDIR)/scanner_model.cpp
	$(CC) $(CFLAGS) ${MYCFLAGS} $(SRCDIR)/scanner_model.cpp

psf.o : $(SRCDIR)/psf.cpp
	$(CC) $(CFLAGS) ${MYCFLAGS} $(SRCDIR)/psf.cpp

write_image_header.o : $(SRCDIR)/write_image_header.cpp
	$(CC) $(CFLAGS) ${MYCFLAGS} $(SRCDIR)/write_image_header.cpp

clean:
	$(RM) hrrt_osem3d_span3 *.o 


depend:
	makedepend -Dunix $(CFLAGS) ${MYCFLAGS} $(SRCDIR)/*.cpp
# DO NOT DELETE THIS LINE -- make depend depends on it.

./bmpi.o: /usr/include/stdio.h /usr/include/features.h
./bmpi.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
./bmpi.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
./bmpi.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
./bmpi.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
./bmpi.o: /usr/include/bits/wchar.h /usr/include/gconv.h
./bmpi.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
./bmpi.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
./bmpi.o: /usr/include/endian.h /usr/include/bits/endian.h
./bmpi.o: /usr/include/sys/select.h /usr/include/bits/select.h
./bmpi.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
./bmpi.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
./bmpi.o: /usr/include/alloca.h bmpi.h
./file_io_processor.o: compile.h /usr/include/malloc.h
./file_io_processor.o: /usr/include/features.h /usr/include/sys/cdefs.h
./file_io_processor.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
./file_io_processor.o: /usr/include/gnu/stubs-64.h /usr/include/string.h
./file_io_processor.o: /usr/include/stdlib.h /usr/include/sys/types.h
./file_io_processor.o: /usr/include/bits/types.h
./file_io_processor.o: /usr/include/bits/typesizes.h /usr/include/time.h
./file_io_processor.o: /usr/include/endian.h /usr/include/bits/endian.h
./file_io_processor.o: /usr/include/sys/select.h /usr/include/bits/select.h
./file_io_processor.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
./file_io_processor.o: /usr/include/sys/sysmacros.h
./file_io_processor.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
./file_io_processor.o: /usr/include/math.h /usr/include/bits/huge_val.h
./file_io_processor.o: /usr/include/bits/mathdef.h
./file_io_processor.o: /usr/include/bits/mathcalls.h /usr/include/stdio.h
./file_io_processor.o: /usr/include/libio.h /usr/include/_G_config.h
./file_io_processor.o: /usr/include/wchar.h /usr/include/bits/wchar.h
./file_io_processor.o: /usr/include/gconv.h /usr/include/bits/stdio_lim.h
./file_io_processor.o: /usr/include/bits/sys_errlist.h
./file_io_processor.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
./file_io_processor.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
./file_io_processor.o: scanner_model.h hrrt_osem3d.h nr_utils.h
./hrrt_osem3d_span3.o: compile.h /usr/include/stdio.h /usr/include/features.h
./hrrt_osem3d_span3.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
./hrrt_osem3d_span3.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
./hrrt_osem3d_span3.o: /usr/include/bits/types.h
./hrrt_osem3d_span3.o: /usr/include/bits/typesizes.h /usr/include/libio.h
./hrrt_osem3d_span3.o: /usr/include/_G_config.h /usr/include/wchar.h
./hrrt_osem3d_span3.o: /usr/include/bits/wchar.h /usr/include/gconv.h
./hrrt_osem3d_span3.o: /usr/include/bits/stdio_lim.h
./hrrt_osem3d_span3.o: /usr/include/bits/sys_errlist.h /usr/include/malloc.h
./hrrt_osem3d_span3.o: /usr/include/string.h /usr/include/stdlib.h
./hrrt_osem3d_span3.o: /usr/include/sys/types.h /usr/include/time.h
./hrrt_osem3d_span3.o: /usr/include/endian.h /usr/include/bits/endian.h
./hrrt_osem3d_span3.o: /usr/include/sys/select.h /usr/include/bits/select.h
./hrrt_osem3d_span3.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
./hrrt_osem3d_span3.o: /usr/include/sys/sysmacros.h
./hrrt_osem3d_span3.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
./hrrt_osem3d_span3.o: /usr/include/math.h /usr/include/bits/huge_val.h
./hrrt_osem3d_span3.o: /usr/include/bits/mathdef.h
./hrrt_osem3d_span3.o: /usr/include/bits/mathcalls.h /usr/include/fcntl.h
./hrrt_osem3d_span3.o: /usr/include/bits/fcntl.h write_image_header.h
./hrrt_osem3d_span3.o: scanner_model.h hrrt_osem3d.h nr_utils.h
./hrrt_osem3d_span3.o: file_io_processor.h simd_operation.h mm_malloc.h
./hrrt_osem3d_span3.o: /usr/include/errno.h /usr/include/bits/errno.h
./hrrt_osem3d_span3.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
./hrrt_osem3d_span3.o: /usr/include/asm-x86_64/errno.h
./hrrt_osem3d_span3.o: /usr/include/asm-generic/errno.h
./hrrt_osem3d_span3.o: /usr/include/asm-generic/errno-base.h
./hrrt_osem3dv_sbrt_span3.o: compile.h mm_malloc.h /usr/include/stdlib.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/features.h /usr/include/sys/cdefs.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/wordsize.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/gnu/stubs.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/gnu/stubs-64.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/sys/types.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/types.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/typesizes.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/time.h /usr/include/endian.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/endian.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/sys/select.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/select.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/sigset.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/time.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/sys/sysmacros.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/pthreadtypes.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/alloca.h /usr/include/errno.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/errno.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/linux/errno.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/asm/errno.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/asm-x86_64/errno.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/asm-generic/errno.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/asm-generic/errno-base.h
./hrrt_osem3dv_sbrt_span3.o: hrrt_osem3d.h /usr/include/stdio.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/libio.h /usr/include/_G_config.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/wchar.h /usr/include/bits/wchar.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/gconv.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/stdio_lim.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/sys_errlist.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/string.h /usr/include/math.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/huge_val.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/mathdef.h
./hrrt_osem3dv_sbrt_span3.o: /usr/include/bits/mathcalls.h scanner_model.h
./hrrt_osem3dv_sbrt_span3.o: nr_utils.h
./mm_malloc.o: /usr/include/stdlib.h /usr/include/features.h
./mm_malloc.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
./mm_malloc.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
./mm_malloc.o: /usr/include/sys/types.h /usr/include/bits/types.h
./mm_malloc.o: /usr/include/bits/typesizes.h /usr/include/time.h
./mm_malloc.o: /usr/include/endian.h /usr/include/bits/endian.h
./mm_malloc.o: /usr/include/sys/select.h /usr/include/bits/select.h
./mm_malloc.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
./mm_malloc.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
./mm_malloc.o: /usr/include/alloca.h /usr/include/errno.h
./mm_malloc.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
./mm_malloc.o: /usr/include/asm/errno.h /usr/include/asm-x86_64/errno.h
./mm_malloc.o: /usr/include/asm-generic/errno.h
./mm_malloc.o: /usr/include/asm-generic/errno-base.h
./nr_utils.o: compile.h /usr/include/stdio.h /usr/include/features.h
./nr_utils.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
./nr_utils.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
./nr_utils.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
./nr_utils.o: /usr/include/libio.h /usr/include/_G_config.h
./nr_utils.o: /usr/include/wchar.h /usr/include/bits/wchar.h
./nr_utils.o: /usr/include/gconv.h /usr/include/bits/stdio_lim.h
./nr_utils.o: /usr/include/bits/sys_errlist.h nr_utils.h
./nr_utils.o: /usr/include/stdlib.h /usr/include/sys/types.h
./nr_utils.o: /usr/include/time.h /usr/include/endian.h
./nr_utils.o: /usr/include/bits/endian.h /usr/include/sys/select.h
./nr_utils.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
./nr_utils.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
./nr_utils.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
./nr_utils.o: mm_malloc.h /usr/include/errno.h /usr/include/bits/errno.h
./nr_utils.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
./nr_utils.o: /usr/include/asm-x86_64/errno.h
./nr_utils.o: /usr/include/asm-generic/errno.h
./nr_utils.o: /usr/include/asm-generic/errno-base.h
./psf.o: /usr/include/stdio.h /usr/include/features.h
./psf.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
./psf.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
./psf.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
./psf.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
./psf.o: /usr/include/bits/wchar.h /usr/include/gconv.h
./psf.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
./psf.o: /usr/include/math.h /usr/include/bits/huge_val.h
./psf.o: /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h
./psf.o: hrrt_osem3d.h compile.h /usr/include/string.h /usr/include/stdlib.h
./psf.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/endian.h
./psf.o: /usr/include/bits/endian.h /usr/include/sys/select.h
./psf.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
./psf.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
./psf.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
./psf.o: scanner_model.h
./scanner_model.o: scanner_model.h /usr/include/stdio.h
./scanner_model.o: /usr/include/features.h /usr/include/sys/cdefs.h
./scanner_model.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
./scanner_model.o: /usr/include/gnu/stubs-64.h /usr/include/bits/types.h
./scanner_model.o: /usr/include/bits/typesizes.h /usr/include/libio.h
./scanner_model.o: /usr/include/_G_config.h /usr/include/wchar.h
./scanner_model.o: /usr/include/bits/wchar.h /usr/include/gconv.h
./scanner_model.o: /usr/include/bits/stdio_lim.h
./scanner_model.o: /usr/include/bits/sys_errlist.h
./simd_operation.o: hrrt_osem3d.h compile.h /usr/include/stdio.h
./simd_operation.o: /usr/include/features.h /usr/include/sys/cdefs.h
./simd_operation.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
./simd_operation.o: /usr/include/gnu/stubs-64.h /usr/include/bits/types.h
./simd_operation.o: /usr/include/bits/typesizes.h /usr/include/libio.h
./simd_operation.o: /usr/include/_G_config.h /usr/include/wchar.h
./simd_operation.o: /usr/include/bits/wchar.h /usr/include/gconv.h
./simd_operation.o: /usr/include/bits/stdio_lim.h
./simd_operation.o: /usr/include/bits/sys_errlist.h /usr/include/string.h
./simd_operation.o: /usr/include/stdlib.h /usr/include/sys/types.h
./simd_operation.o: /usr/include/time.h /usr/include/endian.h
./simd_operation.o: /usr/include/bits/endian.h /usr/include/sys/select.h
./simd_operation.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
./simd_operation.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
./simd_operation.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
./simd_operation.o: /usr/include/math.h /usr/include/bits/huge_val.h
./simd_operation.o: /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h
./simd_operation.o: scanner_model.h
./write_image_header.o: /usr/include/stdio.h /usr/include/features.h
./write_image_header.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
./write_image_header.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-64.h
./write_image_header.o: /usr/include/bits/types.h
./write_image_header.o: /usr/include/bits/typesizes.h /usr/include/libio.h
./write_image_header.o: /usr/include/_G_config.h /usr/include/wchar.h
./write_image_header.o: /usr/include/bits/wchar.h /usr/include/gconv.h
./write_image_header.o: /usr/include/bits/stdio_lim.h
./write_image_header.o: /usr/include/bits/sys_errlist.h /usr/include/string.h
./write_image_header.o: write_image_header.h /usr/include/stdlib.h
./write_image_header.o: /usr/include/sys/types.h /usr/include/time.h
./write_image_header.o: /usr/include/endian.h /usr/include/bits/endian.h
./write_image_header.o: /usr/include/sys/select.h /usr/include/bits/select.h
./write_image_header.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
./write_image_header.o: /usr/include/sys/sysmacros.h
./write_image_header.o: /usr/include/bits/pthreadtypes.h
./write_image_header.o: /usr/include/alloca.h
