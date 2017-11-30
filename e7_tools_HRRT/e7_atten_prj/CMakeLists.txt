set (MY_INCLUDE_DIRS
	${LIB_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libconvert
	)

add_executable (e7_atten_u e7_atten.cpp)
target_link_libraries (e7_atten_u LINK_PUBLIC pthread sinogram ecat7 interfile parser fft ipc exception image convert atten filter common ${LIBSCATTER} )
target_include_directories (e7_atten_u PUBLIC ${MY_INCLUDE_DIRS})

install(TARGETS e7_atten_u
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
