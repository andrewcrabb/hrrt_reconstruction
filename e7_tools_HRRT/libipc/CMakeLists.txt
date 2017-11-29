add_library (ipc
		comm_socket.cpp progress.cpp sockets.cpp stream_buffer.cpp)

install(TARGETS ipc
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
