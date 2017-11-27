# add_subdirectory(e7_atten_prj)

add_library (libimage SHARED 
	rebin_xy.cpp wholebody.cpp image_conversion.cpp image_io.cpp)

install(TARGETS libimage
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
