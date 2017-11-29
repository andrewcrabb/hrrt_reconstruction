add_library (image
	rebin_xy.cpp wholebody.cpp image_conversion.cpp image_io.cpp)

install(TARGETS image
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
