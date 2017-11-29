set (MY_INCLUDE_DIRS
	${LIB_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libconvert
	)

# set (MY_LINK_DIRS
# 	)

find_library (LIBCOMMON    NAME common    PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libcommon")
find_library (LIBEXCEPTION NAME exception PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libexception")
find_library (LIBSINOGRAM  NAME sinogram  PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libsinogram")
find_library (LIBECAT7     NAME ecat7     PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libecat7")
find_library (LIBINTERFILE NAME interfile PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libinterfile")
find_library (LIBPARSER    NAME parser    PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libparser")
find_library (LIBFFT       NAME fft       PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libfft")
find_library (LIBIPC       NAME ipc       PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libipc")
find_library (LIBCONVERT   NAME convert   PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libconvert")
find_library (LIBIMAGE     NAME image     PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libimage")
find_library (LIBATTEN     NAME atten     PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libatten")
find_library (LIBFILTER    NAME filter    PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libfilter")

message("CMAKE_BINARY_DIR is ${CMAKE_BINARY_DIR}")

message("LIBCOMMON    in e7_atten_prj is ${LIBCOMMON}")
message("LIBEXCEPTION in e7_atten_prj is ${LIBEXCEPTION}")
message("LIBSINOGRAM  in e7_atten_prj is ${LIBSINOGRAM}")
message("LIBECAT7     in e7_atten_prj is ${LIBECAT7}")
message("LIBINTERFILE in e7_atten_prj is ${LIBINTERFILE}")
message("LIBPARSER    in e7_atten_prj is ${LIBPARSER}")
message("LIBFFT       in e7_atten_prj is ${LIBFFT}")
message("LIBIPC       in e7_atten_prj is ${LIBIPC}")
message("LIBCONVERT   in e7_atten_prj is ${LIBCONVERT}")
message("LIBIMAGE     in e7_atten_prj is ${LIBIMAGE}")
message("LIBATTEN     in e7_atten_prj is ${LIBATTEN}")
message("LIBFILTER    in e7_atten_prj is ${LIBFILTER}")

# Hack.  Don't build first time round, since libraries not made yet.
if(LIBCOMMON)
	add_executable (e7_atten_u e7_atten.cpp)
	target_link_libraries (e7_atten_u LINK_PUBLIC pthread ${LIBSINOGRAM} ${LIBECAT7} ${LIBINTERFILE} ${LIBPARSER} ${LIBFFT} ${LIBIPC} ${LIBEXCEPTION} ${LIBIMAGE} ${LIBCONVERT} ${LIBATTEN} ${LIBFILTER} ${LIBCOMMON} )
	target_include_directories (e7_atten_u PUBLIC ${MY_INCLUDE_DIRS})
endif()

# message("LIB_INCLUDE_DIRS in e7_atten_prj are ${LIB_INCLUDE_DIRS}")

install(TARGETS e7_atten_u
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
