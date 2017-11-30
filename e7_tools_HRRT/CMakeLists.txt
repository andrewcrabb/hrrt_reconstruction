# Note that all subdirs inherit this very long list.
# Might be worth doing them individually in each subdir
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
find_library (LIBSCATTER   NAME scatter    PATHS "${CMAKE_SOURCE_DIR}/libscatter")
message("LIBSCATTER in root is ${LIBSCATTER}")

add_subdirectory(libatten)
add_subdirectory(libcommon)
add_subdirectory(libconvert)
add_subdirectory(libecat7)
add_subdirectory(libexception)
add_subdirectory(libfft)
add_subdirectory(libfilter)
add_subdirectory(libimage)
add_subdirectory(libinterfile)
add_subdirectory(libipc)
add_subdirectory(libparser)
add_subdirectory(libsinogram)
add_subdirectory(e7_atten_prj)
add_subdirectory(e7_fwd_prj)
add_subdirectory(e7_sino_prj)

# find_library (LIBCOMMON    NAME common    PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libcommon")
# find_library (LIBEXCEPTION NAME exception PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libexception")
# find_library (LIBSINOGRAM  NAME sinogram  PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libsinogram")
# find_library (LIBECAT7     NAME ecat7     PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libecat7")
# find_library (LIBINTERFILE NAME interfile PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libinterfile")
# find_library (LIBPARSER    NAME parser    PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libparser")
# find_library (LIBFFT       NAME fft       PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libfft")
# find_library (LIBIPC       NAME ipc       PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libipc")
# find_library (LIBCONVERT   NAME convert   PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libconvert")
# find_library (LIBIMAGE     NAME image     PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libimage")
# find_library (LIBATTEN     NAME atten     PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libatten")
# find_library (LIBFILTER    NAME filter    PATHS "${CMAKE_BINARY_DIR}/e7_tools_HRRT/libfilter")

target_include_directories (atten     PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (common    PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (convert   PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (ecat7     PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (exception PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (fft       PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (filter    PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (image     PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (interfile PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (ipc       PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (parser    PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (sinogram  PUBLIC ${LIB_INCLUDE_DIRS})

# list(APPEND includePath 	${CMAKE_SOURCE_DIR}
# 	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libcommon
# 	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libexception
# 	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libecat7
# 	)



# add_library (motion_distance SHARED motion_distance.cpp )

# target_include_directories (motion_distance PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ../AIR5.3.0/src)
# install(TARGETS motion_distance
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
