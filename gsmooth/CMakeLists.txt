add_library (gsmooth SHARED 
	gsmooth.cpp image_filter.cpp )

target_include_directories (gsmooth PUBLIC ../e7_tools_HRRT/libfft)
install(TARGETS gsmooth
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
