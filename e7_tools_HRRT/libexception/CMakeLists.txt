add_library (exception
		exception.cpp logging.cpp semaphore_al.cpp timedate.cpp)

install(TARGETS exception
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
