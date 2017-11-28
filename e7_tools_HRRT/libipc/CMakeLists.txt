# add_subdirectory(e7_atten_prj)

add_library (libipc SHARED 
		comm_socket.cpp progress.cpp sockets.cpp stream_buffer.cpp)

install(TARGETS libipc
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
