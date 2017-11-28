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
	)

message("LIB_INCLUDE_DIRS are ${LIB_INCLUDE_DIRS}")

target_include_directories (libatten     PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (libcommon    PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (libconvert   PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (libecat7     PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (libexception PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (libfft       PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (libfilter    PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (libimage     PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (libinterfile PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (libipc PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (libparser PUBLIC ${LIB_INCLUDE_DIRS})

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
