# add_subdirectory(e7_atten_prj)

add_library (libfilter SHARED 
		image_filter.cpp)

install(TARGETS libfilter
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
