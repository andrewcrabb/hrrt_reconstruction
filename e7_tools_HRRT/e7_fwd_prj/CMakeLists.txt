set (MY_INCLUDE_DIRS
	${LIB_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libconvert
	)

# Hack.  Don't build first time round, since libraries not made yet.
if(LIBCOMMON)
	add_executable (e7_fwd_u e7_fwd.cpp)
	target_link_libraries (e7_fwd_u LINK_PUBLIC pthread ${LIBSINOGRAM} ${LIBECAT7} ${LIBINTERFILE} ${LIBPARSER} ${LIBFFT} ${LIBIPC} ${LIBEXCEPTION} ${LIBIMAGE} ${LIBCONVERT} ${LIBATTEN} ${LIBFILTER} ${LIBCOMMON} ${LIBSCATTER} )
	target_include_directories (e7_fwd_u PUBLIC ${MY_INCLUDE_DIRS})
endif()

install(TARGETS e7_fwd_u
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
