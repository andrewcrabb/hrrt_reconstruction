set (MY_INCLUDE_DIRS
	${LIB_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libconvert
	)

# message("LIBCOMMON    is ${LIBCOMMON}")
# message("LIBEXCEPTION is ${LIBEXCEPTION}")
# message("LIBSINOGRAM  is ${LIBSINOGRAM}")
# message("LIBECAT7     is ${LIBECAT7}")
# message("LIBINTERFILE is ${LIBINTERFILE}")
# message("LIBPARSER    is ${LIBPARSER}")
# message("LIBFFT       is ${LIBFFT}")
# message("LIBIPC       is ${LIBIPC}")
# message("LIBCONVERT   is ${LIBCONVERT}")
# message("LIBIMAGE     is ${LIBIMAGE}")
# message("LIBATTEN     is ${LIBATTEN}")
# message("LIBFILTER    is ${LIBFILTER}")
# message("LIBSCATTER   is ${LIBSCATTER}")

# Hack.  Don't build first time round, since libraries not made yet.
if(LIBCOMMON)
	add_executable (e7_atten_u e7_atten.cpp)
	# target_compile_definitions(e7_atten_u PUBLIC _GLIBCXX_USE_CXX11_ABI=0)
	target_link_libraries (e7_atten_u LINK_PUBLIC pthread ${LIBSINOGRAM} ${LIBECAT7} ${LIBINTERFILE} ${LIBPARSER} ${LIBFFT} ${LIBIPC} ${LIBEXCEPTION} ${LIBIMAGE} ${LIBCONVERT} ${LIBATTEN} ${LIBFILTER} ${LIBCOMMON} ${LIBSCATTER} )
	target_include_directories (e7_atten_u PUBLIC ${MY_INCLUDE_DIRS})
	# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -D _GLIBCXX_USE_CXX11_ABI=0")
endif()

# message("LIB_INCLUDE_DIRS in e7_atten_prj are ${LIB_INCLUDE_DIRS}")

install(TARGETS e7_atten_u
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
