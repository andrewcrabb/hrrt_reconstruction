# Hack.  Don't build first time round, since libraries not made yet.
if(LIBCOMMON)
	add_executable (e7_sino_u e7_sino.cpp)
	target_link_libraries (e7_sino_u LINK_PUBLIC pthread ${LIBSINOGRAM} ${LIBECAT7} ${LIBINTERFILE} ${LIBPARSER} ${LIBFFT} ${LIBIPC} ${LIBEXCEPTION} ${LIBCONVERT} ${LIBIMAGE} ${LIBATTEN} ${LIBFILTER} ${LIBCOMMON} ${LIBSCATTER} )
	target_include_directories (e7_sino_u PUBLIC ${LIB_INCLUDE_DIRS})
endif()

install(TARGETS e7_sino_u
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
