set (LIB_INCLUDE_DIRS
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libcommon
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libexception
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libecat7
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libimage
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libinterfile
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libparser
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libsinogram
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libfft
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libipc
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libatten
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libfilter
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libconvert
	)

add_executable (gsmooth_ps gsmooth.cpp image_filter.cpp )
target_link_libraries (gsmooth_ps LINK_PUBLIC ecatx)

add_definitions(-D GSMOOTH)
target_include_directories (gsmooth_ps PUBLIC ${LIB_INCLUDE_DIRS})
install(TARGETS gsmooth_ps
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
