# cmake_minimum_required (VERSION 3.7)

# From https://goo.gl/ppjWnK, std=c99 will not find strdup, so assumes in and complains about type.
# set (GCC_COMPILE_FLAGS "-std=gnu99")
add_definitions(${GCC_COMPILE_FLAGS})

add_library (ecatx analyze.cpp machine_indep.cpp plandefs.cpp
	crash.cpp interfile.cpp matrix.cpp matrix_slice.cpp scanner_model.cpp
	DICOM.cpp isotope_info.cpp matrix_extra.cpp num_sort.cpp matpkg.cpp)


target_include_directories (ecatx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS ecatx
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
