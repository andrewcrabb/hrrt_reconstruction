# add_subdirectory(e7_atten_prj)

add_library (libinterfile SHARED 
		interfile.cpp kv.cpp)

install(TARGETS libinterfile
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
