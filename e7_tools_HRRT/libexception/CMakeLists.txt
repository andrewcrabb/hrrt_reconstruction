# add_subdirectory(e7_atten_prj)

add_library (libexception SHARED 
		exception.cpp logging.cpp semaphore_al.cpp timedate.cpp)

install(TARGETS libexception
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
