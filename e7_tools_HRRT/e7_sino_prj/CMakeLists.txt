add_executable (e7_sino_u e7_sino.cpp)
target_link_libraries (e7_sino_u LINK_PUBLIC pthread sinogram ecat7 interfile parser fft ipc exception convert image atten filter common ${LIBSCATTER})
target_include_directories (e7_sino_u PUBLIC ${LIB_INCLUDE_DIRS})

install(TARGETS e7_sino_u
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
