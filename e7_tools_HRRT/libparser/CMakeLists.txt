add_library (parser
	parser.cpp getopt_wrapper.cpp)

install(TARGETS parser
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
