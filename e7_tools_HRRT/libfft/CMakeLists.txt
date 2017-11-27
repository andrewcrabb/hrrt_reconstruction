# add_subdirectory(e7_atten_prj)

add_library (libfft SHARED 
		fft.cpp)

install(TARGETS libfft
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
