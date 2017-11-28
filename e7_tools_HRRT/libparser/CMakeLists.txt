# add_subdirectory(e7_atten_prj)

add_library (libparser SHARED 
	parser.cpp getopt_wrapper.cpp)

install(TARGETS libparser
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
