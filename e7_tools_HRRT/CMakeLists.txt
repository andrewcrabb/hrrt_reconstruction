add_subdirectory(libatten)
add_subdirectory(libcommon)

# target_include_directories (libatten PUBLIC
# include_directories (
set (LIB_INCLUDE_DIRS
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libcommon
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libexception
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libecat7
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libimage
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libinterfile
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libparser
	${CMAKE_SOURCE_DIR}/e7_tools_HRRT/libsinogram
	)

message("LIB_INCLUDE_DIRS are ${LIB_INCLUDE_DIRS}")

target_include_directories (libatten PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories (libcommon PUBLIC ${LIB_INCLUDE_DIRS})

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
