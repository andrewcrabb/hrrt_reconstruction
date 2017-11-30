# cmake_minimum_required (VERSION 3.7)

# From https://goo.gl/ppjWnK, std=c99 will not find strdup, so assumes in and complains about type.
set (GCC_COMPILE_FLAGS "-std=gnu99")
add_definitions(${GCC_COMPILE_FLAGS})

add_library (ecatx analyze.c machine_indep.c plandefs.c
	crash.c interfile.c matrix.c matrix_slice.c scanner_model.c
	DICOM.c isotope_info.c matrix_extra.c num_sort.c matpkg.c)


target_include_directories (ecatx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS ecatx
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
