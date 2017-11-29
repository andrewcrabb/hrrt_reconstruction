add_library (interfile
		interfile.cpp kv.cpp)

install(TARGETS interfile
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
